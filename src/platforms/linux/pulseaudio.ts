import { PlatformImplementation } from '@/types';
import { throwCompatibilityError } from '@/utils/errors';

export const linuxPulseAudio: PlatformImplementation = {
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
    return Promise.resolve(0);
  },
  async setGlobalVolume(volume: number) {

  },
  isMuted: async function () {
    return Promise.resolve(false);
  },
  async setMuted(muted: boolean) {
  },
  getStatus: throwCompatibilityError,
  getNodeVolumeInfoById: throwCompatibilityError,
  setNodeVolumeById: throwCompatibilityError,
  setNodeMutedById: throwCompatibilityError,
};