export type GetPlatformCompatibility = () => PlatformCompatibility;
export type GetGlobalVolume = () => Promise<number>;
export type SetGlobalVolume = (volume: number) => Promise<void>;
export type IsGlobalMuted = () => Promise<boolean>;
export type SetGlobalMuted = (muted: boolean) => Promise<void>;
export type GetStatus = () => Promise<Status>;
export type GetNodeVolumeInfoById = (id: string) => Promise<VolumeInfo>;
export type SetNodeVolumeById = (id: string, volume: number) => Promise<void>;
export type SetNodeMutedById = (id: string, muted: boolean) => Promise<void>;

export interface PlatformImplementation {
  /**
   * Get the compatibility of the current platform.
   * @returns {PlatformCompatibility} The compatibility of the platform.
   */
  getPlatformCompatibility: GetPlatformCompatibility;
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
  isGlobalMuted: IsGlobalMuted;
  /**
   * Set the mute state of the system.
   * @param {boolean} muted The mute state to set.
   * @returns {Promise<void>} A promise that resolves when the mute state has been set.
   */
  setGlobalMuted: SetGlobalMuted;
  /**
   * Get the status of the system.
   * @returns {Promise<Status>} A promise that resolves to the status of the system.
   */
  getStatus: GetStatus;
  /**
   * Get the volume info of a node.
   * @param {string} id The id of the node.
   * @returns {Promise<VolumeInfo>} A promise that resolves to the volume info of the node.
   */
  getNodeVolumeInfoById: GetNodeVolumeInfoById;
  /**
   * Set the volume of a node.
   * @param {string} id The id of the node.
   * @param {number} volume The volume to set, from 0 to 100.
   * @returns {Promise<void>} A promise that resolves when the volume has been set.
   */
  setNodeVolumeById: SetNodeVolumeById;
  /**
   * Set the mute state of a node.
   * @param {string} id The id of the node.
   * @param {boolean} muted The mute state to set.
   * @returns {Promise<void>} A promise that resolves when the mute state has been set.
   */
  setNodeMutedById: SetNodeMutedById;
}

export type PlatformCompatibility = {
  status: boolean;
  listStreams: boolean;
  listSinks: boolean;
  listSources: boolean;
  setStreamVolume: boolean;
  setSinkVolume: boolean;
  setSourceVolume: boolean;
}

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