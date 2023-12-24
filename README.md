# Volume Supervisor

Volume Supervisor is a Node.js library that allows you to control system and application volume. It provides a simple and intuitive API to get and set the global volume, mute and unmute the system, and get the status of the system.
The project is inspired by [easy-volume](https://github.com/Arciiix/easy-volume) by [Arciiix](https://github.com/Arciiix).

This is in a really early stage of development, so expect bugs and missing features. The API is also subject to consequential changes.

## Installation

You can install Volume Supervisor using npm:

```bash
$ npm install volume_supervisor
$ yarn add volume_supervisor
$ pnpm add volume_supervisor
```

## Compatibility

Currently, Volume Supervisor only fully supports Linux and basic function on windows. The windows support is planned to be expended in the near future.
If you are a Mac user and would like to contribute to this project, feel free to fork the repository and make a merge request.

### Functions Compatibility

The compatibility of features on linux depends on whether `amixer`, `wireplumber` or `pulseaudio` is installed on your Linux system.
Here is a table that outlines the compatibility:

| Feature                | Amixer | Wireplumber | Pulseaudio | Windows |
|------------------------|--------|-------------|------------|---------|
| Global volume features | Yes    | Yes         | Yes        | Yes     |
| Audio status           | No     | Yes         | Yes        | Yes     |
| List streams           | No     | Yes         | Yes        | Yes     |
| List sinks             | No     | Yes         | Yes        | Yes     |
| List sources           | No     | Yes         | Yes        | Yes     |
| Stream volume features | No     | Yes         | Yes        | Yes     |
| Sink volume features   | No     | Yes         | Yes        | Yes     |
| Source volume features | No     | Yes         | Yes        | Yes     |
| Get stream destination | No     | No          | Yes        | No      |
| Set stream destination | No     | No          | Yes        | No      |

Priority for linux: `pulseaudio` (`pactl`) > `wireplumber` (`wpctl`) > `amixer`

Volume features correspond to get/set volume and mute/unmute.

## Usage

Here is a basic example of how to use Volume Supervisor:

```typescript
import { volumeControl } from 'volume_supervisor';

// Get the global volume
volumeControl.getGlobalVolume().then(volume => {
  console.log(`Current volume is ${volume}`);
});

// Set the global volume
volumeControl.setGlobalVolume(50).then(() => {
  console.log('Volume has been set to 50');
});

// Mute the system
volumeControl.setGlobalMuted(true).then(() => {
  console.log('System has been muted');
});
```

### Per node volume

Volume Supervisor also allows you to get and set the volume of individual nodes. Here is an example:

```typescript
import { volumeControl } from 'volume_supervisor';

// Get the status of the system
volumeControl.getStatus().then(async status => {
  // Get the first stream
  const stream = status.streams[0];
  if (!stream) {
    console.log('No streams found');
    return;
  }

  // Display the stream's name
  console.log(`Stream name: ${stream.name}`); // e.g. 'Firefox'

  // Get the stream's volume
  const volume = (await volumeControl.getNodeVolumeInfoById(stream.id)).volume;
  console.log(`Stream volume: ${volume}`); // e.g. 50

  // Increase the stream's volume by 10%
  await volumeControl.setNodeVolumeById(stream.id, volume + 10);
  
  // Set new destination for the stream
  await volumeControl.setStreamDestination(stream.id, status.sinks[0].id);
});
```

## Types

All the types used in the API are defined in the `types.ts` file. Here is a list of the types:
Here are some of the types used in the API:
```typescript
export type VolumeInfo = {
  volume: number;
  muted: boolean;
};

export type VsNodeTypes = 'sink' | 'source' | 'stream';

export type VsNode = {
  type: VsNodeTypes;
  id: string;
  name: string;
  isDefault: boolean;
} & VolumeInfo;

export type VsStreamNode = VsNode & {
  type: 'stream';
  destinationId?: string;
};

export type SinkStatus = {
  sinks: VsNode[];
  defaultSink?: string;
};

export type SourceStatus = {
  sources: VsNode[];
  defaultSource?: string;
};

export type StreamStatus = {
  streams: VsStreamNode[];
};

export type Status = SinkStatus & SourceStatus & StreamStatus;
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.