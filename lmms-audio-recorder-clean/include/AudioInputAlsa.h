#ifndef AUDIO_INPUT_ALSA_H
#define AUDIO_INPUT_ALSA_H

#include "AudioInputDevice.h"
#include <alsa/asoundlib.h>
#include <thread>
#include <atomic>

namespace lmms {

class AudioInputAlsa : public AudioInputDevice {
public:
    AudioInputAlsa();
    ~AudioInputAlsa() override;
    
    std::vector<AudioDeviceInfo> getAvailableDevices() override;
    bool open(const std::string& deviceId, int channels, int sampleRate, int bufferSize) override;
    void close() override;
    bool isOpen() const override;
    
    bool startRecording() override;
    bool stopRecording() override;
    bool isRecording() const override;
    
    std::string getBackendName() const override { return "ALSA"; }
    
private:
    void audioThread();
    
    snd_pcm_t* m_pcmHandle = nullptr;
    std::thread m_audioThread;
    std::atomic<bool> m_shouldRun{false};
    std::vector<float> m_buffer;
};

} // namespace lmms

#endif // AUDIO_INPUT_ALSA_H
