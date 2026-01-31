#ifndef RECORDING_TRACK_H
#define RECORDING_TRACK_H

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

namespace lmms {

class WavWriter;

class RecordingTrack {
public:
    RecordingTrack(const std::string& name, int channels, int sampleRate);
    ~RecordingTrack();
    
    // Track info
    std::string getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    // Recording control
    void arm(bool armed);
    bool isArmed() const { return m_isArmed; }
    
    void startRecording();
    void stopRecording();
    bool isRecording() const { return m_isRecording; }
    
    // Audio processing
    void processAudio(const float* buffer, size_t frames);
    
    // File management
    bool startWriting(const std::string& filepath);
    void stopWriting();
    std::string getFilePath() const { return m_filePath; }
    
    // Status
    double getRecordedSeconds() const;
    size_t getTotalFrames() const { return m_totalFrames; }
    
private:
    std::string m_name;
    std::string m_filePath;
    
    int m_channels;
    int m_sampleRate;
    
    std::atomic<bool> m_isArmed{false};
    std::atomic<bool> m_isRecording{false};
    
    std::unique_ptr<WavWriter> m_writer;
    std::mutex m_writerMutex;
    
    size_t m_totalFrames = 0;
    double m_startTime = 0;
};

} // namespace lmms

#endif // RECORDING_TRACK_H
