import { volumeControl } from '@/index';

describe('Volume status test', () => {
  let oldVolume: number = -1;
  let oldMuted: boolean = false;
  const doTest = volumeControl.getPlatformCompatibility().status;

  beforeAll(async () => {
    if (!doTest) return;
    oldVolume = await volumeControl.getGlobalVolume();
    oldMuted = await volumeControl.isMuted();
  });

  beforeEach(async () => {
    if (!doTest) return;
    await volumeControl.setGlobalVolume(0);
    await volumeControl.setMuted(false);
  });

  it('should get default sink status', async () => {
    if (!doTest) return;
    const statusBefore = await volumeControl.getStatus();
    const defaultSink = await statusBefore.sinks.find(sink => sink.isDefault);
    expect(defaultSink).toBeDefined();
    expect(defaultSink?.volume).toBe(0);
    expect(defaultSink?.muted).toBe(false);

    await volumeControl.setGlobalVolume(77);
    await volumeControl.setMuted(true);
    const statusAfter = await volumeControl.getStatus();
    const defaultSinkAfter = await statusAfter.sinks.find(sink => sink.isDefault);
    expect(defaultSinkAfter).toBeDefined();
    expect(defaultSinkAfter?.volume).toBe(77);
  });

  it('should get default sink and source', async () => {
    if (!doTest) return;
    const status = await volumeControl.getStatus();
    const defaultSink = await status.sinks.find(sink => sink.isDefault);
    const defaultSource = await status.sources.find(source => source.isDefault);

    expect(defaultSink.id).toBe(status.defaultSink);
    expect(defaultSource.id).toBe(status.defaultSource);
  });

  afterAll(async () => {
    if (!doTest) return;
    await volumeControl.setGlobalVolume(oldVolume);
    await volumeControl.setMuted(oldMuted);
  });
});