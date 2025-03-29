import * as cheerio from 'cheerio';

export default function tocPlugin(eleventyConfig) {
  eleventyConfig.addFilter('toc', function (content) {
    const minLevel = 2;
    const maxLevel = 4;

    const headingSelector = Array.from(
      { length: maxLevel - minLevel + 1 },
      (_, i) => `h${i + minLevel}`
    ).join(',');

    const $ = cheerio.load(content);
    const headings = $(headingSelector).toArray();

    if (headings.length < 2) return '';

    const root = { level: minLevel - 1, children: [] };
    const stack = [root];

    headings.forEach(el => {
      const $el = $(el);
      const level = parseInt(el.tagName[1], 10);
      const id = $el.attr('id');
      const text = $el
        .text()
        .replace(/\(.*?\)/g, '')
        .trim();

      if (!id) return;

      const node = { id, text, level, children: [] };

      while (stack.length && stack[stack.length - 1].level >= level) {
        stack.pop();
      }

      stack[stack.length - 1].children.push(node);
      stack.push(node);
    });

    function renderTree(nodes, nested = false) {
      if (!nodes.length) return '';
      return `<ul class="nav flex-column${nested ? ' ms-3' : ''}">${nodes
        .map(
          n => `
        <li class="nav-item">
          <a class="nav-link" href="#${n.id}">${n.text}</a>
          ${renderTree(n.children, true)}
        </li>
      `
        )
        .join('')}</ul>`;
    }

    return renderTree(root.children);
  });
}
