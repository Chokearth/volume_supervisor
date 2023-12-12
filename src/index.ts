import * as os from 'os';
import { PlatformImplementation } from '@/types';
import { linux } from '@/platforms/linux';

const osType = os.type();

let platformImplementation: PlatformImplementation;

switch (osType) {
  case 'Linux':
    platformImplementation = linux;
    break;
  default:
    throw new Error('Unsupported OS found: ' + osType);
}

export const volumeControl = platformImplementation;