#include "AudioInputAlsa.h"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace lmms {

AudioInputAlsa::AudioInputAlsa() = default;

AudioInputAlsa::~AudioInputAlsa() {
    close();
}

std::vector<AudioDeviceInfo> AudioInputAlsa::getAvailableDevices() {
    std::vector<AudioDeviceInfo> devices;
    
    // Get card list
    int card = -1;
    while (snd_card_next(&card) >= 0 && card >= 0) {
        char* name = nullptr;
        if (snd_card_get_name(card, &name) >= 0) {
            AudioDeviceInfo info;
            info.name = name;
            info.id = "hw:" + std::to_string(card);
            info.channels = 2; // Default stereo
            info.sampleRates = {44100, 48000, 96000};
            devices.push_back(info);
            free(name);
        }
    }
    
    // Always add default device
    if (devices.empty()) {
        AudioDeviceInfo defaultDev;
        defaultDev.name = "Default";
        defaultDev.id = "default";
        defaultDev.channels = 2;
        defaultDev.sampleRates = {44100, 48000, 96000};
        devices.push_back(defaultDev);
    }
    
    return devices;
}

bool AudioInputAlsa::open(const std::string& deviceId, int channels, int sampleRate, int bufferSize) {
    if (m_isOpen) {
        close();
    }
    
    m_channels = channels;
    m_sampleRate = sampleRate;
    m_bufferSize = bufferSize;
    
    int err = snd_pcm_open(&m_pcmHandle, deviceId.c_str(), SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        std::cerr << "ALSA: Cannot open device " << deviceId << ": " << snd_strerror(err) << std::endl;
        return false;
    }
    
    snd_pcm_hw_params_t* hwParams = nullptr;
    snd_pcm_hw_params_alloca(&hwParams);
    
    err = snd_pcm_hw_params_any(m_pcmHandle, hwParams);
    if (err < 0) {
        std::cerr << "ALSA: Cannot initialize hardware params: " << snd_strerror(err) << std::endl;
        close();
        return false;
    }
    
    err = snd_pcm_hw_params_set_access(m_pcmHandle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        std::cerr << "ALSA: Cannot set access type: " << snd_strerror(err) << std::endl;
        close();
        return false;
    }
    
    snd_pcm_format_t format = SND_PCM_FORMAT_FLOAT;
    err = snd_pcm_hw_params_set_format(m_pcmHandle, hwParams, format);
    if (err < 0) {
        // Fallback to S16_LE
        format = SND_PCM_FORMAT_S16_LE;
        err = snd_pcm_hw_params_set_format(m_pcmHandle, hwParams, format);
        if (err < 0) {
            std::cerr << "ALSA: Cannot set format: " << snd_strerror(err) << std::endl;
            close();
            return false;
        }
    }
    
    err = snd_pcm_hw_params_set_channels(m_pcmHandle, hwParams, channels);
    if (err < 0) {
        std::cerr << "ALSA: Cannot set channels: " << snd_strerror(err) << std::endl;
        close();
        return false;
    }
    
    unsigned int rate = sampleRate;
    int dir = 0;
    err = snd_pcm_hw_params_set_rate_near(m_pcmHandle, hwParams, &rate, &dir);
    if (err < 0) {
        std::cerr << "ALSA: Cannot set sample rate: " << snd_strerror(err) << std::endl;
        close();
        return false;
    }
    
    err = snd_pcm_hw_params(m_pcmHandle, hwParams);
    if (err < 0) {
        std::cerr << "ALSA: Cannot set hardware params: " << snd_strerror(err) << std::endl;
        close();
        return false;
    }
    
    m_isOpen = true;
    std::cout << "ALSA: Opened device " << deviceId << " at " << rate << "Hz, " << channels << " channels" << std::endl;
    
    return true;
}

void AudioInputAlsa::close() {
    stopRecording();
    
    if (m_pcmHandle) {
        snd_pcm_close(m_pcmHandle);
        m_pcmHandle = nullptr;
    }
    
    m_isOpen = false;
}

bool AudioInputAlsa::isOpen() const {
    return m_isOpen;
}

bool AudioInputAlsa::startRecording() {
    if (!m_isOpen || m_isRecording) {
        return false;
    }
    
    int err = snd_pcm_prepare(m_pcmHandle);
    if (err < 0) {
        std::cerr << "ALSA: Cannot prepare: " << snd_strerror(err) << std::endl;
        return false;
    }
    
    m_shouldRun = true;
    m_audioThread = std::thread(&AudioInputAlsa::audioThread, this);
    m_isRecording = true;
    
    std::cout << "ALSA: Recording started" << std::endl;
    return true;
}

bool AudioInputAlsa::stopRecording() {
    if (!m_isRecording) {
        return false;
    }
    
    m_shouldRun = false;
    m_isRecording = false;
    
    if (m_audioThread.joinable()) {
        m_audioThread.join();
    }
    
    if (m_pcmHandle) {
        snd_pcm_drop(m_pcmHandle);
    }
    
    std::cout << "ALSA: Recording stopped" << std::endl;
    return true;
}

bool AudioInputAlsa::isRecording() const {
    return m_isRecording;
}

void AudioInputAlsa::audioThread() {
    m_buffer.resize(m_bufferSize * m_channels);
    
    while (m_shouldRun) {
        snd_pcm_sframes_t frames = snd_pcm_readi(m_pcmHandle, m_buffer.data(), m_bufferSize);
        
        if (frames < 0) {
            // Try to recover from error
            frames = snd_pcm_recover(m_pcmHandle, frames, 0);
            if (frames < 0) {
                std::cerr << "ALSA: Read error: " << snd_strerror(frames) << std::endl;
                continue;
            }
        }
        
        if (frames > 0 && m_callback) {
            m_callback(m_buffer.data(), frames);
        }
    }
}

} // namespace lmms
