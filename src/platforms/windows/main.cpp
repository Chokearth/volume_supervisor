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
    std::cout << "  setGlobalMute [mute] - Set the global mute, mute must be 1 or 0" << std::endl;
    std::cout << "  getSinks - Get the sinks" << std::endl;
    std::cout << "  getSources - Get the sources" << std::endl;
    std::cout << "  getStreams - Get the streams" << std::endl;
    std::cout << "  getVolumeById [id] - Get the volume of a device by its ID" << std::endl;
    std::cout << "  setVolumeById [id] [volume] - Set the volume of a device by its ID, volume must be between 0 and 100"
              << std::endl;
    std::cout << "  isMutedById [id] - Check if a device by its ID is muted" << std::endl;
    std::cout << "  setMuteById [id] [mute] - Set the mute of a device by its ID, mute must be 1 or 0" << std::endl;
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
        std::cout << "Failed to create device enumerator" << std::endl;
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
        std::cout << "Failed to get default audio endpoint" << std::endl;
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
        std::cout << "Failed to get device ID" << std::endl;
        return nullptr;
    }

    return pwszID;
}

PROPVARIANT getDeviceProperty(IMMDevice *device, const PROPERTYKEY &key) {
    HRESULT hr;

    IPropertyStore *propertyStore = nullptr;
    hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
    if (FAILED(hr)) {
        std::cout << "Failed to open property store" << std::endl;
        return PROPVARIANT();
    }

    PROPVARIANT property;
    PropVariantInit(&property);
    hr = propertyStore->GetValue(key, &property);
    propertyStore->Release();
    if (FAILED(hr)) {
        std::cout << "Failed to get property value" << std::endl;
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
        std::cout << "Failed to enumerate audio endpoints" << std::endl;
        return;
    }

    UINT deviceCount;
    hr = deviceCollection->GetCount(&deviceCount);
    if (FAILED(hr)) {
        std::cout << "Failed to get device count" << std::endl;
        return;
    }

    for (UINT i = 0; i < deviceCount; i++) {
        IMMDevice *device = nullptr;
        hr = deviceCollection->Item(i, &device);
        if (FAILED(hr)) {
            std::cout << "Failed to get device" << std::endl;
            return;
        }

        if (!callback(device)) {
            break;
        }
    }

    deviceCollection->Release();
}

// Device functions
int getVolume(IMMDevice *device) {
    if (device == nullptr) return 0;
    HRESULT hr;

    // Activate an audio interface
    IAudioEndpointVolume *endpointVolume = nullptr;
    hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr,
                          (LPVOID *) &endpointVolume);
    if (FAILED(hr)) {
        std::cout << "Failed to activate audio endpoint" << std::endl;
        return 1;
    }

    // Get the current volume
    float volume;
    hr = endpointVolume->GetMasterVolumeLevelScalar(&volume);
    endpointVolume->Release();
    if (FAILED(hr)) {
        std::cout << "Failed to get master volume level" << std::endl;
        return 1;
    }

    return (int) round(volume * 100);
}

void setVolume(IMMDevice *device, int volume) {
    if (device == nullptr) return;
    HRESULT hr;

    // Activate an audio interface
    IAudioEndpointVolume *endpointVolume = nullptr;
    hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr,
                          (LPVOID *) &endpointVolume);
    if (FAILED(hr)) {
        std::cout << "Failed to activate audio endpoint" << std::endl;
        return;
    }

    // Set the volume
    hr = endpointVolume->SetMasterVolumeLevelScalar((float) volume / 100, nullptr);
    endpointVolume->Release();
    if (FAILED(hr)) {
        std::cout << "Failed to set master volume level" << std::endl;
        return;
    }
}

bool isMuted(IMMDevice *device) {
    if (device == nullptr) return false;
    HRESULT hr;

    // Activate an audio interface
    IAudioEndpointVolume *endpointVolume = nullptr;
    hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr,
                          (LPVOID *) &endpointVolume);
    if (FAILED(hr)) {
        std::cout << "Failed to activate audio endpoint" << std::endl;
        return false;
    }

    // Get the current mute state
    BOOL mute;
    hr = endpointVolume->GetMute(&mute);
    endpointVolume->Release();
    if (FAILED(hr)) {
        std::cout << "Failed to get mute state" << std::endl;
        return false;
    }

    return mute;
}

void setMute(IMMDevice *device, bool mute) {
    if (device == nullptr) return;
    HRESULT hr;

    // Activate an audio interface
    IAudioEndpointVolume *endpointVolume = nullptr;
    hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr,
                          (LPVOID *) &endpointVolume);
    if (FAILED(hr)) {
        std::cout << "Failed to activate audio endpoint" << std::endl;
        return;
    }

    // Set the mute state
    hr = endpointVolume->SetMute(mute, nullptr);
    endpointVolume->Release();
    if (FAILED(hr)) {
        std::cout << "Failed to set mute state" << std::endl;
        return;
    }
}

// Default device functions
int getGlobalVolume() {
    return getVolume(getDefaultDevice());;
}

void setGlobalVolume(int volume) {
    setVolume(getDefaultDevice(), volume);
}

bool isGlobalMuted() {
    return isMuted(getDefaultDevice());
}

void setGlobalMute(bool mute) {
    setMute(getDefaultDevice(), mute);
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
            std::cout << "Failed to get device ID" << std::endl;
            return false;
        }

        VsNode node;
        node.id = pwszID;
        node.name = property.pwszVal;
        node.volume = getVolume(device);
        node.muted = isMuted(device);
        node.isDefault = wcscmp(pwszID, defaultDeviceId) == 0;
        nodes->push_back(node);

        return true;
    };

    forEachDevice(fn, dataFlow);
}

void getStreams(std::vector<VsNode> *nodes) {
    auto fn = [&nodes](IMMDevice *device) -> bool {
        HRESULT hr;

        IAudioSessionManager2 *sessionManager = nullptr;
        hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_INPROC_SERVER, nullptr,
                              (LPVOID *) &sessionManager);
        if (FAILED(hr)) {
            std::cout << "Failed to activate audio session manager" << std::endl;
            return false;
        }

        IAudioSessionEnumerator *sessionEnumerator = nullptr;
        hr = sessionManager->GetSessionEnumerator(&sessionEnumerator);
        sessionManager->Release();
        if (FAILED(hr)) {
            std::cout << "Failed to get session enumerator" << std::endl;
            return false;
        }

        int sessionCount;
        hr = sessionEnumerator->GetCount(&sessionCount);
        if (FAILED(hr)) {
            std::cout << "Failed to get session count" << std::endl;
            return false;
        }

        for (int i = 0; i < sessionCount; i++) {
            IAudioSessionControl *sessionControl = nullptr;
            hr = sessionEnumerator->GetSession(i, &sessionControl);
            if (FAILED(hr)) {
                std::cout << "Failed to get session" << std::endl;
                return false;
            }

            IAudioSessionControl2 *sessionControl2 = nullptr;
            hr = sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void **) &sessionControl2);
            sessionControl->Release();
            if (FAILED(hr)) {
                std::cout << "Failed to get session control" << std::endl;
                return false;
            }

            LPWSTR pwszIDBad = nullptr;
            hr = sessionControl2->GetSessionInstanceIdentifier(&pwszIDBad);
            sessionControl2->Release();
            if (FAILED(hr)) {
                std::cout << "Failed to get session instance identifier" << std::endl;
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
                continue;
            }

            DWORD processId;
            hr = sessionControl2->GetProcessId(&processId);
            if (FAILED(hr)) {
                std::cout << "Failed to get process ID" << std::endl;
                return false;
            }

            LPWSTR displayName = getProcessName(processId);

            float volume = 0;
            BOOL mute = false;

            ISimpleAudioVolume *simpleAudioVolume = nullptr;
            hr = sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void **) &simpleAudioVolume);
            if (FAILED(hr)) {
                std::cout << "Failed to get simple audio volume" << std::endl;
                return false;
            }

            hr = simpleAudioVolume->GetMasterVolume(&volume);
            if (FAILED(hr)) {
                std::cout << "Failed to get master volume" << std::endl;
                return false;
            }
            hr = simpleAudioVolume->GetMute(&mute);
            if (FAILED(hr)) {
                std::cout << "Failed to get mute" << std::endl;
                return false;
            }
            simpleAudioVolume->Release();

            VsNode node;
            node.id = pwszID;
            node.name = displayName;
            node.volume = (int) (volume * 100);
            node.muted = mute;
            node.isDefault = false;
            nodes->push_back(node);
        }

        return true;
    };

    forEachDevice(fn, eRender);
}

int getVolumeById(LPWSTR id) {
    HRESULT hr;

    IMMDeviceEnumerator *deviceEnumerator = getDeviceEnumerator();
    if (deviceEnumerator == nullptr) {
        return 0;
    }

    IMMDevice *device = nullptr;
    hr = deviceEnumerator->GetDevice(id, &device);
    if (FAILED(hr)) {
        std::cout << "Failed to get device" << std::endl;
        return 0;
    }

    return getVolume(device);
}

void setVolumeById(LPWSTR id, int volume) {
    HRESULT hr;

    IMMDeviceEnumerator *deviceEnumerator = getDeviceEnumerator();
    if (deviceEnumerator == nullptr) {
        return;
    }

    IMMDevice *device = nullptr;
    hr = deviceEnumerator->GetDevice(id, &device);
    if (FAILED(hr)) {
        std::cout << "Failed to get device" << std::endl;
        return;
    }

    setVolume(device, volume);
}

bool isMutedById(LPWSTR id) {
    HRESULT hr;

    IMMDeviceEnumerator *deviceEnumerator = getDeviceEnumerator();
    if (deviceEnumerator == nullptr) {
        return false;
    }

    IMMDevice *device = nullptr;
    hr = deviceEnumerator->GetDevice(id, &device);
    if (FAILED(hr)) {
        std::cout << "Failed to get device" << std::endl;
        return false;
    }

    return isMuted(device);
}

void setMuteById(LPWSTR id, bool mute) {
    HRESULT hr;

    IMMDeviceEnumerator *deviceEnumerator = getDeviceEnumerator();
    if (deviceEnumerator == nullptr) {
        return;
    }

    IMMDevice *device = nullptr;
    hr = deviceEnumerator->GetDevice(id, &device);
    if (FAILED(hr)) {
        std::cout << "Failed to get device" << std::endl;
        return;
    }

    setMute(device, mute);
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
    } else if (command == "setGlobalMute") {
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

        setGlobalMute(muteStr == "1");
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
            std::cout << "Volume must be between 0 and 100" << std::endl;
            printUsage(argv);
            uninitialize();
            return 1;
        }

        setVolumeById(id, volume);
    } else if (command == "isMutedById") {
        if (argc < 3) {
            std::cout << "Missing ID argument" << std::endl;
            printUsage(argv);
            uninitialize();
            return 1;
        }

        std::wstring idW = std::wstring(argv[2], argv[2] + strlen(argv[2]));
        LPWSTR_FROM_WSTRING(id, idW);

        std::cout << isMutedById(id) << std::endl;
    } else if (command == "setMuteById") {
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

        setMuteById(id, muteStr == "1");
    } else {
        std::cout << "Unknown command: " << command << std::endl;
        printUsage(argv);
        uninitialize();
        return 1;
    }

    uninitialize();
    return 0;
}
