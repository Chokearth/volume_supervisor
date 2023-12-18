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

IMMDeviceEnumerator *deviceEnumerator = nullptr;
IMMDevice *defaultDevice = nullptr;

// Structures
struct VsNode {
    LPWSTR id;
    LPWSTR name;
    int volume;
    bool mute;
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

void uninitialize() {
    clearGlobal();
    CoUninitialize();
}

void printVsNode(VsNode &node, bool last = true) {
    printf("{\n");
    printf("  \"id\": \"%S\",\n", node.id);
    printf("  \"name\": \"%S\",\n", node.name);
    printf("  \"volume\": %d,\n", node.volume);
    printf("  \"mute\": %s,\n", node.mute ? "true" : "false");
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

    return (int) (volume * 100);
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
        node.mute = isMuted(device);
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

            LPWSTR pwszID = nullptr;
            hr = sessionControl2->GetSessionInstanceIdentifier(&pwszID);
            sessionControl2->Release();
            if (FAILED(hr)) {
                std::cout << "Failed to get session instance identifier" << std::endl;
                return false;
            }

            hr = sessionControl2->IsSystemSoundsSession();
            if (hr == S_OK) {
                continue;
            }

            LPWSTR displayName = nullptr;
            hr = sessionControl->GetDisplayName(&displayName);
            if (FAILED(hr)) {
                std::cout << "Failed to get display name" << std::endl;
                return false;
            }

            float volume = 0;
            BOOL mute = false;

            VsNode node;
            node.id = pwszID;
            node.name = displayName;
            node.volume = (int) (volume * 100);
            node.mute = mute;
            node.isDefault = false;
            nodes->push_back(node);
        }

        return true;
    };

    forEachDevice(fn, eRender);
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
        delete nodes;
    } else if (command == "getSources") {
        std::vector<VsNode> *nodes = new std::vector<VsNode>();
        getVsNodeOfType(nodes, eCapture);
        printVsNodeVector(*nodes);
        delete nodes;
    } else if (command == "getStreams") {
        std::vector<VsNode> *nodes = new std::vector<VsNode>();
        getStreams(nodes);
        printVsNodeVector(*nodes);
        delete nodes;
    } else {
        std::cout << "Unknown command: " << command << std::endl;
        printUsage(argv);
        uninitialize();
        return 1;
    }

    uninitialize();
    return 0;
}
