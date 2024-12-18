import asciidoctor from 'asciidoctor';

const Asciidoctor = asciidoctor();

export function asciidocToHtml(asciidoc: string): string {
  const oldLogger = Asciidoctor.LoggerManager.getLogger();
  const memoryLogger = Asciidoctor.MemoryLogger.create();

  try {
    Asciidoctor.LoggerManager.setLogger(memoryLogger);

    const document = Asciidoctor.load(asciidoc, {
      standalone: true,
      attributes: { linkcss: false },
    });
    const html = document.convert();

    const messages = memoryLogger.getMessages();
    if (messages.length > 0) {
      const messagesString = messages
        .map(message => `${message.getSeverity()}: ${message.getText()}`)
        .join('\n');
      const excerpt = asciidoc.split('\n').slice(0, 10).join('\n') + '\n...';
      throw new Error(`Asciidoctor failed:\n${messagesString}\n\nOriginal markup:\n${excerpt}`);
    }

    return html;
  } finally {
    Asciidoctor.LoggerManager.setLogger(oldLogger);
  }
}

export function markdownToHtml(markdown: string): string {
  return asciidocToHtml(markdownToAsciidoc(markdown));
}

/** Cheap best-effort heuristics for converting Markdown to AsciiDoc. */
export function markdownToAsciidoc(markdown: string): string {
  let text = markdown;

  // Convert headings
  text = text.replaceAll(/^#+/gm, m => '='.repeat(m.length));

  // Convert italics
  text = text.replaceAll(
    /(?<![*_])[*_]((?!\s)[^*_\n]+(?<!\s))[*_](?![*_])/g,
    (_, inner: string) => `_${inner}_`,
  );

  // Convert boldface
  text = text.replaceAll(
    /(?<![*_])[*_]{2}((?!\s)[^*_\n]+(?<!\s))[*_]{2}(?![*_])/g,
    (_, inner: string) => `*${inner}*`,
  );

  // Convert links
  text = text.replaceAll(
    /\[([^\]]+)\]\(([^)]+)\)/g,
    (_, label: string, url: string) => `${url}[${label}]`,
  );

  // Convert continuations
  text = text.replaceAll(/\n\n\s+/g, '\n+\n');

  return text;
}
