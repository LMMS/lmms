#ifndef AUDIO_INPUT_PIPEWIRE_H
#define AUDIO_INPUT_PIPEWIRE_H

#include "AudioInputDevice.h"
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <thread>
#include <atomic>

namespace lmms {

class AudioInputPipeWire : public AudioInputDevice {
public:
    AudioInputPipeWire();
    ~AudioInputPipeWire() override;
    
    std::vector<AudioDeviceInfo> getAvailableDevices() override;
    bool open(const std::string& deviceId, int channels, int sampleRate, int bufferSize) override;
    void close() override;
    bool isOpen() const override;
    
    bool startRecording() override;
    bool stopRecording() override;
    bool isRecording() const override;
    
    std::string getBackendName() const override { return "PipeWire"; }
    
private:
    static void onProcess(void* userdata);
    void processAudio();
    
    struct pw_main_loop* m_loop = nullptr;
    struct pw_stream* m_stream = nullptr;
    struct pw_context* m_context = nullptr;
    struct pw_core* m_core = nullptr;
    
    std::thread m_thread;
    std::atomic<bool> m_shouldRun{false};
};

} // namespace lmms

#endif // AUDIO_INPUT_PIPEWIRE_H
