import { resolve as resolvePath, dirname } from 'path';
import { fileURLToPath } from 'url';

const fileName = fileURLToPath(import.meta.url);
const dirName = dirname(fileName);
export const repoDir = resolvePath(dirName, '..');
