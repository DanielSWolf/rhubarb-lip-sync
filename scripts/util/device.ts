export type Platform = `${OperatingSystem}-${CpuArchitecture}`;

export type OperatingSystem = 'windows' | 'macos' | 'linux';

export type CpuArchitecture = 'amd64' | 'arm64';

export const device = {
  platform: getPlatform(),
  operatingSystem: getOperatingSystem(),
  cpuArchitecture: getCpuArchitecture(),
};

function getPlatform(): Platform {
  return `${getOperatingSystem()}-${getCpuArchitecture()}`;
}

function getOperatingSystem(): OperatingSystem {
  const platform = process.platform;
  if (platform === 'win32') return 'windows';
  if (platform === 'darwin') return 'macos';
  if (platform === 'linux') return 'linux';
  throw new Error(`Unsupported operating system: ${platform}`);
}

function getCpuArchitecture(): CpuArchitecture {
  const arch = process.arch;
  if (arch === 'x64') return 'amd64';
  if (arch === 'arm64') return 'arm64';
  throw new Error(`Unsupported architecture: ${arch}`);
}
