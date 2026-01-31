#include "RecordingTrack.h"
#include "WavWriter.h"
#include <iostream>
#include <cmath>

namespace lmms {

RecordingTrack::RecordingTrack(const std::string& name, int channels, int sampleRate)
    : m_name(name)
    , m_channels(channels)
    , m_sampleRate(sampleRate)
{
    m_writer = std::make_unique<WavWriter>();
}

RecordingTrack::~RecordingTrack() {
    stopRecording();
    stopWriting();
}

void RecordingTrack::arm(bool armed) {
    m_isArmed = armed;
}

void RecordingTrack::startRecording() {
    if (!m_isArmed) {
        return;
    }
    
    m_isRecording = true;
    m_startTime = 0; // Will be set on first audio frame
    std::cout << "RecordingTrack '" << m_name << "': Started recording" << std::endl;
}

void RecordingTrack::stopRecording() {
    m_isRecording = false;
    std::cout << "RecordingTrack '" << m_name << "': Stopped recording" << std::endl;
}

void RecordingTrack::processAudio(const float* buffer, size_t frames) {
    if (!m_isRecording) {
        return;
    }
    
    if (m_startTime == 0) {
        m_startTime = 0; // Use frame counter instead
    }
    
    // Write to file if we have a writer
    {
        std::lock_guard<std::mutex> lock(m_writerMutex);
        if (m_writer && m_writer->isOpen()) {
            m_writer->writeInterleaved(buffer, frames);
        }
    }
    
    m_totalFrames += frames;
}

bool RecordingTrack::startWriting(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(m_writerMutex);
    
    m_filePath = filepath;
    
    if (!m_writer->open(filepath, m_channels, m_sampleRate)) {
        std::cerr << "RecordingTrack: Failed to open file " << filepath << std::endl;
        return false;
    }
    
    std::cout << "RecordingTrack '" << m_name << "': Writing to " << filepath << std::endl;
    return true;
}

void RecordingTrack::stopWriting() {
    std::lock_guard<std::mutex> lock(m_writerMutex);
    
    if (m_writer) {
        m_writer->close();
    }
}

double RecordingTrack::getRecordedSeconds() const {
    return static_cast<double>(m_totalFrames) / m_sampleRate;
}

} // namespace lmms
