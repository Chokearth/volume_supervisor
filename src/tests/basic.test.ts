import { volumeControl } from '@/index';

describe('Basic volume test', () => {
  let oldVolume: number = -1;
  let oldMuted: boolean = false;

  beforeAll(async () => {
    oldVolume = await volumeControl.getGlobalVolume();
    oldMuted = await volumeControl.isGlobalMuted();
  });

  beforeEach(async () => {
    await volumeControl.setGlobalVolume(10);
    await volumeControl.setGlobalMuted(true);
  });

  it('should get volume', async () => {
    expect(await volumeControl.getGlobalVolume()).toBe(10);
  });

  it('should set volume', async () => {
    await volumeControl.setGlobalVolume(50);
    expect(await volumeControl.getGlobalVolume()).toBe(50);
  });

  it('should get muted', async () => {
    expect(await volumeControl.isGlobalMuted()).toBe(true);
  });

  it('should set muted', async () => {
    await volumeControl.setGlobalMuted(false);
    expect(await volumeControl.isGlobalMuted()).toBe(false);
    await volumeControl.setGlobalMuted(true);
    expect(await volumeControl.isGlobalMuted()).toBe(true);
  });

  afterAll(async () => {
    await volumeControl.setGlobalVolume(oldVolume);
    await volumeControl.setGlobalMuted(oldMuted);
  });
});