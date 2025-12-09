#include "Delay2.h"
#include "Engine.h" // Essential for correct timing
#include "embed.h"
#include "plugin_export.h"
#include <cmath>
#include <algorithm>

namespace lmms {

const float PI = 3.14159265359f;

extern "C" {
Plugin::Descriptor PLUGIN_EXPORT Delay2_plugin_descriptor = {
    "Delay2",
    "Delay2",
    QT_TRANSLATE_NOOP("PluginBrowser", "Standard Delay with Stereo Offset"),
    "Your Name",
    0x0100,
    Plugin::Type::Effect,
    new PixmapLoader("artwork.svg"),
    nullptr,
    nullptr,
};
}

Delay2Effect::Delay2Effect(Model* parent, const Descriptor::SubPluginFeatures::Key* key)
    : Effect(&Delay2_plugin_descriptor, parent, key), 
      m_controls(this),
      m_writePos(0),
      m_filterStateL(0.0f),
      m_filterStateR(0.0f)
{
    // 2 Second buffer at 96kHz = ~192000 samples. Plenty.
    m_bufferSize = 192000;
    m_bufferL.resize(m_bufferSize, 0.0f);
    m_bufferR.resize(m_bufferSize, 0.0f);
}

// Linear Interpolation
inline float interpolate(const std::vector<float>& buf, float index, int size) {
    int i = static_cast<int>(index);
    float frac = index - i;
    int i2 = i + 1;
    if (i2 >= size) i2 = 0;
    return buf[i] * (1.0f - frac) + buf[i2] * frac;
}

Effect::ProcessStatus Delay2Effect::processImpl(SampleFrame* buf, const fpp_t frames) {
    // 1. Get Sample Rate correctly
    float sRate = (float)Engine::audioEngine()->outputSampleRate();
    if (sRate <= 0.0f) sRate = 44100.0f;

    // 2. Fetch Controls
    float inVol = m_controls.m_inputVolModel.value() / 100.0f;
    float feedback = m_controls.m_feedbackModel.value() / 100.0f;
    float dry = m_controls.m_dryModel.value() / 100.0f;
    float wet = m_controls.m_wetModel.value() / 100.0f;
    
    bool pingPong = (m_controls.m_pingPongModel.value() > 0.5f);
    
    float timeMs = m_controls.m_timeModel.value();
    float offsetMs = m_controls.m_offsetModel.value();
    float cutoffHz = m_controls.m_cutoffModel.value();

    // 3. Calculate Delay Times (samples)
    float baseDelaySamps = (timeMs / 1000.0f) * sRate;
    float offsetSamps = (offsetMs / 1000.0f) * sRate;
    
    // Right channel delay = base + offset (offset can be negative!)
    float delayL = baseDelaySamps;
    float delayR = baseDelaySamps + offsetSamps;

    // Safety Clamps
    if (delayL < 1.0f) delayL = 1.0f;
    if (delayR < 1.0f) delayR = 1.0f;
    if (delayL > m_bufferSize - 1) delayL = m_bufferSize - 1;
    if (delayR > m_bufferSize - 1) delayR = m_bufferSize - 1;

    // 4. Calculate Filter Coefficient (Simple Low Pass)
    // alpha = 2 * pi * dt * fc
    // This is a rough approximation for a 1-pole filter, good enough for "damping"
    float dt = 1.0f / sRate;
    float alpha = 2.0f * PI * dt * cutoffHz;
    if (alpha > 1.0f) alpha = 1.0f;

    for (fpp_t i = 0; i < frames; ++i) {
        float inSampleL = buf[i][0] * inVol;
        float inSampleR = buf[i][1] * inVol;

        // READ from Buffer
        float readPosL = m_writePos - delayL;
        float readPosR = m_writePos - delayR;
        
        // Wrap read pointers
        while (readPosL < 0) readPosL += m_bufferSize;
        while (readPosR < 0) readPosR += m_bufferSize;

        float echoL = interpolate(m_bufferL, readPosL, m_bufferSize);
        float echoR = interpolate(m_bufferR, readPosR, m_bufferSize);

        // FILTER the echoes (Damping)
        m_filterStateL += alpha * (echoL - m_filterStateL);
        m_filterStateR += alpha * (echoR - m_filterStateR);
        
        float filteredL = m_filterStateL;
        float filteredR = m_filterStateR;

        // FEEDBACK Logic
        float feedL, feedR;
        
        if (pingPong) {
            // Ping Pong: Left Output feeds Right Input buffer, and vice versa
            feedL = filteredR * feedback;
            feedR = filteredL * feedback;
        } else {
            // Normal: Left feeds Left
            feedL = filteredL * feedback;
            feedR = filteredR * feedback;
        }

        // WRITE to Buffer (Input + Feedback)
        m_bufferL[m_writePos] = inSampleL + feedL;
        m_bufferR[m_writePos] = inSampleR + feedR;

        // OUTPUT Mix
        buf[i][0] = (buf[i][0] * dry) + (echoL * wet);
        buf[i][1] = (buf[i][1] * dry) + (echoR * wet);

        // Advance
        m_writePos++;
        if (m_writePos >= m_bufferSize) m_writePos = 0;
    }

    return Effect::ProcessStatus::Continue;
}

extern "C" {
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data) {
    return new Delay2Effect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}
}

} // namespace lmms