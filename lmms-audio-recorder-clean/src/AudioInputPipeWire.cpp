#include "AudioInputPipeWire.h"
#include <iostream>
#include <cstring>

namespace lmms {

AudioInputPipeWire::AudioInputPipeWire() = default;

AudioInputPipeWire::~AudioInputPipeWire() {
    close();
}

std::vector<AudioDeviceInfo> AudioInputPipeWire::getAvailableDevices() {
    std::vector<AudioDeviceInfo> devices;
    
    // Initialize PipeWire for device enumeration
    pw_init(nullptr, nullptr);
    
    // For now, return default device
    // Full implementation would use pw_registry to list nodes
    AudioDeviceInfo defaultDev;
    defaultDev.name = "Default";
    defaultDev.id = "default";
    defaultDev.channels = 2;
    defaultDev.sampleRates = {44100, 48000, 96000};
    devices.push_back(defaultDev);
    
    return devices;
}

bool AudioInputPipeWire::open(const std::string& deviceId, int channels, int sampleRate, int bufferSize) {
    if (m_isOpen) {
        close();
    }
    
    m_channels = channels;
    m_sampleRate = sampleRate;
    m_bufferSize = bufferSize;
    
    pw_init(nullptr, nullptr);
    
    m_loop = pw_main_loop_new(nullptr);
    if (!m_loop) {
        std::cerr << "PipeWire: Failed to create main loop" << std::endl;
        return false;
    }
    
    struct pw_loop* l = pw_main_loop_get_loop(m_loop);
    m_context = pw_context_new(l, nullptr, 0);
    if (!m_context) {
        std::cerr << "PipeWire: Failed to create context" << std::endl;
        close();
        return false;
    }
    
    m_core = pw_context_connect(m_context, nullptr, 0);
    if (!m_core) {
        std::cerr << "PipeWire: Failed to connect" << std::endl;
        close();
        return false;
    }
    
    // Create stream
    static const struct pw_stream_events streamEvents = {
        PW_VERSION_STREAM_EVENTS,
        .process = onProcess,
    };
    
    m_stream = pw_stream_new(m_core, "LMMS Recording", nullptr);
    if (!m_stream) {
        std::cerr << "PipeWire: Failed to create stream" << std::endl;
        close();
        return false;
    }
    
    // Set up audio format
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    
    struct spa_audio_info_raw info = {};
    info.format = SPA_AUDIO_FORMAT_F32;
    info.channels = channels;
    info.rate = sampleRate;
    
    const struct spa_pod* params = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);
    
    int res = pw_stream_connect(m_stream,
                                PW_DIRECTION_INPUT,
                                PW_ID_ANY,
                                (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
                                &params, 1);
    
    if (res < 0) {
        std::cerr << "PipeWire: Failed to connect stream" << std::endl;
        close();
        return false;
    }
    
    pw_stream_add_listener(m_stream, nullptr, &streamEvents, this);
    
    m_isOpen = true;
    std::cout << "PipeWire: Opened device at " << sampleRate << "Hz, " << channels << " channels" << std::endl;
    
    return true;
}

void AudioInputPipeWire::close() {
    stopRecording();
    
    if (m_stream) {
        pw_stream_destroy(m_stream);
        m_stream = nullptr;
    }
    
    if (m_core) {
        pw_core_disconnect(m_core);
        m_core = nullptr;
    }
    
    if (m_context) {
        pw_context_destroy(m_context);
        m_context = nullptr;
    }
    
    if (m_loop) {
        pw_main_loop_destroy(m_loop);
        m_loop = nullptr;
    }
    
    m_isOpen = false;
}

bool AudioInputPipeWire::isOpen() const {
    return m_isOpen;
}

bool AudioInputPipeWire::startRecording() {
    if (!m_isOpen || m_isRecording) {
        return false;
    }
    
    m_shouldRun = true;
    m_thread = std::thread([this]() {
        pw_main_loop_run(m_loop);
    });
    
    m_isRecording = true;
    std::cout << "PipeWire: Recording started" << std::endl;
    return true;
}

bool AudioInputPipeWire::stopRecording() {
    if (!m_isRecording) {
        return false;
    }
    
    m_shouldRun = false;
    
    if (m_loop) {
        pw_main_loop_quit(m_loop);
    }
    
    if (m_thread.joinable()) {
        m_thread.join();
    }
    
    m_isRecording = false;
    std::cout << "PipeWire: Recording stopped" << std::endl;
    return true;
}

bool AudioInputPipeWire::isRecording() const {
    return m_isRecording;
}

void AudioInputPipeWire::onProcess(void* userdata) {
    AudioInputPipeWire* self = static_cast<AudioInputPipeWire*>(userdata);
    self->processAudio();
}

void AudioInputPipeWire::processAudio() {
    struct pw_buffer* b = pw_stream_dequeue_buffer(m_stream);
    if (!b) return;
    
    struct spa_buffer* buf = b->buffer;
    float* data = (float*)buf->datas[0].data;
    
    if (data && m_callback) {
        uint32_t frames = buf->datas[0].chunk->size / (m_channels * sizeof(float));
        m_callback(data, frames);
    }
    
    pw_stream_queue_buffer(m_stream, b);
}

} // namespace lmms
