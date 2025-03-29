import asciidocPlugin from 'eleventy-plugin-asciidoc';
import * as sass from 'sass';
import path from 'path';
import markdownIt from 'markdown-it';
import markdownItAnchor from 'markdown-it-anchor';
import tocPlugin from './_plugins/toc.js';
import { emptyDirSync } from 'fs-extra';
import { readFileSync, writeFileSync } from 'fs';
import * as cheerio from 'cheerio';
import * as pagefind from 'pagefind';

// Copy external files to the input directory
addExternalFiles([
  ['CHANGELOG.md', '/changelog/'],
  ['LICENSE.md', '/license/'],
  ['CODE-OF-CONDUCT.adoc', '/code-of-conduct/']
]);

export default function (eleventyConfig) {
  // Set output directory
  eleventyConfig.setOutputDirectory('public');

  // Set layout directory
  eleventyConfig.setLayoutsDirectory('_layouts');

  // Include files that are not part of the repo
  eleventyConfig.setUseGitIgnore(false);

  // Configure Markdown
  eleventyConfig.setLibrary('md', markdownIt().use(markdownItAnchor));

  // Enable AsciiDoc syntax, transforming AsciiDoc HTML to Bootstrap CSS
  eleventyConfig.addPlugin(asciidocPlugin, {
    attributes: { idprefix: '', idseparator: '-', lead: false }
  });
  eleventyConfig.addTransform('transformAsciidocHtml', function (content) {
    return this.page.inputPath.endsWith('.adoc') && content
      ? transformAsciidocHtml(content)
      : content;
  });

  // Enable Sass support
  eleventyConfig.addTemplateFormats('scss');
  eleventyConfig.addExtension('scss', {
    outputFileExtension: 'css',
    compile: async (inputContent, inputPath) => {
      const result = sass.compileString(inputContent, {
        loadPaths: [path.dirname(inputPath)],
        silenceDeprecations: ['color-functions', 'global-builtin', 'import']
      });
      return async () => result.css;
    }
  });

  // Enable TOC
  eleventyConfig.addPlugin(tocPlugin);

  // Add lookup data for navigation titles
  eleventyConfig.addCollection('titlesByUrl', collectionsApi =>
    Object.fromEntries(
      collectionsApi
        .getAll()
        .filter(page => page.url && page.data.title)
        .map(page => [page.url, page.data.title])
    )
  );

  // Copy images
  // Regarding syntax, see https://github.com/11ty/eleventy/issues/2461#issuecomment-1238279042
  eleventyConfig.addPassthroughCopy('{,!(public|node_modules)/**/}*.png');
  eleventyConfig.addPassthroughCopy('{,!(public|node_modules)/**/}*.svg');
  eleventyConfig.addPassthroughCopy('{,!(public|node_modules)/**/}*.webp');
  eleventyConfig.addPassthroughCopy('favicon.ico');

  // Copy libraries
  eleventyConfig.addPassthroughCopy('node_modules/bootstrap/dist/js/bootstrap.min.*');

  // Build search index
  eleventyConfig.on('eleventy.after', async args => {
    const { index } = await pagefind.createIndex();
    await index.addDirectory({ path: args.directories.output });
    await index.writeFiles({
      outputPath: path.join(args.directories.output, 'pagefind')
    });
  });
}

function addExternalFiles(specs) {
  const externalDir = './external';
  emptyDirSync(externalDir);
  specs.forEach(([inputPath, permalink]) => {
    let content = readFileSync(path.join('..', inputPath), 'utf-8');
    if (inputPath.endsWith('.adoc')) {
      content = content.replace(
        /^= (.*)\r?\n/,
        '= $1\n' +
          ':eleventy-layout: page\n' +
          `:eleventy-permalink: ${permalink}\n` +
          `:eleventy-originalPath: ${inputPath}\n`
      );
    } else if (inputPath.endsWith('.md')) {
      content = content.replace(
        /^# (.*)\r?\n/,
        '---\n' +
          'title: $1\n' +
          'layout: page\n' +
          `permalink: ${permalink}\n` +
          `originalPath: ${inputPath}\n` +
          '---\n'
      );
    }

    const extension = path.extname(inputPath);
    const outputFilePath = path.join(externalDir, path.posix.basename(permalink) + extension);
    writeFileSync(outputFilePath, content);
  });
}

function transformAsciidocHtml(content) {
  const $ = cheerio.load(content);

  // Transform blocks
  $(`.admonitionblock, .exampleblock, .listingblock, .sidebarblock`).each((_, block) => {
    const $block = $(block);

    const $content = $block.find('> .content, > table > tbody > tr > td.content');

    // AsciiDoc blocks can have an outer title (rendered before the block) and/or an inner title
    // (rendered inside the block)
    const outerTitle = $block.find('> .title, > table > tbody > tr > td.icon > .title').html();
    const innerTitle = $content.find('> .title').remove().html();

    const classes = $block.attr('class').split(' ');
    const isAdmonition = classes.includes('admonitionblock');
    const admonition = isAdmonition
      ? (() => {
          const type = classes.find(c => c.match(/^(note|tip|important|caution|warning)$/));
          const [_class, icon] = {
            note: ['info', 'bi-info-circle'],
            tip: ['success', 'bi-lightbulb'],
            important: ['warning', 'bi-exclamation-circle'],
            caution: ['danger', 'bi-exclamation-triangle'],
            warning: ['danger', 'bi-exclamation-triangle']
          }[type];
          const badge = `
            <span class="badge text-bg-${_class} position-absolute start-0 ms-3 top-0 translate-middle-y">
              <i class="bi ${icon}"></i> ${outerTitle}
            </span>`;
          return { type, _class, icon, badge };
        })()
      : null;

    $block.replaceWith(`
      ${outerTitle && !isAdmonition ? `<div class="title">${outerTitle}</div>` : ''}
      <div class="card mb-3 ${
        isAdmonition
          ? `mt-4 border-${admonition._class} bg-${admonition._class}-subtle`
          : 'bg-light'
      }">
        ${admonition ? admonition.badge : ''}
        <div class="card-body">
          ${innerTitle ? `<h5 class="card-title">${innerTitle}</h5>` : ''}
          ${$content.html()}
        </div>
      </div>
    `);
  });

  // Transform tables
  $('table').each((_, table) => {
    const $table = $(table);
    $table.removeAttr('class');
    $table.addClass('table');
  });

  // Transform YouTube embeds to prevent cookies
  $('iframe').each((_, iframe) => {
    const $iframe = $(iframe);
    const src = $iframe.attr('src');

    const match = src?.match(/^https:\/\/www\.youtube\.com\/embed\/([^?&]+)/);
    if (!match) return;

    const videoId = match[1];
    const $wrapper = $iframe.closest('.videoblock');
    if ($wrapper.length > 0) {
      $wrapper.replaceWith(`<lite-youtube videoid="${videoId}"></lite-youtube>`);
    }
  });

  // Remove unnecessary remaining nesting
  for (const selector of [
    ...[1, 2, 3, 4, 5].map(i => `div.sect${i}`),
    'div.sectionbody',
    'div.paragraph',
    'div.ulist',
    'li > p:only-child',
    'td > p:only-child'
  ]) {
    $(`${selector}`).each((_, element) => {
      $(element).replaceWith($(element).html());
    });
  }

  return $.html();
}
