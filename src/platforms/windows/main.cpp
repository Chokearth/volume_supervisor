#include <iostream>
#include <string>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

void printUsage(char *argv[]) {
    std::cout << "Usage: " << argv[0] << " [command] [args...]\n" << std::endl;

    std::cout << "Commands:" << std::endl;
    std::cout << "  getGlobalVolume - Get the global volume" << std::endl;
    std::cout << "  setGlobalVolume [volume] - Set the global volume, volume must be between 0 and 100" << std::endl;
    std::cout << "  isGlobalMuted - Check if the global volume is muted" << std::endl;
    std::cout << "  setGlobalMute [mute] - Set the global mute, mute must be 1 or 0" << std::endl;
}

IAudioEndpointVolume *getEndpointVolume() {
    HRESULT hr;
    
    // Get the speakers (1st render + multimedia) device
    CoInitialize(nullptr);
    IMMDeviceEnumerator *deviceEnumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator),
                          (LPVOID *) &deviceEnumerator);
    if (FAILED(hr)) {
        std::cout << "Failed to create device enumerator" << std::endl;
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

int getGlobalVolume() {
    HRESULT hr;

    IAudioEndpointVolume *endpointVolume = getEndpointVolume();
    if (endpointVolume == nullptr) {
        return 1;
    }

    // Get the current volume
    float volume;
    hr = endpointVolume->GetMasterVolumeLevelScalar(&volume);
    endpointVolume->Release();
    CoUninitialize();
    if (FAILED(hr)) {
        std::cout << "Failed to get master volume level" << std::endl;
        return 1;
    }

    return (int) (volume * 100);
}

void setGlobalVolume(int volume) {
    HRESULT hr;

    IAudioEndpointVolume *endpointVolume = getEndpointVolume();
    if (endpointVolume == nullptr) {
        return;
    }

    // Set the volume
    hr = endpointVolume->SetMasterVolumeLevelScalar((float) volume / 100, nullptr);
    endpointVolume->Release();
    CoUninitialize();
    if (FAILED(hr)) {
        std::cout << "Failed to set master volume level" << std::endl;
        return;
    }
}

bool isGlobalMuted() {
    HRESULT hr;

    IAudioEndpointVolume *endpointVolume = getEndpointVolume();
    if (endpointVolume == nullptr) {
        return false;
    }

    // Get the current mute state
    BOOL mute;
    hr = endpointVolume->GetMute(&mute);
    endpointVolume->Release();
    CoUninitialize();
    if (FAILED(hr)) {
        std::cout << "Failed to get mute state" << std::endl;
        return false;
    }

    return mute;
}

void setGlobalMute(bool mute) {
    HRESULT hr;

    IAudioEndpointVolume *endpointVolume = getEndpointVolume();
    if (endpointVolume == nullptr) {
        return;
    }

    // Set the mute state
    hr = endpointVolume->SetMute(mute, nullptr);
    endpointVolume->Release();
    CoUninitialize();
    if (FAILED(hr)) {
        std::cout << "Failed to set mute state" << std::endl;
        return;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printUsage(argv);
        return 1;
    }

    std::string command = argv[1];

    if (command == "getGlobalVolume") {
        std::cout << getGlobalVolume() << std::endl;
    } else if (command == "setGlobalVolume") {
        if (argc < 3) {
            std::cout << "Missing volume argument" << std::endl;
            printUsage(argv);
            return 1;
        }

        int volume = std::stoi(argv[2]);
        if (volume < 0 || volume > 100) {
            std::cout << "Volume must be between 0 and 100" << std::endl;
            printUsage(argv);
            return 1;
        }

        setGlobalVolume(volume);
    } else if (command == "isGlobalMuted") {
        std::cout << isGlobalMuted() << std::endl;
    } else if (command == "setGlobalMute") {
        if (argc < 3) {
            std::cout << "Missing mute argument" << std::endl;
            printUsage(argv);
            return 1;
        }

        std::string muteStr = argv[2];
        if (muteStr != "1" && muteStr != "0") {
            std::cout << "Mute must be 1 or 0" << std::endl;
            printUsage(argv);
            return 1;
        }

        setGlobalMute(muteStr == "1");
    } else {
        std::cout << "Unknown command: " << command << std::endl;
        printUsage(argv);
        return 1;
    }

    return 0;
}
