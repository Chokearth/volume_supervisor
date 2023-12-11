import * as child_process from 'child_process';
import { PlatformImplementation } from '@/types';
import { linuxWireplumber } from '@/platforms/linux/wireplumber';
import { linuxAmixer } from '@/platforms/linux/amixer';

let wpctlCompatible: boolean;
try {
  const stdout = child_process.execSync('command -v wpctl 2>/dev/null' +
    ' && { echo >&1 wpctl; exit 0; }').toString();
  wpctlCompatible = !!stdout;
} catch (e) {
  wpctlCompatible = false;
}

export const linux: PlatformImplementation = wpctlCompatible ? linuxWireplumber : linuxAmixer;

