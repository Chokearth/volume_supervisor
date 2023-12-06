import { PlatformImplementation } from '@/types';

export const linuxPactl: PlatformImplementation = {
  async getGlobalVolume() {
    throw new Error('Not implemented');
    return 0;
  },
  async setGlobalVolume(volume: number) {
    throw new Error('Not implemented');
  },
  isMuted: async function () {
    throw new Error('Not implemented');
    return false;
  },
  async setMuted(muted: boolean) {
    throw new Error('Not implemented');
  },
};