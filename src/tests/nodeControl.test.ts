import { volumeControl } from '@/index';
import { VsNode } from '@/types';
import * as stream from 'stream';

describe('Volume status test', () => {
  let oldMuted: boolean = false;
  const doTestStatus = volumeControl.getPlatformCompatibility().status;

  beforeAll(async () => {
    oldMuted = await volumeControl.isGlobalMuted();
  });

  beforeEach(async () => {
    await volumeControl.setGlobalMuted(true);
  });

  async function testControlOnNode(node: VsNode) {
    const oldVolume = node.volume;
    const oldMuted = node.muted;
    expect((await volumeControl.getNodeVolumeInfoById(node.id)).volume).toBe(oldVolume);
    expect((await volumeControl.getNodeVolumeInfoById(node.id)).muted).toBe(oldMuted);

    await volumeControl.setNodeVolumeById(node.id, 20 == oldVolume ? 50 : 20);
    expect((await volumeControl.getNodeVolumeInfoById(node.id)).volume).not.toBe(oldVolume);

    await volumeControl.setNodeMutedById(node.id, !oldMuted);
    expect((await volumeControl.getNodeVolumeInfoById(node.id)).muted).not.toBe(oldMuted);

    await volumeControl.setNodeVolumeById(node.id, oldVolume);
    expect((await volumeControl.getNodeVolumeInfoById(node.id)).volume).toBe(oldVolume);

    await volumeControl.setNodeMutedById(node.id, oldMuted);
    expect((await volumeControl.getNodeVolumeInfoById(node.id)).muted).toBe(oldMuted);
  }

  it('should have individual control on stream', async () => {
    if (!doTestStatus || !volumeControl.getPlatformCompatibility().listStreams || !volumeControl.getPlatformCompatibility().setStreamVolume) return;
    const status = await volumeControl.getStatus();
    const streams = status.streams;

    expect(streams.length).toBeGreaterThan(0); // Please play some background audio before running this test

    const stream = streams[0];
    await testControlOnNode(stream);

    if (!volumeControl.getPlatformCompatibility().getStreamDestination) return;

    const oldDestination = stream.destinationId;
    expect(oldDestination).toBeDefined();
    const currentSink = status.sinks.find(sink => sink.id === oldDestination);
    expect(currentSink).toBeDefined();

    if (!volumeControl.getPlatformCompatibility().setStreamDestination) return;

    const sinks = status.sinks;
    const otherSinks = sinks.filter(sink => sink.id !== oldDestination);
    expect(otherSinks.length).toBeGreaterThan(0); // Please have at least two sinks before running this test

    const newDestination = otherSinks[0].id;
    await volumeControl.setStreamDestination(stream.id, newDestination);
    expect((await volumeControl.getStatus()).streams[0].destinationId).toBe(newDestination);

    await volumeControl.setStreamDestination(stream.id, oldDestination);
    expect((await volumeControl.getStatus()).streams[0].destinationId).toBe(oldDestination);
  });

  it('should have individual control on sink', async () => {
    if (!doTestStatus || !volumeControl.getPlatformCompatibility().listSinks || !volumeControl.getPlatformCompatibility().setSinkVolume) return;
    const status = await volumeControl.getStatus();
    const sinks = status.sinks;

    expect(sinks.length).toBeGreaterThan(0); // Please have a sink before running this test

    await testControlOnNode(sinks[0]);
  });

  it('should have individual control on source', async () => {
    if (!doTestStatus || !volumeControl.getPlatformCompatibility().listSources || !volumeControl.getPlatformCompatibility().setSourceVolume) return;
    const status = await volumeControl.getStatus();
    const sources = status.sources;

    expect(sources.length).toBeGreaterThan(0); // Please have a source before running this test

    await testControlOnNode(sources[0]);
  });

  afterAll(async () => {
    await volumeControl.setGlobalMuted(oldMuted);
  });
});