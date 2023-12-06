import { execCommand } from '@/utils/commands';
import * as child_process from 'child_process';
import { PlatformImplementation } from '@/types';
import { linuxAmixer } from '@/platforms/linux/amixer';
import { linuxPactl } from '@/platforms/linux/pactl';

let pactlCompatible: boolean;
try {
  const stdout = child_process.execSync('command -v pactl 2>/dev/null' +
              ' && { echo >&1 pactl; exit 0; }').toString();
  pactlCompatible = !!stdout;
} catch (e) {
  pactlCompatible = false;
}

export const linux: PlatformImplementation = pactlCompatible ? linuxPactl : linuxAmixer;

