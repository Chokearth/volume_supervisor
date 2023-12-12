# Volume Supervisor

Volume Supervisor is a Node.js library that allows you to control system and application volume. It provides a simple and intuitive API to get and set the global volume, mute and unmute the system, and get the status of the system.
The project is inspired by [easy-volume](https://github.com/Arciiix/easy-volume) by [Arciiix](https://github.com/Arciiix).

## Installation

You can install Volume Supervisor using npm:

```bash
$ npm install volume_supervisor
$ yarn add volume_supervisor
$ pnpm add volume_supervisor
```

## Compatibility

Currently, Volume Supervisor only supports Linux. However, support for Windows is planned for future releases. If you are a Mac user and would like to contribute to this project, feel free to fork the repository and make a merge request.

### Linux Compatibility

The compatibility of features depends on whether `amixer` or `wireplumber` is installed on your Linux system. Here is a table that outlines the compatibility:

| Feature                | amixer | wireplumber |
|------------------------|--------|-------------|
| Global volume features | Yes    | Yes |
| Audio status           | No     | Yes |
| List streams           | No     | Yes |
| List sinks             | No     | Yes |
| List sources           | No     | Yes |
| Stream volume features | No     | Yes |
| Sink volume features   | No     | Yes |
| Source volume features | No     | Yes |

Volume features correspond to get/set volume and mute/unmute.

By default `amixer` is used. If `wireplumber` is detected (by checking if the `wpctl` command exists), it will be used instead.


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
volumeControl.setMuted(true).then(() => {
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
  const volume = await volumeControl.getVolume(stream.id);
  console.log(`Stream volume: ${volume}`); // e.g. 50

  // Increase the stream's volume by 10%
  await volumeControl.setVolume(stream.id, volume + 10);
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

export type NodeTypes = 'sink' | 'source' | 'stream';

export type Node = {
  type: NodeTypes;
  id: string;
  name: string;
  isDefault: boolean;
} & VolumeInfo;

export type Status = {
  sinks: Node[];
  sources: Node[];
  streams: Node[];

  defaultSink?: string;
  defaultSource?: string;
}
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.