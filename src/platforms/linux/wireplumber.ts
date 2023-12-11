import { Node, NodeTypes, PlatformImplementation, Status, VolumeInfo } from '@/types';
import { execCommand } from '@/utils/commands';

const SUB_SECTION_TO_EXTRACT: {
  name: string;
  attribute: keyof Status;
  type: Node['type'];
  defaultAttribute: keyof Status;
}[] = [
  { name: 'Sinks', attribute: 'sinks', type: 'sink', defaultAttribute: 'defaultSink' },
  { name: 'Sources', attribute: 'sources', type: 'source', defaultAttribute: 'defaultSource' },
  { name: 'Streams', attribute: 'streams', type: 'stream', defaultAttribute: undefined },
];

function exportSinksOrSources(lines: string[], type: NodeTypes) {
  const nodes: Node[] = [];

  for (const line of lines) {
    const match = line.match(/(\*?)\s+(\d+)\.\s+(.+)\s+\[vol: ([\d\.]+) ?(MUTED)?\]/);
    if (!match) continue;

    const isDefault = match[1].startsWith('*');
    const id = match[2];
    const name = match[3].trim();
    const volume = Math.round(Number.parseFloat(match[4]) * 100);
    const muted = match[5] === 'MUTED';

    nodes.push({
      type,
      id,
      name,
      volume,
      muted,
      isDefault,
    });
  }

  return nodes;
}

function exportStreams(lines: string[]) {
  const nodes: Node[] = [];

  for (const line of lines) {
    if (line.startsWith('        ')) continue;
    const match = line.match(/\s+(\d+)\.\s+(.+)/);
    if (!match) continue;

    const id = match[1];
    const name = match[2].trim();

    nodes.push({
      type: 'stream',
      id,
      name,
      volume: 0,
      muted: false,
      isDefault: false,
    });
  }

  return nodes;
}

function exportStatus(stdout: string) {
  const status: Status = {
    sinks: [],
    sources: [],
    streams: [],
    defaultSink: undefined,
    defaultSource: undefined,
  };

  const sections = stdout.split('\n\n').reduce((acc, section) => {
    const [name, ...content] = section.split('\n');
    acc[name] = content.join('\n');
    return acc;
  }, {} as Record<string, string>);

  const audioSections = sections['Audio'];
  if (!audioSections) throw new Error('Failed to get audio sections');

  const subSections = audioSections.replace(/└/g, '├').split('├').slice(1).reduce((acc, section) => {
    const [nameLine, ...rawLines] = section.split('\n');

    const name = nameLine.match(/─ (.+):/)?.[1];
    if (!name) return acc;

    acc[name] = rawLines
      .map((line) => line.slice(4))
      .filter((line) => line.length > 0);
    return acc;
  }, {} as Record<string, string[]>);

  for (const { name, attribute, type, defaultAttribute } of SUB_SECTION_TO_EXTRACT) {
    const subSection = subSections[name];
    if (!subSection) throw new Error(`Failed to get sub section ${name}`);

    if (type === 'stream') {
      status['streams'] = exportStreams(subSection);
    } else {
      const nodes = exportSinksOrSources(subSection, type);
      const defaultSink = nodes.find((node) => node.isDefault);

      status[attribute as 'sinks' | 'sources'] = nodes;
      if (defaultSink) {
        status[defaultAttribute as 'defaultSink' | 'defaultSource'] = defaultSink.id;
      }
    }
  }

  return status;
}

async function getNodeVolumeInfoById(id: string): Promise<VolumeInfo> {
  const stdout = await execCommand('wpctl', ['get-volume', id]);
  if (!stdout) throw new Error('Failed to get volume');

  const splitted = stdout.trim().split(' ');
  const volumeStr = splitted[1];
  if (volumeStr === undefined) throw new Error('Failed to get volume');

  const volume = Number.parseFloat(volumeStr);
  if (isNaN(volume)) throw new Error('Failed to get volume');

  const muted = splitted[2] === '[MUTED]';

  return {
    volume: Math.round(volume * 100),
    muted,
  };
}

async function updateStreamStatus(stream: Node) {
  const volumeInfo = await getNodeVolumeInfoById(stream.id.toString());
  stream.volume = volumeInfo.volume;
  stream.muted = volumeInfo.muted;
}

async function updateStreamsStatus(streams: Node[]) {
  await Promise.all(streams.map(updateStreamStatus));
}

async function getStatus() {
  const stdout = await execCommand('wpctl', ['status']);
  if (!stdout) throw new Error('Failed to get volume');

  const status = exportStatus(stdout);
  await updateStreamsStatus(status.streams);

  return status;
}

async function setNodeVolumeById(id: string, volume: number) {
  if (volume < 0 || volume > 100) throw new Error('Volume must be between 0 and 100');

  await execCommand('wpctl', ['set-volume', id, (volume / 100).toString()]);
}

async function setNodeMutedById(id: string, muted: boolean) {
  await execCommand('wpctl', ['set-volume', id, muted ? '1' : '0']);
}

export const linuxWireplumber: PlatformImplementation = {
  getPlatformCompatibility: () => ({
    status: true,
    listStreams: true,
    listSinks: true,
    listSources: true,
    setStreamVolume: true,
    setSinkVolume: true,
    setSourceVolume: true,
  }),
  async getGlobalVolume() {
    return getNodeVolumeInfoById('@DEFAULT_AUDIO_SINK@').then((volumeInfo) => volumeInfo.volume);
  },
  async setGlobalVolume(volume: number) {
    await setNodeVolumeById('@DEFAULT_AUDIO_SINK@', volume);
  },
  isMuted: async function () {
    return getNodeVolumeInfoById('@DEFAULT_AUDIO_SINK@').then((volumeInfo) => volumeInfo.muted);
  },
  async setMuted(muted: boolean) {
    await setNodeMutedById('@DEFAULT_AUDIO_SINK@', muted);
  },
  getStatus,
  getNodeVolumeInfoById,
  setNodeVolumeById,
  setNodeMutedById,
};