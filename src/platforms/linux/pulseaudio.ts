import {
  PlatformImplementation,
  SinkStatus,
  SourceStatus,
  Status,
  StreamStatus,
  VolumeInfo,
  VsNode,
  VsNodeTypes,
  VsStreamNode,
} from '@/types';
import { execCommand } from '@/utils/commands';

const DEFAULT_SINK_NAME = '@DEFAULT_SINK@';
const DEFAULT_SOURCE_NAME = '@DEFAULT_SOURCE@';

function extractVolume(stdout: string) {
  const nodeRegex = /[a-zA-Z0-9\-]+:\s*\S* \/\s*(\d{1,3})% \/\s+\S+ dB/g;

  let volumeMatch = [...stdout.matchAll(nodeRegex)].map((match) => match[1]);
  if (!volumeMatch) {
    return null;
  }

  if (volumeMatch.length === 0) {
    return 0;
  }

  return Math.round(
    volumeMatch.map((v) => Number.parseInt(v, 10)).reduce((a, b) => a + b, 0) / volumeMatch.length,
  );
}

function extractMuted(stdout: string) {
  const split = stdout.trim().split(' ');
  const muted = split[split.length - 1];

  return muted === 'yes';
}

async function getTypeVolumeById(type: VsNodeTypes, id: string) {
  if (type === 'stream') {
    const streamStatus = await getStreamStatus();
    const stream = streamStatus.streams.find((stream) => stream.id === id);
    if (!stream) throw new Error('Failed to get volume');

    return stream.volume;
  }

  const stdout = await execCommand('pactl', [`get-${type}-volume`, id]);
  if (!stdout) throw new Error('Failed to get volume');

  return extractVolume(stdout);
}

async function setTypeVolumeById(type: VsNodeTypes, id: string, volume: number) {
  if (volume < 0 || volume > 100) throw new Error('Volume must be between 0 and 100');

  const convertedType = type === 'stream' ? 'sink-input' : type;
  return execCommand('pactl', [`set-${convertedType}-volume`, id, `${volume}%`]);
}

async function getTypeMutedById(type: VsNodeTypes, id: string) {
  if (type === 'stream') {
    const streamStatus = await getStreamStatus();
    const stream = streamStatus.streams.find((stream) => stream.id === id);
    if (!stream) throw new Error('Failed to get volume');

    return stream.muted;
  }

  const stdout = await execCommand('pactl', [`get-${type}-mute`, id]);
  if (!stdout) throw new Error('Failed to get volume');

  return extractMuted(stdout);
}

async function setTypeMuteById(type: VsNodeTypes, id: string, muted: boolean) {

  const convertedType = type === 'stream' ? 'sink-input' : type;
  return execCommand('pactl', [`set-${convertedType}-mute`, id, muted ? '1' : '0']);
}

function exportStatusOutput(stdout: string) {
  const lines = stdout.split('\n');
  const linesWithLevel = lines.map((line) => {
    const startSpaces = line.search(/\S/);
    return {
      level: startSpaces,
      line: line.trim(),
    };
  });

  const obj: any = {};
  const prevObjs: any[] = [];
  let currentLevel = 0;
  let currentKey = '';
  let currentObj = obj;

  for (const { level, line } of linesWithLevel) {
    if (level === -1) {
      currentObj = obj;
      currentLevel = 0;
      continue;
    }

    if (level === currentLevel + 1) {
      prevObjs.push(currentObj);
      currentObj = currentObj[currentKey];
      currentLevel = level;
    } else if (level < currentLevel) {
      for (let i = 0; i < currentLevel - level; i++) {
        currentObj = prevObjs.pop();
      }
      currentLevel = level;
    } else if (level > currentLevel + 1) {
      if (typeof currentObj[currentKey] === 'string') {
        currentObj[currentKey] = currentObj[currentKey] + '\n' + line;
        continue;
      }
    }

    const [key, ...values] = line.split(':').map((s) => s.trim());
    const value = values.join(':');
    if (key.length > 0) {
      currentKey = key;
      if (value) {
        currentObj[currentKey] = value;
      } else {
        currentObj[currentKey] = {};
      }
    } else {
    }
  }

  return obj;
}

async function getSinkStatus(): Promise<SinkStatus> {
  const stdout = await execCommand('pactl', ['list', 'sinks']);
  if (!stdout) return { sinks: [] };

  const stdoutDefaultSink = await execCommand('pactl', ['get-default-sink']);
  if (!stdoutDefaultSink) throw new Error('Failed to get default sink');
  const defaultSinkName = stdoutDefaultSink.trim();

  let defaultSink: string | undefined = undefined;

  const obj = exportStatusOutput(stdout);
  const sinks: VsNode[] = Object.keys(obj).map((key) => {
    const properties = obj[key];
    const name = properties['Description'];
    const volume = extractVolume(properties['Volume']);
    const muted = properties['Mute'] === 'yes';
    const isDefault = properties['Name'] === defaultSinkName;
    const id = key.split(' ').at(-1).substring(1);

    if (isDefault) {
      defaultSink = id;
    }

    return {
      type: 'sink',
      id,
      name,
      volume,
      muted,
      isDefault,
    };
  });

  return {
    sinks,
    defaultSink,
  };
}

async function getSourceStatus(): Promise<SourceStatus> {
  const stdout = await execCommand('pactl', ['list', 'sources']);
  if (!stdout) return { sources: [] };

  const stdoutDefaultSource = await execCommand('pactl', ['get-default-source']);
  if (!stdoutDefaultSource) throw new Error('Failed to get default source');
  const defaultSourceName = stdoutDefaultSource.trim();

  let defaultSource: string | undefined = undefined;

  const obj = exportStatusOutput(stdout);
  const sources: VsNode[] = Object.keys(obj).map((key) => {
    const properties = obj[key];
    const name = properties['Description'];
    const volume = extractVolume(properties['Volume']);
    const muted = properties['Mute'] === 'yes';
    const isDefault = properties['Name'] === defaultSourceName;
    const id = key.split(' ').at(-1).substring(1);

    if (isDefault) {
      defaultSource = id;
    }

    return {
      type: 'source',
      id,
      name,
      volume,
      muted,
      isDefault,
    };
  });

  return {
    sources,
    defaultSource,
  };
}

async function getStreamStatus(): Promise<StreamStatus> {
  const stdout = await execCommand('pactl', ['list', 'sink-inputs']);
  if (!stdout) return { streams: [] };

  const obj = exportStatusOutput(stdout);
  const streams: VsStreamNode[] = Object.keys(obj).map((key) => {
    const properties = obj[key];
    const name = properties['application.name'];
    const volume = extractVolume(properties['Volume']);
    const muted = properties['Mute'] === 'yes';
    const id = key.split(' ').at(-1).substring(1);
    const destination = properties['Sink'];

    return {
      type: 'stream',
      id,
      name,
      volume,
      muted,
      isDefault: false,
      destinationId: destination,
    };
  });

  return {
    streams,
  };
}

async function isType(id: string, type: VsNodeTypes) {
  const convertedType = type === 'stream' ? 'sink-input' : type;

  const stdout = await execCommand('pactl', ['list', 'short', convertedType + 's']);
  if (!stdout) return false;

  return stdout.split('\n').map((line) => line.split('\t')[0]).includes(id);
}

async function getNodeTypeById(id: string) {
  if (await isType(id, 'sink')) {
    return 'sink';
  } else if (await isType(id, 'source')) {
    return 'source';
  } else if (await isType(id, 'stream')) {
    return 'stream';
  } else {
    return null;
  }
}

async function getStatus(): Promise<Status> {
  const stdout = await execCommand('pactl', ['info']);
  if (!stdout) throw new Error('Failed to get status');

  const sinkStatus = await getSinkStatus();
  const sourceStatus = await getSourceStatus();
  const streamStatus = await getStreamStatus();

  return {
    streams: streamStatus.streams,
    sinks: sinkStatus.sinks,
    sources: sourceStatus.sources,
    defaultSink: sinkStatus.defaultSink,
    defaultSource: sourceStatus.defaultSource,
  };
}

async function setStreamDestination(streamId: string, destinationId: string) {
  await execCommand('pactl', ['move-sink-input', streamId, destinationId]);
}

export const linuxPulseAudio: PlatformImplementation = {
  getPlatformCompatibility: () => ({
    status: true,
    listStreams: true,
    listSinks: true,
    listSources: true,
    setStreamVolume: true,
    setSinkVolume: true,
    setSourceVolume: true,
    getStreamDestination: true,
    setStreamDestination: true,
  }),
  async getGlobalVolume() {
    return getTypeVolumeById('sink', DEFAULT_SINK_NAME);
  },
  async setGlobalVolume(volume: number) {
    await setTypeVolumeById('sink', DEFAULT_SINK_NAME, volume);
  },
  isGlobalMuted: async function () {
    return getTypeMutedById('sink', DEFAULT_SINK_NAME);
  },
  async setGlobalMuted(muted: boolean) {
    await setTypeMuteById('sink', DEFAULT_SINK_NAME, muted);
  },
  getStatus: getStatus,
  async getNodeVolumeInfoById(id: string): Promise<VolumeInfo> {
    const type = await getNodeTypeById(id);
    if (!type) throw new Error('Failed to get node type');

    return {
      volume: await getTypeVolumeById(type, id),
      muted: await getTypeMutedById(type, id),
    };
  },
  async setNodeVolumeById(id: string, volume: number) {
    const type = await getNodeTypeById(id);
    if (!type) throw new Error('Failed to get node type');

    await setTypeVolumeById(type, id, volume);
  },
  async setNodeMutedById(id: string, muted: boolean) {
    const type = await getNodeTypeById(id);
    if (!type) throw new Error('Failed to get node type');

    await setTypeMuteById(type, id, muted);
  },
  setStreamDestination,
};