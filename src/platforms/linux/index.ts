import * as child_process from 'child_process';
import { PlatformImplementation } from '@/types';
import { linuxWireplumber } from '@/platforms/linux/wireplumber';
import { linuxPulseAudio } from '@/platforms/linux/pulseaudio';
import { linuxAmixer } from '@/platforms/linux/amixer';

function testCommand(command: string): boolean {
  try {
    child_process.execSync(`command -v ${command} 2>/dev/null` +
      ' && { echo >&1 wpctl; exit 0; }').toString();
    return true;
  } catch (e) {
    return false;
  }
}

const wpctlCompatible = testCommand('wpctl');
const pactlCompatible = testCommand('pactl');

export const linux: PlatformImplementation =
  wpctlCompatible ? linuxWireplumber :
    pactlCompatible ? linuxPulseAudio :
      linuxAmixer;

