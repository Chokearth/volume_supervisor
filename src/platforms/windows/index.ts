import { PlatformImplementation } from '@/types';
import { throwCompatibilityError } from '@/utils/errors';
import ToElectronPath from '@/utils/toEletcronPath';
import { execCommand } from '@/utils/commands';
import { join } from 'path';

const EXE_NAME = "vsExec.exe";

function execVsCmd(args: string[]) {
  return execCommand(
    ToElectronPath(join(__dirname, EXE_NAME)),
    args
  );
}

export const windows: PlatformImplementation = {
  getPlatformCompatibility: () => ({
    status: false,
    listStreams: false,
    listSinks: false,
    listSources: false,
    setStreamVolume: false,
    setSinkVolume: false,
    setSourceVolume: false,
    getStreamDestination: false,
    setStreamDestination: false,
  }),
  async getGlobalVolume() {
    const res = await execVsCmd(['getGlobalVolume']);
    const volume = parseInt(res.trim());

    if (isNaN(volume) ||volume === -1) throw new Error('Failed to get volume');
    return volume;
  },
  async setGlobalVolume(volume: number) {
    if (volume < 0 || volume > 100) throw new Error('Volume must be between 0 and 100');

    await execVsCmd(['setGlobalVolume', volume.toString()]);
  },
  async isGlobalMuted() {
    const res = await execVsCmd(['isGlobalMuted']);
    return res.trim() === '1';
  },
  async setGlobalMuted(muted: boolean) {
    await execVsCmd(['setGlobalMute', muted ? '1' : '0']);
  },
  getStatus: throwCompatibilityError,
  getNodeVolumeInfoById: throwCompatibilityError,
  setNodeVolumeById: throwCompatibilityError,
  setNodeMutedById: throwCompatibilityError,
  setStreamDestination: throwCompatibilityError,
}