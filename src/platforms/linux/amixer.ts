import { PlatformImplementation } from '@/types';
import { execCommand } from '@/utils/commands';
import { throwCompatibilityError } from '@/utils/errors';

export const linuxAmixer: PlatformImplementation = {
  getPlatformCompatibility: () => ({
    status: false,
    listStreams: false,
    listSinks: false,
    listSources: false,
    setStreamVolume: false,
    setSinkVolume: false,
    setSourceVolume: false,
  }),
  async getGlobalVolume() {

    const stdout = await execCommand('amixer', ['sget', 'Master']);
    if (!stdout) throw new Error('Failed to get volume');

    const channels = stdout.match(/(?<=\[)(\d+)(?=%\])/g); // This regex finds a digit(s) which is prefixed with "[" and suffixed with "%]"
    if (!channels.length) throw new Error('Failed to get volume');

    const sum = channels.reduce((acc, channel) => acc + parseInt(channel), 0);

    if (isNaN(sum)) throw new Error('Failed to get volume');

    return sum / channels.length;
  },
  async setGlobalVolume(volume: number) {
    if (volume < 0 || volume > 100) throw new Error('Volume must be between 0 and 100');

    await execCommand('amixer', ['set', 'Master', `${volume}%`]);
  },
  isGlobalMuted: async function () {
    const stdout = await execCommand('amixer', ['sget', 'Master']);
    if (!stdout) throw new Error('Failed to get mute state');

    return stdout
      .match(/\[(on|off)\]/)
      .some((state) => state === 'off');
  },
  async setGlobalMuted(muted: boolean) {
    await execCommand('amixer', ['set', 'Master', muted ? 'mute' : 'unmute']);
  },
  getStatus: throwCompatibilityError,
  getNodeVolumeInfoById: throwCompatibilityError,
  setNodeVolumeById: throwCompatibilityError,
  setNodeMutedById: throwCompatibilityError,
};