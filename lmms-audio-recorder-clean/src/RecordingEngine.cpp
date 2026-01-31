#include "RecordingEngine.h"
#include "RecordingTrack.h"
#include "AudioInputAlsa.h"
#include "AudioInputPipeWire.h"
#include <iostream>
#include <algorithm>

namespace lmms {

RecordingEngine::RecordingEngine() = default;

RecordingEngine::~RecordingEngine() {
    shutdown();
}

bool RecordingEngine::initialize(const std::string& backend) {
    shutdown();
    
    if (backend == "alsa") {
        m_inputDevice = std::make_unique<AudioInputAlsa>();
    } else if (backend == "pipewire") {
        m_inputDevice = std::make_unique<AudioInputPipeWire>();
    } else if (backend == "auto") {
        // Try PipeWire first, then ALSA
        auto pipewire = std::make_unique<AudioInputPipeWire>();
        auto devices = pipewire->getAvailableDevices();
        if (!devices.empty()) {
            m_inputDevice = std::move(pipewire);
            std::cout << "RecordingEngine: Using PipeWire backend" << std::endl;
        } else {
            m_inputDevice = std::make_unique<AudioInputAlsa>();
            std::cout << "RecordingEngine: Using ALSA backend" << std::endl;
        }
    } else {
        std::cerr << "RecordingEngine: Unknown backend: " << backend << std::endl;
        return false;
    }
    
    // Set up callback
    m_inputDevice->setAudioCallback([this](const float* buffer, size_t frames) {
        onAudioInput(buffer, frames);
    });
    
    return true;
}

void RecordingEngine::shutdown() {
    stopRecording();
    
    if (m_inputDevice) {
        m_inputDevice->close();
        m_inputDevice.reset();
    }
    
    m_tracks.clear();
    m_activeTracks.clear();
}

bool RecordingEngine::isInitialized() const {
    return m_inputDevice != nullptr;
}

std::vector<AudioDeviceInfo> RecordingEngine::getAvailableDevices() {
    if (!m_inputDevice) {
        return {};
    }
    return m_inputDevice->getAvailableDevices();
}

bool RecordingEngine::setDevice(const std::string& deviceId) {
    if (!m_inputDevice) {
        return false;
    }
    
    if (m_state != RecordingState::Stopped) {
        std::cerr << "RecordingEngine: Cannot change device while recording" << std::endl;
        return false;
    }
    
    if (m_inputDevice->isOpen()) {
        m_inputDevice->close();
    }
    
    if (!m_inputDevice->open(deviceId, m_channels, m_sampleRate, m_bufferSize)) {
        return false;
    }
    
    m_currentDevice = deviceId;
    return true;
}

std::string RecordingEngine::getCurrentDevice() const {
    return m_currentDevice;
}

bool RecordingEngine::armRecording() {
    if (!m_inputDevice) {
        std::cerr << "RecordingEngine: Not initialized" << std::endl;
        return false;
    }
    
    if (m_state == RecordingState::Recording) {
        std::cerr << "RecordingEngine: Already recording" << std::endl;
        return false;
    }
    
    // Open device if not already open
    if (!m_inputDevice->isOpen() && !m_currentDevice.empty()) {
        if (!m_inputDevice->open(m_currentDevice, m_channels, m_sampleRate, m_bufferSize)) {
            return false;
        }
    }
    
    // Auto-open first available device if none selected
    if (!m_inputDevice->isOpen()) {
        auto devices = getAvailableDevices();
        if (devices.empty()) {
            std::cerr << "RecordingEngine: No audio devices available" << std::endl;
            return false;
        }
        
        if (!m_inputDevice->open(devices[0].id, m_channels, m_sampleRate, m_bufferSize)) {
            return false;
        }
        m_currentDevice = devices[0].id;
    }
    
    // Arm all tracks
    for (auto* track : m_activeTracks) {
        track->arm(true);
    }
    
    m_state = RecordingState::Armed;
    std::cout << "RecordingEngine: Armed" << std::endl;
    return true;
}

bool RecordingEngine::startRecording() {
    if (m_state != RecordingState::Armed) {
        if (!armRecording()) {
            return false;
        }
    }
    
    // Start all armed tracks
    for (auto* track : m_activeTracks) {
        if (track->isArmed()) {
            std::string filepath = m_outputDir + "/" + track->getName() + "_" + 
                                   std::to_string(time(nullptr)) + ".wav";
            track->startWriting(filepath);
            track->startRecording();
        }
    }
    
    if (!m_inputDevice->startRecording()) {
        return false;
    }
    
    m_state = RecordingState::Recording;
    std::cout << "RecordingEngine: Recording started" << std::endl;
    return true;
}

bool RecordingEngine::stopRecording() {
    if (m_state != RecordingState::Recording) {
        return false;
    }
    
    m_inputDevice->stopRecording();
    
    // Stop all recording tracks
    for (auto* track : m_activeTracks) {
        if (track->isRecording()) {
            track->stopRecording();
            track->stopWriting();
            track->arm(false);
        }
    }
    
    m_state = RecordingState::Stopped;
    std::cout << "RecordingEngine: Recording stopped" << std::endl;
    return true;
}

RecordingState RecordingEngine::getState() const {
    return m_state;
}

RecordingTrack* RecordingEngine::createTrack(const std::string& name) {
    auto track = std::make_unique<RecordingTrack>(name, m_channels, m_sampleRate);
    RecordingTrack* ptr = track.get();
    m_tracks.push_back(std::move(track));
    m_activeTracks.push_back(ptr);
    return ptr;
}

void RecordingEngine::removeTrack(RecordingTrack* track) {
    auto it = std::find(m_activeTracks.begin(), m_activeTracks.end(), track);
    if (it != m_activeTracks.end()) {
        m_activeTracks.erase(it);
    }
    
    auto trackIt = std::find_if(m_tracks.begin(), m_tracks.end(),
        [track](const auto& t) { return t.get() == track; });
    if (trackIt != m_tracks.end()) {
        m_tracks.erase(trackIt);
    }
}

std::vector<RecordingTrack*> RecordingEngine::getTracks() const {
    return m_activeTracks;
}

void RecordingEngine::onAudioInput(const float* buffer, size_t frames) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto* track : m_activeTracks) {
        if (track->isRecording()) {
            track->processAudio(buffer, frames);
        }
    }
}

} // namespace lmms
