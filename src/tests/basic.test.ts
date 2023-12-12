import { volumeControl } from '@/index';

describe('Basic volume test', () => {
  let oldVolume: number = -1;
  let oldMuted: boolean = false;

  beforeAll(async () => {
    oldVolume = await volumeControl.getGlobalVolume();
    oldMuted = await volumeControl.isMuted();
  });

  beforeEach(async () => {
    await volumeControl.setGlobalVolume(0);
    await volumeControl.setMuted(false);
  });

  it('should get volume', async () => {
    expect(await volumeControl.getGlobalVolume()).toBe(0);
  });

  it('should set volume', async () => {
    await volumeControl.setGlobalVolume(50);
    expect(await volumeControl.getGlobalVolume()).toBe(50);
  });

  it('should get muted', async () => {
    expect(await volumeControl.isMuted()).toBe(false);
  });

  it('should set muted', async () => {
    await volumeControl.setMuted(true);
    expect(await volumeControl.isMuted()).toBe(true);
    await volumeControl.setMuted(false);
    expect(await volumeControl.isMuted()).toBe(false);
  });

  afterAll(async () => {
    await volumeControl.setGlobalVolume(oldVolume);
    await volumeControl.setMuted(oldMuted);
  });
});