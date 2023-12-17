#include <iostream>
#include <string>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <initguid.h>
#include <Functiondiscoverykeys_devpkey.h>

void printUsage(char *argv[]) {
    std::cout << "Usage: " << argv[0] << " [command] [args...]\n" << std::endl;

    std::cout << "Commands:" << std::endl;
    std::cout << "  getGlobalVolume - Get the global volume" << std::endl;
    std::cout << "  setGlobalVolume [volume] - Set the global volume, volume must be between 0 and 100" << std::endl;
    std::cout << "  isGlobalMuted - Check if the global volume is muted" << std::endl;
    std::cout << "  setGlobalMute [mute] - Set the global mute, mute must be 1 or 0" << std::endl;
    std::cout << "  getSinks - Get the sinks" << std::endl;
}

IMMDeviceEnumerator *getDeviceEnumerator() {
    HRESULT hr;

    // Get the speakers device
    IMMDeviceEnumerator *deviceEnumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator),
                          (LPVOID *) &deviceEnumerator);
    if (FAILED(hr)) {
        std::cout << "Failed to create device enumerator" << std::endl;
        return nullptr;
    }

    return deviceEnumerator;
}

IMMDevice *getDefaultDevice() {
    HRESULT hr;

    IMMDeviceEnumerator *deviceEnumerator = getDeviceEnumerator();
    if (deviceEnumerator == nullptr) {
        return nullptr;
    }

    // Get default audio endpoint that the system is currently using
    IMMDevice *defaultDevice = nullptr;
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &defaultDevice);
    deviceEnumerator->Release();
    if (FAILED(hr)) {
        std::cout << "Failed to get default audio endpoint" << std::endl;
        return nullptr;
    }

    return defaultDevice;
}

IAudioEndpointVolume *getDefaultEndpointVolume() {
    HRESULT hr;

    IMMDevice *defaultDevice = getDefaultDevice();

    // Activate an audio interface
    IAudioEndpointVolume *endpointVolume = nullptr;
    hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr,
                                 (LPVOID *) &endpointVolume);
    defaultDevice->Release();
    if (FAILED(hr)) {
        std::cout << "Failed to activate audio endpoint" << std::endl;
        return nullptr;
    }

    return endpointVolume;
}

LPWSTR getDefaultDeviceId() {
    HRESULT hr;

    IMMDevice *defaultDevice = getDefaultDevice();
    if (defaultDevice == nullptr) {
        return nullptr;
    }

    LPWSTR pwszID = nullptr;
    hr = defaultDevice->GetId(&pwszID);
    defaultDevice->Release();
    if (FAILED(hr)) {
        std::cout << "Failed to get device ID" << std::endl;
        return nullptr;
    }

    return pwszID;
}

int getVolume(IMMDevice *device) {
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

int getGlobalVolume() {
    IMMDevice *defaultDevice = getDefaultDevice();
    if (defaultDevice == nullptr) {
        return 0;
    }

    int volume = getVolume(defaultDevice);
    defaultDevice->Release();
    return volume;
}

void setGlobalVolume(int volume) {
    IMMDevice *defaultDevice = getDefaultDevice();
    if (defaultDevice == nullptr) {
        return;
    }

    setVolume(defaultDevice, volume);
    defaultDevice->Release();
}

bool isGlobalMuted() {
    IMMDevice *defaultDevice = getDefaultDevice();
    if (defaultDevice == nullptr) {
        return false;
    }

    bool mute = isMuted(defaultDevice);
    defaultDevice->Release();
    return mute;
}

void setGlobalMute(bool mute) {
    IMMDevice *defaultDevice = getDefaultDevice();
    if (defaultDevice == nullptr) {
        return;
    }

    setMute(defaultDevice, mute);
    defaultDevice->Release();
}

struct VsNode {
    LPWSTR id;
    LPWSTR name;
    int volume;
    bool mute;
    bool isDefault;
};

UINT getSinks(VsNode **nodes) {
    HRESULT hr;
    LPWSTR defaultDeviceId = getDefaultDeviceId();

    IMMDeviceEnumerator *deviceEnumerator = getDeviceEnumerator();
    if (deviceEnumerator == nullptr) {
        return 0;
    }
    std::cout << "Got device enumerator" << std::endl;

    // Iterate through all devices
    IMMDeviceCollection *deviceCollection = nullptr;
    hr = deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
    deviceEnumerator->Release();
    if (FAILED(hr)) {
        std::cout << "Failed to enumerate audio endpoints" << std::endl;
        return 0;
    }
    std::cout << "Got device collection" << std::endl;

    UINT deviceCount;
    hr = deviceCollection->GetCount(&deviceCount);
    if (FAILED(hr)) {
        std::cout << "Failed to get device count" << std::endl;
        return 0;
    }
    std::cout << "Got device count: " << deviceCount << std::endl;

    *nodes = new VsNode[deviceCount];

    for (UINT i = 0; i < deviceCount; i++) {
        IMMDevice *device = nullptr;
        hr = deviceCollection->Item(i, &device);
        if (FAILED(hr)) {
            std::cout << "Failed to get device" << std::endl;
            return i;
        }

        IPropertyStore *propertyStore = nullptr;
        hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
        device->Release();
        if (FAILED(hr)) {
            std::cout << "Failed to open property store" << std::endl;
            return i;
        }

        PROPVARIANT property;
        PropVariantInit(&property);
        hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &property);
        propertyStore->Release();
        if (FAILED(hr)) {
            std::cout << "Failed to get property value" << std::endl;
            return i;
        }

        LPWSTR pwszID = nullptr;
        hr = device->GetId(&pwszID);
        if (FAILED(hr)) {
            std::cout << "Failed to get device ID" << std::endl;
            return i;
        }

        (*nodes)[i].id = pwszID;
        (*nodes)[i].name = property.pwszVal;
        (*nodes)[i].volume = getVolume(device);
        (*nodes)[i].mute = isMuted(device);
        (*nodes)[i].isDefault = wcscmp(pwszID, defaultDeviceId) == 0;
    }

    deviceCollection->Release();

    return deviceCount;
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
            CoUninitialize();
            return 1;
        }

        int volume = std::stoi(argv[2]);
        if (volume < 0 || volume > 100) {
            std::cout << "Volume must be between 0 and 100" << std::endl;
            printUsage(argv);
            CoUninitialize();
            return 1;
        }

        setGlobalVolume(volume);
    } else if (command == "isGlobalMuted") {
        std::cout << isGlobalMuted() << std::endl;
    } else if (command == "setGlobalMute") {
        if (argc < 3) {
            std::cout << "Missing mute argument" << std::endl;
            printUsage(argv);
            CoUninitialize();
            return 1;
        }

        std::string muteStr = argv[2];
        if (muteStr != "1" && muteStr != "0") {
            std::cout << "Mute must be 1 or 0" << std::endl;
            printUsage(argv);
            CoUninitialize();
            return 1;
        }

        setGlobalMute(muteStr == "1");
    } else if (command == "getSinks") {
        VsNode **nodes;
        UINT count = getSinks(nodes);

        std::cout << "[" << std::endl;
        for (UINT i = 0; i < count; i++) {
            std::cout << "  {" << std::endl;
            std::cout << "    \"id\": \"" << (*nodes)[i].id << "\"," << std::endl;
            std::cout << "    \"name\": \"" << (*nodes)[i].name << "\"," << std::endl;
            std::cout << "    \"volume\": " << (*nodes)[i].volume << "," << std::endl;
            std::cout << "    \"mute\": " << (*nodes)[i].mute << "," << std::endl;
            std::cout << "    \"isDefault\": " << (*nodes)[i].isDefault << std::endl;
            std::cout << "  }" << (i < count - 1 ? "," : "") << std::endl;
        }
        std::cout << "]" << std::endl;
    } else {
        std::cout << "Unknown command: " << command << std::endl;
        printUsage(argv);
        CoUninitialize();
        return 1;
    }

    CoUninitialize();
    return 0;
}
