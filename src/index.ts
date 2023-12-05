import * as os from 'os';

const osType = os.type();

switch (osType) {
  case 'Linux':
    break;
  default:
    throw new Error('Unsupported OS found: ' + osType);
}

console.log('OS type: ' + osType);