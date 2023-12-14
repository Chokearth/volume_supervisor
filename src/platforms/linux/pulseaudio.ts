import { PlatformImplementation } from '@/types';
import { throwCompatibilityError } from '@/utils/errors';
import { execCommand } from '@/utils/commands';

const DEFAULT_SINK_NAME = '@DEFAULT_SINK@';
const DEFAULT_SOURCE_NAME = '@DEFAULT_SOURCE@';

function extractVolume(stdout: string) {
  const nodeRegex = /[a-zA-Z0-9\-]+:\s*\S+ \/\s*(\d{1,3})% \/\s+\S+ dB/g;

  let volumeMatch = [...stdout.matchAll(nodeRegex)].map((match) => match[1]);
  if (!volumeMatch) {
    return null;
  }

  return Math.round(
    volumeMatch.map((v) => Number.parseInt(v, 10)).reduce((a, b) => a + b) / volumeMatch.length,
  );
}

function extractMuted(stdout: string) {
  const split = stdout.trim().split(' ');
  const muted = split[split.length - 1];

  return muted === 'yes';
}

async function getSinkVolumeById(id: string) {
  const stdout = await execCommand('pactl', ['get-sink-volume', id]);
  if (!stdout) throw new Error('Failed to get volume');

  return extractVolume(stdout);
}

async function setSinkVolumeById(id: string, volume: number) {
  if (volume < 0 || volume > 100) throw new Error('Volume must be between 0 and 100');

  return execCommand('pactl', ['set-sink-volume', id, `${volume}%`]);
}

async function getSinkMutedById(id: string) {
  const stdout = await execCommand('pactl', ['get-sink-mute', id]);
  if (!stdout) throw new Error('Failed to get mute status');

  return extractMuted(stdout);
}

async function setSinkMutedById(id: string, muted: boolean) {
  return execCommand('pactl', ['set-sink-mute', id, muted ? '1' : '0']);
}

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
    return getSinkVolumeById(DEFAULT_SINK_NAME);
  },
  async setGlobalVolume(volume: number) {
    await setSinkVolumeById(DEFAULT_SINK_NAME, volume);
  },
  isGlobalMuted: async function () {
    return getSinkMutedById(DEFAULT_SINK_NAME);
  },
  async setGlobalMuted(muted: boolean) {
    await setSinkMutedById(DEFAULT_SINK_NAME, muted);
  },
  getStatus: throwCompatibilityError,
  getNodeVolumeInfoById: throwCompatibilityError,
  setNodeVolumeById: throwCompatibilityError,
  setNodeMutedById: throwCompatibilityError,
};