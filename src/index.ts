import * as os from 'os';
import { PlatformImplementation } from '@/types';
import { linux } from '@/platforms/linux';
import { windows } from '@/platforms/windows';

const osType = os.type();

let platformImplementation: PlatformImplementation;

switch (osType) {
  case 'Linux':
    platformImplementation = linux;
    break;
  case 'Windows_NT':
    platformImplementation = windows;
    break;
  default:
    throw new Error('Unsupported OS found: ' + osType);
}

export const volumeControl = platformImplementation;