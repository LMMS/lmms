#ifndef RECORDING_ENGINE_H
#define RECORDING_ENGINE_H

#include "AudioInputDevice.h"
#include <memory>
#include <vector>
#include <string>
#include <mutex>

namespace lmms {

class RecordingTrack;
class WavWriter;

enum class RecordingState {
    Stopped,
    Armed,
    Recording
};

class RecordingEngine {
public:
    RecordingEngine();
    ~RecordingEngine();
    
    // Backend management
    bool initialize(const std::string& backend = "auto"); // auto, alsa, pipewire
    void shutdown();
    bool isInitialized() const;
    
    // Device selection
    std::vector<AudioDeviceInfo> getAvailableDevices();
    bool setDevice(const std::string& deviceId);
    std::string getCurrentDevice() const;
    
    // Recording control
    bool armRecording();
    bool startRecording();
    bool stopRecording();
    RecordingState getState() const;
    
    // Track management
    RecordingTrack* createTrack(const std::string& name);
    void removeTrack(RecordingTrack* track);
    std::vector<RecordingTrack*> getTracks() const;
    
    // Settings
    void setSampleRate(int rate) { m_sampleRate = rate; }
    void setChannels(int channels) { m_channels = channels; }
    void setOutputDirectory(const std::string& path) { m_outputDir = path; }
    
    int getSampleRate() const { return m_sampleRate; }
    int getChannels() const { return m_channels; }
    std::string getOutputDirectory() const { return m_outputDir; }
    
private:
    void onAudioInput(const float* buffer, size_t frames);
    
    std::unique_ptr<AudioInputDevice> m_inputDevice;
    std::vector<std::unique_ptr<RecordingTrack>> m_tracks;
    std::vector<RecordingTrack*> m_activeTracks;
    
    RecordingState m_state = RecordingState::Stopped;
    std::mutex m_mutex;
    
    int m_sampleRate = 44100;
    int m_channels = 2;
    int m_bufferSize = 1024;
    std::string m_outputDir = ".";
    std::string m_currentDevice;
};

} // namespace lmms

#endif // RECORDING_ENGINE_H
