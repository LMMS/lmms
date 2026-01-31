#include "WavWriter.h"
#include <cstring>
#include <vector>
#include <algorithm>

namespace lmms {

WavWriter::WavWriter() = default;

WavWriter::~WavWriter() {
    close();
}

bool WavWriter::open(const std::string& filepath, int channels, int sampleRate, int bitsPerSample) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_isOpen) {
        close();
    }
    
    m_file = fopen(filepath.c_str(), "wb");
    if (!m_file) {
        return false;
    }
    
    m_filepath = filepath;
    m_channels = channels;
    m_sampleRate = sampleRate;
    m_bitsPerSample = bitsPerSample;
    m_totalFrames = 0;
    
    // Write placeholder header
    writeHeader();
    
    m_isOpen = true;
    return true;
}

void WavWriter::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_isOpen || !m_file) {
        return;
    }
    
    // Update header with final sizes
    updateHeader();
    
    fclose(m_file);
    m_file = nullptr;
    m_isOpen = false;
}

bool WavWriter::isOpen() const {
    return m_isOpen;
}

bool WavWriter::write(const float* data, size_t frames) {
    return writeInterleaved(data, frames);
}

bool WavWriter::writeInterleaved(const float* data, size_t frames) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_isOpen || !m_file) {
        return false;
    }
    
    if (m_bitsPerSample == 16) {
        // Convert float to int16
        std::vector<int16_t> buffer(frames * m_channels);
        for (size_t i = 0; i < frames * m_channels; ++i) {
            float sample = data[i];
            // Clamp to [-1.0, 1.0]
            sample = std::max(-1.0f, std::min(1.0f, sample));
            // Convert to int16
            buffer[i] = static_cast<int16_t>(sample * 32767.0f);
        }
        
        size_t written = fwrite(buffer.data(), sizeof(int16_t), buffer.size(), m_file);
        if (written != buffer.size()) {
            return false;
        }
    } else if (m_bitsPerSample == 24) {
        // Convert float to int24 (3 bytes)
        std::vector<uint8_t> buffer(frames * m_channels * 3);
        for (size_t i = 0; i < frames * m_channels; ++i) {
            float sample = data[i];
            sample = std::max(-1.0f, std::min(1.0f, sample));
            int32_t intSample = static_cast<int32_t>(sample * 8388607.0f);
            
            buffer[i * 3] = intSample & 0xFF;
            buffer[i * 3 + 1] = (intSample >> 8) & 0xFF;
            buffer[i * 3 + 2] = (intSample >> 16) & 0xFF;
        }
        
        size_t written = fwrite(buffer.data(), 1, buffer.size(), m_file);
        if (written != buffer.size()) {
            return false;
        }
    } else if (m_bitsPerSample == 32) {
        // Convert float to int32
        std::vector<int32_t> buffer(frames * m_channels);
        for (size_t i = 0; i < frames * m_channels; ++i) {
            float sample = data[i];
            sample = std::max(-1.0f, std::min(1.0f, sample));
            buffer[i] = static_cast<int32_t>(sample * 2147483647.0f);
        }
        
        size_t written = fwrite(buffer.data(), sizeof(int32_t), buffer.size(), m_file);
        if (written != buffer.size()) {
            return false;
        }
    } else {
        // Float32 directly
        size_t written = fwrite(data, sizeof(float), frames * m_channels, m_file);
        if (written != frames * m_channels) {
            return false;
        }
    }
    
    m_totalFrames += frames;
    return true;
}

void WavWriter::writeHeader() {
    WavHeader header;
    
    header.numChannels = m_channels;
    header.sampleRate = m_sampleRate;
    header.bitsPerSample = m_bitsPerSample;
    header.byteRate = m_sampleRate * m_channels * (m_bitsPerSample / 8);
    header.blockAlign = m_channels * (m_bitsPerSample / 8);
    
    // For float32, use format 3 (IEEE float)
    if (m_bitsPerSample == 32) {
        header.audioFormat = 3; // IEEE float
    }
    
    // Calculate data chunk offset (after header)
    m_dataChunkOffset = sizeof(WavHeader);
    
    fwrite(&header, sizeof(WavHeader), 1, m_file);
}

void WavWriter::updateHeader() {
    if (!m_file) return;
    
    // Seek to beginning
    fseek(m_file, 0, SEEK_SET);
    
    WavHeader header;
    
    header.numChannels = m_channels;
    header.sampleRate = m_sampleRate;
    header.bitsPerSample = m_bitsPerSample;
    header.byteRate = m_sampleRate * m_channels * (m_bitsPerSample / 8);
    header.blockAlign = m_channels * (m_bitsPerSample / 8);
    
    if (m_bitsPerSample == 32) {
        header.audioFormat = 3; // IEEE float
    }
    
    // Calculate sizes
    uint32_t dataSize = m_totalFrames * m_channels * (m_bitsPerSample / 8);
    header.riffSize = sizeof(WavHeader) - 8 + dataSize;
    header.dataSize = dataSize;
    
    fwrite(&header, sizeof(WavHeader), 1, m_file);
}

} // namespace lmms
