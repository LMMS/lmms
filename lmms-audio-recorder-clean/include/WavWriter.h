#ifndef WAV_WRITER_H
#define WAV_WRITER_H

#include <string>
#include <cstdio>
#include <cstdint>
#include <mutex>

namespace lmms {

// Simple WAV file writer for real-time audio recording
class WavWriter {
public:
    WavWriter();
    ~WavWriter();
    
    // Open file for writing
    bool open(const std::string& filepath, int channels, int sampleRate, int bitsPerSample = 16);
    void close();
    bool isOpen() const;
    
    // Write audio data (thread-safe)
    bool write(const float* data, size_t frames);
    bool writeInterleaved(const float* data, size_t frames);
    
    // Get info
    std::string getFilePath() const { return m_filepath; }
    size_t getTotalFramesWritten() const { return m_totalFrames; }
    
private:
    void writeHeader();
    void updateHeader();
    
    FILE* m_file = nullptr;
    std::string m_filepath;
    std::mutex m_mutex;
    
    int m_channels = 2;
    int m_sampleRate = 44100;
    int m_bitsPerSample = 16;
    
    size_t m_totalFrames = 0;
    size_t m_dataChunkOffset = 0;
    
    bool m_isOpen = false;
};

// WAV file header structure
struct WavHeader {
    // RIFF chunk
    char riffId[4] = {'R', 'I', 'F', 'F'};
    uint32_t riffSize = 0;
    char waveId[4] = {'W', 'A', 'V', 'E'};
    
    // fmt chunk
    char fmtId[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1; // PCM
    uint16_t numChannels = 2;
    uint32_t sampleRate = 44100;
    uint32_t byteRate = 0;
    uint16_t blockAlign = 0;
    uint16_t bitsPerSample = 16;
    
    // data chunk
    char dataId[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize = 0;
};

} // namespace lmms

#endif // WAV_WRITER_H
