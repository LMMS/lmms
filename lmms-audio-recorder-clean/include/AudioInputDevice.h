#ifndef AUDIO_INPUT_DEVICE_H
#define AUDIO_INPUT_DEVICE_H

#include <vector>
#include <string>
#include <functional>
#include <cstddef>

namespace lmms {

struct AudioDeviceInfo {
    std::string name;
    std::string id;
    int channels;
    std::vector<int> sampleRates;
};

class AudioInputDevice {
public:
    using AudioCallback = std::function<void(const float* buffer, size_t frames)>;
    
    virtual ~AudioInputDevice() = default;
    
    // Device management
    virtual std::vector<AudioDeviceInfo> getAvailableDevices() = 0;
    virtual bool open(const std::string& deviceId, int channels, int sampleRate, int bufferSize) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    
    // Recording control
    virtual bool startRecording() = 0;
    virtual bool stopRecording() = 0;
    virtual bool isRecording() const = 0;
    
    // Callback for audio data (called from audio thread)
    void setAudioCallback(AudioCallback callback) { m_callback = callback; }
    
    // Device info
    virtual std::string getBackendName() const = 0;
    
protected:
    AudioCallback m_callback;
    bool m_isRecording = false;
    bool m_isOpen = false;
    int m_channels = 2;
    int m_sampleRate = 44100;
    int m_bufferSize = 1024;
};

} // namespace lmms

#endif // AUDIO_INPUT_DEVICE_H
