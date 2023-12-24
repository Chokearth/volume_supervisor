#include <iostream>
#include <string>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <initguid.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <vector>
#include <functional>
#include <audiopolicy.h>
#include <winver.h>
#include <math.h>

IMMDeviceEnumerator *deviceEnumerator = nullptr;
IMMDevice *defaultDevice = nullptr;

#define LPWSTR_FROM_WSTRING(lp, ws) LPWSTR lp = new WCHAR[ws.length() + 1]; std::copy(ws.begin(), ws.end(), lp); lp[ws.length()] = 0

// Structures
struct VsNode {
    LPWSTR id;
    LPWSTR name;
    int volume;
    bool muted;
    bool isDefault;
};

// Utils
void printUsage(char *argv[]) {
    std::cout << "Usage: " << argv[0] << " [command] [args...]\n" << std::endl;

    std::cout << "Commands:" << std::endl;
    std::cout << "  getGlobalVolume - Get the global volume" << std::endl;
    std::cout << "  setGlobalVolume [volume] - Set the global volume, volume must be between 0 and 100" << std::endl;
    std::cout << "  isGlobalMuted - Check if the global volume is muted" << std::endl;
    std::cout << "  setGlobalMuted [mute] - Set the global mute, mute must be 1 or 0" << std::endl;
    std::cout << "  getSinks - Get the sinks" << std::endl;
    std::cout << "  getSources - Get the sources" << std::endl;
    std::cout << "  getStreams - Get the streams" << std::endl;
    std::cout << "  getVolumeById [id] - Get the volume of a device by its ID" << std::endl;
    std::cout
            << "  setVolumeById [id] [volume] - Set the volume of a device by its ID, volume must be between 0 and 100"
            << std::endl;
    std::cout << "  isMutedById [id] - Check if a device by its ID is muted" << std::endl;
    std::cout << "  setMutedById [id] [mute] - Set the mute of a device by its ID, mute must be 1 or 0" << std::endl;
}

void clearGlobal() {
    if (deviceEnumerator != nullptr) {
        deviceEnumerator->Release();
        deviceEnumerator = nullptr;
    }

    if (defaultDevice != nullptr) {
        defaultDevice->Release();
        defaultDevice = nullptr;
    }
}

void clearVsNode(VsNode &node) { // TODO there is probably memory leaks
//    delete node.id;
//    delete node.name;
}

void clearVsNode(std::vector<VsNode> &nodes) {
    UINT count = nodes.size();

    for (UINT i = 0; i < count; i++) {
        clearVsNode(nodes[i]);
    }

    delete &nodes;
}

void uninitialize() {
    clearGlobal();
    CoUninitialize();
}

void printVsNode(VsNode &node, bool last = true) {
    printf("{\n");
    printf("  \"id\": \"%S\",\n", node.id);
    printf("  \"name\": \"%S\",\n", node.name);
    printf("  \"volume\": %d,\n", node.volume);
    printf("  \"muted\": %s,\n", node.muted ? "true" : "false");
    printf("  \"isDefault\": %s\n", node.isDefault ? "true" : "false");
    printf("}%s\n", last ? "" : ",");
}

void printVsNodeVector(std::vector<VsNode> &nodes) {
    UINT count = nodes.size();

    std::cout << "[" << std::endl;
    for (UINT i = 0; i < count; i++) {
        printVsNode(nodes[i], i == count - 1);
    }
    std::cout << "]" << std::endl;
}

ISimpleAudioVolume *toSAV(IAudioSessionControl2 *sessionControl) {
    HRESULT hr;

    ISimpleAudioVolume *simpleAudioVolume = nullptr;
    hr = sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void **) &simpleAudioVolume);
    if (FAILED(hr)) {
        std::cerr << "Failed to get simple audio volume from session control" << std::endl;
        return nullptr;
    }


    return simpleAudioVolume;
}

IAudioEndpointVolume *toAEV(IMMDevice *device) {
    HRESULT hr;

    IAudioEndpointVolume *audioEndpointVolume = nullptr;
    hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr,
                          (LPVOID *) &audioEndpointVolume);
    if (FAILED(hr)) {
        std::cerr << "Failed to activate audio endpoint volume" << std::endl;
        return nullptr;
    }

    return audioEndpointVolume;
}

// Get functions
IMMDeviceEnumerator *getDeviceEnumerator() {
    HRESULT hr;

    if (deviceEnumerator != nullptr) {
        return deviceEnumerator;
    }

    // Get the speakers device
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator),
                          (LPVOID *) &deviceEnumerator);
    if (FAILED(hr)) {
        std::cerr << "Failed to create device enumerator" << std::endl;
        return nullptr;
    }

    return deviceEnumerator;
}

IMMDevice *getDefaultDevice(EDataFlow dataFlow = eRender) {
    HRESULT hr;

    if (defaultDevice != nullptr) {
        return defaultDevice;
    }

    IMMDeviceEnumerator *deviceEnumerator = getDeviceEnumerator();
    if (deviceEnumerator == nullptr) {
        return nullptr;
    }

    // Get default audio endpoint that the system is currently using
    hr = deviceEnumerator->GetDefaultAudioEndpoint(dataFlow, eMultimedia, &defaultDevice);
    if (FAILED(hr)) {
        std::cerr << "Failed to get default audio endpoint" << std::endl;
        return nullptr;
    }

    return defaultDevice;
}

LPWSTR getDefaultDeviceId(EDataFlow dataFlow = eRender) {
    HRESULT hr;

    IMMDevice *defaultDevice = getDefaultDevice(dataFlow);
    if (defaultDevice == nullptr) {
        return nullptr;
    }

    LPWSTR pwszID = nullptr;
    hr = defaultDevice->GetId(&pwszID);
    if (FAILED(hr)) {
        std::cerr << "Failed to get device ID" << std::endl;
        return nullptr;
    }

    return pwszID;
}

PROPVARIANT getDeviceProperty(IMMDevice *device, const PROPERTYKEY &key) {
    HRESULT hr;

    IPropertyStore *propertyStore = nullptr;
    hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
    if (FAILED(hr)) {
        std::cerr << "Failed to open property store" << std::endl;
        return PROPVARIANT();
    }

    PROPVARIANT property;
    PropVariantInit(&property);
    hr = propertyStore->GetValue(key, &property);
    propertyStore->Release();
    if (FAILED(hr)) {
        std::cerr << "Failed to get property value" << std::endl;
        return PROPVARIANT();
    }

    return property;
}

LPWSTR getProcessName(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess) {
        WCHAR processPath[MAX_PATH];
        DWORD pathSize = sizeof(processPath) / sizeof(processPath[0]);

        if (QueryFullProcessImageNameW(hProcess, 0, processPath, &pathSize)) {
            // Extract the executable name from the full path
            std::wstring fullPath(processPath);
            size_t lastSlashPos = fullPath.find_last_of(L'\\');

            if (lastSlashPos != std::wstring::npos && lastSlashPos + 1 < fullPath.length()) {
                std::wstring executableNameW = fullPath.substr(lastSlashPos + 1);
                LPWSTR_FROM_WSTRING(executableName, executableNameW);

                // Get the product name from the executable
                DWORD versionHandle = 0;
                DWORD versionSize = GetFileVersionInfoSizeW(processPath, &versionHandle);
                if (versionSize > 0) {
                    std::vector<BYTE> versionData(versionSize);
                    if (GetFileVersionInfoW(processPath, versionHandle, versionSize, versionData.data())) {
                        LPWSTR productName = nullptr;
                        UINT productNameSize = 0;

                        if (VerQueryValueW(versionData.data(), L"\\StringFileInfo\\040904b0\\ProductName",
                                           (LPVOID *) &productName, &productNameSize)) {
                            if (productNameSize > 0) {
                                return productName;
                            }
                        }
                    }
                }

                return executableName;
            }
        }

        CloseHandle(hProcess);
    }

    LPWSTR unknown = new WCHAR[8];
    wcscpy_s(unknown, 8, L"Unknown");
    return unknown;
}

// Mapper
void forEachDevice(const std::function<bool(IMMDevice *)> &callback, EDataFlow dataFlow = eRender) {
    HRESULT hr;

    IMMDeviceEnumerator *deviceEnumerator = getDeviceEnumerator();
    if (deviceEnumerator == nullptr) {
        return;
    }

    // Iterate through all devices
    IMMDeviceCollection *deviceCollection = nullptr;
    hr = deviceEnumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, &deviceCollection);
    if (FAILED(hr)) {
        std::cerr << "Failed to enumerate audio endpoints" << std::endl;
        return;
    }

    UINT deviceCount;
    hr = deviceCollection->GetCount(&deviceCount);
    if (FAILED(hr)) {
        std::cerr << "Failed to get device count" << std::endl;
        return;
    }

    for (UINT i = 0; i < deviceCount; i++) {
        IMMDevice *device = nullptr;
        hr = deviceCollection->Item(i, &device);
        if (FAILED(hr)) {
            std::cerr << "Failed to get device" << std::endl;
            return;
        }

        if (!callback(device)) {
            break;
        }
    }

    deviceCollection->Release();
}

void forEachSession(const std::function<bool(IAudioSessionControl2 *)> &callback) {
    auto fn = [&callback](IMMDevice *device) -> bool {
        HRESULT hr;

        IAudioSessionManager2 *sessionManager = nullptr;
        hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_INPROC_SERVER, nullptr,
                              (LPVOID *) &sessionManager);
        if (FAILED(hr)) {
            std::cerr << "Failed to activate audio session manager" << std::endl;
            return false;
        }

        IAudioSessionEnumerator *sessionEnumerator = nullptr;
        hr = sessionManager->GetSessionEnumerator(&sessionEnumerator);
        sessionManager->Release();
        if (FAILED(hr)) {
            std::cerr << "Failed to get session enumerator" << std::endl;
            return false;
        }

        int sessionCount;
        hr = sessionEnumerator->GetCount(&sessionCount);
        if (FAILED(hr)) {
            std::cerr << "Failed to get session count" << std::endl;
            return false;
        }

        for (int i = 0; i < sessionCount; i++) {
            IAudioSessionControl *sessionControl = nullptr;
            hr = sessionEnumerator->GetSession(i, &sessionControl);
            if (FAILED(hr)) {
                std::cerr << "Failed to get session" << std::endl;
                return false;
            }

            IAudioSessionControl2 *sessionControl2 = nullptr;
            hr = sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void **) &sessionControl2);
            sessionControl->Release();
            if (FAILED(hr)) {
                std::cerr << "Failed to get session control" << std::endl;
                return false;
            }

            if (!callback(sessionControl2)) {
                return false;
            }
        }

        return true;
    };

    forEachDevice(fn, eRender);
}

// Complex getter functions
IAudioSessionControl2 *getSessionById(LPWSTR id) {
    IAudioSessionControl2 *result = nullptr;

    auto fn = [&result, id](IAudioSessionControl2 *sessionControl2) -> bool {
        HRESULT hr;

        LPWSTR pwszIDBad = nullptr;
        hr = sessionControl2->GetSessionInstanceIdentifier(&pwszIDBad);
        sessionControl2->Release();
        if (FAILED(hr)) {
            std::cerr << "Failed to get session instance identifier" << std::endl;
            return false;
        }

        std::wstring idW(pwszIDBad);
        size_t pos = 0;
        while ((pos = idW.find(L"\\", pos)) != std::wstring::npos) {
            idW.replace(pos, 1, L"\\\\");
            pos += 2;
        }
        LPWSTR_FROM_WSTRING(pwszID, idW);

        if (wcscmp(pwszID, id) == 0) {
            result = sessionControl2;
            return false;
        }

        return true;
    };

    forEachSession(fn);

    return result;
}

// AudioEndpointVolume functions
int getVolume(IAudioEndpointVolume *audioEndpointVolume) {
    if (audioEndpointVolume == nullptr) return 0;
    HRESULT hr;

    // Get the current volume
    float volume;
    hr = audioEndpointVolume->GetMasterVolumeLevelScalar(&volume);
    if (FAILED(hr)) {
        std::cerr << "Failed to get master volume level" << std::endl;
        return 0;
    }

    return (int) round(volume * 100);
}

void setVolume(IAudioEndpointVolume *audioEndpointVolume, int volume) {
    if (audioEndpointVolume == nullptr) return;
    HRESULT hr;

    // Set the volume
    hr = audioEndpointVolume->SetMasterVolumeLevelScalar((float) volume / 100, nullptr);
    if (FAILED(hr)) {
        std::cerr << "Failed to set master volume level" << std::endl;
        return;
    }
}

bool isMuted(IAudioEndpointVolume *audioEndpointVolume) {
    if (audioEndpointVolume == nullptr) return false;
    HRESULT hr;

    // Get the current mute state
    BOOL mute;
    hr = audioEndpointVolume->GetMute(&mute);
    if (FAILED(hr)) {
        std::cerr << "Failed to get mute state" << std::endl;
        return false;
    }

    return mute;
}

void setMuted(IAudioEndpointVolume *audioEndpointVolume, bool mute) {
    if (audioEndpointVolume == nullptr) return;
    HRESULT hr;

    // Set the mute state
    hr = audioEndpointVolume->SetMute(mute, nullptr);
    if (FAILED(hr)) {
        std::cerr << "Failed to set mute state" << std::endl;
        return;
    }
}

IAudioEndpointVolume *getAEVById(LPWSTR id) {
    IMMDeviceEnumerator *deviceEnumerator = getDeviceEnumerator();
    if (deviceEnumerator == nullptr) {
        return nullptr;
    }

    IMMDevice *device = nullptr;
    HRESULT hr = deviceEnumerator->GetDevice(id, &device);
    if (FAILED(hr)) {
        return nullptr;
    }

    IAudioEndpointVolume *audioEndpointVolume = toAEV(device);
    device->Release();

    return audioEndpointVolume;
}

// SimpleAudioVolume functions
int getVolume(ISimpleAudioVolume *simpleAudioVolume) {
    if (simpleAudioVolume == nullptr) return 0;
    HRESULT hr;

    // Get the current volume
    float volume;
    hr = simpleAudioVolume->GetMasterVolume(&volume);
    if (FAILED(hr)) {
        std::cerr << "Failed to get master volume level" << std::endl;
        return 0;
    }

    return (int) round(volume * 100);
}

void setVolume(ISimpleAudioVolume *simpleAudioVolume, int volume) {
    if (simpleAudioVolume == nullptr) return;
    HRESULT hr;

    // Set the volume
    hr = simpleAudioVolume->SetMasterVolume((float) volume / 100, nullptr);
    if (FAILED(hr)) {
        std::cerr << "Failed to set master volume level" << std::endl;
        return;
    }
}

bool isMuted(ISimpleAudioVolume *simpleAudioVolume) {
    if (simpleAudioVolume == nullptr) return false;
    HRESULT hr;

    // Get the current mute state
    BOOL mute;
    hr = simpleAudioVolume->GetMute(&mute);
    if (FAILED(hr)) {
        std::cerr << "Failed to get mute state" << std::endl;
        return false;
    }

    return mute;
}

void setMuted(ISimpleAudioVolume *simpleAudioVolume, bool mute) {
    if (simpleAudioVolume == nullptr) return;
    HRESULT hr;

    // Set the mute state
    hr = simpleAudioVolume->SetMute(mute, nullptr);
    if (FAILED(hr)) {
        std::cerr << "Failed to set mute state" << std::endl;
        return;
    }
}

ISimpleAudioVolume *getSAVById(LPWSTR id) {
    IAudioSessionControl2 *sessionControl2 = getSessionById(id);
    if (sessionControl2 == nullptr) {
        return nullptr;
    }

    ISimpleAudioVolume *simpleAudioVolume = toSAV(sessionControl2);
    sessionControl2->Release();

    return simpleAudioVolume;
}

// Default device functions
int getGlobalVolume() {
    return getVolume(toAEV(getDefaultDevice()));
}

void setGlobalVolume(int volume) {
    setVolume(toAEV(getDefaultDevice()), volume);
}

bool isGlobalMuted() {
    return isMuted(toAEV(getDefaultDevice()));
}

void setGlobalMuted(bool mute) {
    setMuted(toAEV(getDefaultDevice()), mute);
}

// Get VsNode functions
void getVsNodeOfType(std::vector<VsNode> *nodes, EDataFlow dataFlow = eRender) {
    LPWSTR defaultDeviceId = getDefaultDeviceId(dataFlow);

    auto fn = [&nodes, defaultDeviceId](IMMDevice *device) -> bool {
        HRESULT hr;
        PROPVARIANT property = getDeviceProperty(device, PKEY_Device_FriendlyName);

        LPWSTR pwszID = nullptr;
        hr = device->GetId(&pwszID);
        if (FAILED(hr)) {
            std::cerr << "Failed to get device ID" << std::endl;
            return false;
        }

        IAudioEndpointVolume *audioEndpointVolume = toAEV(device);

        VsNode node;
        node.id = pwszID;
        node.name = property.pwszVal;
        node.volume = getVolume(audioEndpointVolume);
        node.muted = isMuted(audioEndpointVolume);
        node.isDefault = wcscmp(pwszID, defaultDeviceId) == 0;
        nodes->push_back(node);
        audioEndpointVolume->Release();

        return true;
    };

    forEachDevice(fn, dataFlow);
}

void getStreams(std::vector<VsNode> *nodes) {
    auto fn = [&nodes](IAudioSessionControl2 *sessionControl2) -> bool {
        HRESULT hr;

        LPWSTR pwszIDBad = nullptr;
        hr = sessionControl2->GetSessionInstanceIdentifier(&pwszIDBad);
        sessionControl2->Release();
        if (FAILED(hr)) {
            std::cerr << "Failed to get session instance identifier" << std::endl;
            return false;
        }

        std::wstring id(pwszIDBad);
        size_t pos = 0;
        while ((pos = id.find(L"\\", pos)) != std::wstring::npos) {
            id.replace(pos, 1, L"\\\\");
            pos += 2;
        }
        LPWSTR_FROM_WSTRING(pwszID, id);


        hr = sessionControl2->IsSystemSoundsSession();
        if (hr == S_OK) {
            return true;
        }

        DWORD processId;
        hr = sessionControl2->GetProcessId(&processId);
        if (FAILED(hr)) {
            std::cerr << "Failed to get process ID" << std::endl;
            return false;
        }

        LPWSTR displayName = getProcessName(processId);
        ISimpleAudioVolume *simpleAudioVolume = toSAV(sessionControl2);

        VsNode node;
        node.id = pwszID;
        node.name = displayName;
        node.volume = getVolume(simpleAudioVolume);
        node.muted = isMuted(simpleAudioVolume);
        node.isDefault = false;
        nodes->push_back(node);
        simpleAudioVolume->Release();

        return true;
    };

    forEachSession(fn);
}

int getVolumeById(LPWSTR id) {
    IAudioEndpointVolume *audioEndpointVolume = getAEVById(id);
    if (audioEndpointVolume != nullptr) {
        int volume = getVolume(audioEndpointVolume);
        audioEndpointVolume->Release();
        return volume;
    }

    ISimpleAudioVolume *simpleAudioVolume = getSAVById(id);
    if (simpleAudioVolume != nullptr) {
        int volume = getVolume(simpleAudioVolume);
        simpleAudioVolume->Release();
        return volume;
    }

    std::cerr << "Failed to get volume by ID" << std::endl;
    return 0;
}

void setVolumeById(LPWSTR id, int volume) {
    IAudioEndpointVolume *audioEndpointVolume = getAEVById(id);
    if (audioEndpointVolume != nullptr) {
        setVolume(audioEndpointVolume, volume);
        audioEndpointVolume->Release();
        return;
    }

    ISimpleAudioVolume *simpleAudioVolume = getSAVById(id);
    if (simpleAudioVolume != nullptr) {
        setVolume(simpleAudioVolume, volume);
        simpleAudioVolume->Release();
        return;
    }

    std::cerr << "Failed to set volume by ID" << std::endl;
}

bool isMutedById(LPWSTR id) {
    IAudioEndpointVolume *audioEndpointVolume = getAEVById(id);
    if (audioEndpointVolume != nullptr) {
        bool muted = isMuted(audioEndpointVolume);
        audioEndpointVolume->Release();
        return muted;
    }

    ISimpleAudioVolume *simpleAudioVolume = getSAVById(id);
    if (simpleAudioVolume != nullptr) {
        bool muted = isMuted(simpleAudioVolume);
        simpleAudioVolume->Release();
        return muted;
    }

    std::cerr << "Failed to get mute state by ID" << std::endl;
    return false;
}

void setMutedById(LPWSTR id, bool mute) {
    IAudioEndpointVolume *audioEndpointVolume = getAEVById(id);
    if (audioEndpointVolume != nullptr) {
        setMuted(audioEndpointVolume, mute);
        audioEndpointVolume->Release();
        return;
    }

    ISimpleAudioVolume *simpleAudioVolume = getSAVById(id);
    if (simpleAudioVolume != nullptr) {
        setMuted(simpleAudioVolume, mute);
        simpleAudioVolume->Release();
        return;
    }

    std::cerr << "Failed to set mute state by ID" << std::endl;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printUsage(argv);
        return 1;
    }

    CoInitialize(nullptr);

    std::string command = argv[1];

    if (command == "getGlobalVolume") {
        std::cout << getGlobalVolume() << std::endl;
    } else if (command == "setGlobalVolume") {
        if (argc < 3) {
            std::cout << "Missing volume argument" << std::endl;
            printUsage(argv);
            uninitialize();
            return 1;
        }

        int volume = std::stoi(argv[2]);
        if (volume < 0 || volume > 100) {
            std::cout << "Volume must be between 0 and 100" << std::endl;
            printUsage(argv);
            uninitialize();
            return 1;
        }

        setGlobalVolume(volume);
    } else if (command == "isGlobalMuted") {
        std::cout << isGlobalMuted() << std::endl;
    } else if (command == "setGlobalMuted") {
        if (argc < 3) {
            std::cout << "Missing mute argument" << std::endl;
            printUsage(argv);
            uninitialize();
            return 1;
        }

        std::string muteStr = argv[2];
        if (muteStr != "1" && muteStr != "0") {
            std::cout << "Mute must be 1 or 0" << std::endl;
            printUsage(argv);
            uninitialize();
            return 1;
        }

        setGlobalMuted(muteStr == "1");
    } else if (command == "getSinks") {
        std::vector<VsNode> *nodes = new std::vector<VsNode>();
        getVsNodeOfType(nodes, eRender);
        printVsNodeVector(*nodes);
        clearVsNode(*nodes);
    } else if (command == "getSources") {
        std::vector<VsNode> *nodes = new std::vector<VsNode>();
        getVsNodeOfType(nodes, eCapture);
        printVsNodeVector(*nodes);
        clearVsNode(*nodes);
    } else if (command == "getStreams") {
        std::vector<VsNode> *nodes = new std::vector<VsNode>();
        getStreams(nodes);
        printVsNodeVector(*nodes);
        clearVsNode(*nodes);
    } else if (command == "getVolumeById") {
        if (argc < 3) {
            std::cout << "Missing ID argument" << std::endl;
            printUsage(argv);
            uninitialize();
            return 1;
        }

        std::wstring idW = std::wstring(argv[2], argv[2] + strlen(argv[2]));
        LPWSTR_FROM_WSTRING(id, idW);

        std::cout << getVolumeById(id) << std::endl;
    } else if (command == "setVolumeById") {
        if (argc < 4) {
            std::cout << "Missing ID and volume arguments" << std::endl;
            printUsage(argv);
            uninitialize();
            return 1;
        }

        std::wstring idW = std::wstring(argv[2], argv[2] + strlen(argv[2]));
        LPWSTR_FROM_WSTRING(id, idW);

        int volume = std::stoi(argv[3]);
        if (volume < 0 || volume > 100) {
            std::cerr << "Volume must be between 0 and 100" << std::endl;
            printUsage(argv);
            uninitialize();
            return 1;
        }

        setVolumeById(id, volume);
    } else if (command == "isMutedById") {
        if (argc < 3) {
            std::cerr << "Missing ID argument" << std::endl;
            printUsage(argv);
            uninitialize();
            return 1;
        }

        std::wstring idW = std::wstring(argv[2], argv[2] + strlen(argv[2]));
        LPWSTR_FROM_WSTRING(id, idW);

        std::cout << isMutedById(id) << std::endl;
    } else if (command == "setMutedById") {
        if (argc < 4) {
            std::cout << "Missing ID and mute arguments" << std::endl;
            printUsage(argv);
            uninitialize();
            return 1;
        }

        std::wstring idW = std::wstring(argv[2], argv[2] + strlen(argv[2]));
        LPWSTR_FROM_WSTRING(id, idW);

        std::string muteStr = argv[3];
        if (muteStr != "1" && muteStr != "0") {
            std::cout << "Mute must be 1 or 0" << std::endl;
            printUsage(argv);
            uninitialize();
            return 1;
        }

        setMutedById(id, muteStr == "1");
    } else {
        std::cout << "Unknown command: " << command << std::endl;
        printUsage(argv);
        uninitialize();
        return 1;
    }

    uninitialize();
    return 0;
}
