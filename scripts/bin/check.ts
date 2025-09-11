// Checks whether all code is properly formatted.
// Exits with a non-zero code if any formatting issues are found.

import { checkCodeFormatting } from '../code-formatting';

async function main() {
  await checkCodeFormatting();
}

main();
