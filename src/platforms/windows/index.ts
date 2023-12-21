import { PlatformImplementation, Status, VsNode, VsStreamNode } from '@/types';
import { throwCompatibilityError } from '@/utils/errors';
import ToElectronPath from '@/utils/toEletcronPath';
import { execCommand } from '@/utils/commands';
import { join } from 'path';

const EXE_NAME = 'vsExec.exe';

function execVsCmd(args: string[]) {
  return execCommand(
    ToElectronPath(join(__dirname, EXE_NAME)),
    args,
  );
}

export const windows: PlatformImplementation = {
  getPlatformCompatibility: () => ({
    status: true,
    listStreams: true,
    listSinks: true,
    listSources: true,
    setStreamVolume: false,
    setSinkVolume: true,
    setSourceVolume: true,
    getStreamDestination: false,
    setStreamDestination: false,
  }),
  async getGlobalVolume() {
    const res = await execVsCmd(['getGlobalVolume']);
    const volume = parseInt(res.trim());

    if (isNaN(volume) || volume === -1) throw new Error('Failed to get volume');
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
  async getStatus() {
    const sinksStr = await execVsCmd(['getSinks']);
    const sourcesStr = await execVsCmd(['getSources']);
    const streamsStr = await execVsCmd(['getStreams']);
    try {
      const sinks = JSON.parse(sinksStr) as VsNode[];
      const sources = JSON.parse(sourcesStr) as VsNode[];
      const streams = JSON.parse(streamsStr) as VsStreamNode[];

      const status: Status = {
        sinks,
        sources,
        streams,

        defaultSink: sinks.find((sink: any) => sink.isDefault)?.id,
        defaultSource: sources.find((source: any) => source.isDefault)?.id,
      };

      return status;
    } catch (e) {
      console.error(streamsStr);
      throw new Error('Failed to get status');
    }
  },
  getNodeVolumeInfoById: throwCompatibilityError,
  async setNodeVolumeById(id: string, volume: number) {
    if (volume < 0 || volume > 100) throw new Error('Volume must be between 0 and 100');

    await execVsCmd(['setVolumeById', id, volume.toString()]);
  },
  async setNodeMutedById(id: string) {
    await execVsCmd(['setMutedById', id, '1']);
  },
  setStreamDestination: throwCompatibilityError,
};