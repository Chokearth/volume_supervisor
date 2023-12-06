export type GetGlobalVolume = () => Promise<number>;
export type SetGlobalVolume = (volume: number) => Promise<void>;
export type IsMuted = () => Promise<boolean>;
export type SetMuted = (muted: boolean) => Promise<void>;

export interface PlatformImplementation {
  /**
   * Get the global volume of the system.
   * @returns {Promise<number>} The global volume of the system, from 0 to 100.
   */
  getGlobalVolume: GetGlobalVolume;
  /**
   * Set the global volume of the system.
   * @param {number} volume The volume to set, from 0 to 100.
   * @returns {Promise<void>} A promise that resolves when the volume has been set.
   */
  setGlobalVolume: SetGlobalVolume;
  /**
   * Get the mute state of the system.
   * @returns {Promise<boolean>} A promise that resolves to true if the system is muted, false otherwise.
   */
  isMuted: IsMuted;
  /**
   * Set the mute state of the system.
   * @param {boolean} muted The mute state to set.
   * @returns {Promise<void>} A promise that resolves when the mute state has been set.
   */
  setMuted: SetMuted;
}