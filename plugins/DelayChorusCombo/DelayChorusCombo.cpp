#include "DelayChorusCombo.h"
#include "Engine.h"
#include "embed.h"
#include "plugin_export.h"
#include <cmath>
#include <algorithm>

namespace lmms {

const float PI = 3.14159265358979f;
const int MAX_DELAY_SAMPLES = 88200; 

extern "C" {
Plugin::Descriptor PLUGIN_EXPORT DelayChorusCombo_plugin_descriptor = {
    "DelayChorusCombo",
    "DelayChorusCombo",
    QT_TRANSLATE_NOOP("PluginBrowser", "Combined Flanger and Chorus Effect"),
    "Your Name",
    0x0100,
    Plugin::Type::Effect,
    new PixmapLoader("artwork.svg"),
    nullptr,
    nullptr,
};
}

DelayChorusComboEffect::DelayChorusComboEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key)
    : Effect(&DelayChorusCombo_plugin_descriptor, parent, key), 
      m_controls(this),
      m_writePos(0),
      m_lfoPhase(0.0f),
      m_filterStateL(0.0f), // Init filters
      m_filterStateR(0.0f)
{
    m_bufferL.resize(MAX_DELAY_SAMPLES, 0.0f);
    m_bufferR.resize(MAX_DELAY_SAMPLES, 0.0f);
}

inline float interpolate(const std::vector<float>& buffer, float index) {
    int i = static_cast<int>(index);
    float frac = index - i;
    int i2 = i + 1;
    if (i2 >= MAX_DELAY_SAMPLES) i2 = 0;
    return buffer[i] * (1.0f - frac) + buffer[i2] * frac;
}

Effect::ProcessStatus DelayChorusComboEffect::processImpl(SampleFrame* buf, const fpp_t frames) {
    float sRate = (float)Engine::audioEngine()->outputSampleRate();
    if (sRate <= 0.0f) sRate = 44100.0f;

    float baseDelayMs = m_controls.m_delayModel.value();
    float depthMs = m_controls.m_depthModel.value();
    float rateHz = m_controls.m_rateModel.value();
    float feedback = m_controls.m_feedbackModel.value() / 100.0f;
    float mix = m_controls.m_mixModel.value() / 100.0f;
    float stereoOffsetRad = m_controls.m_stereoModel.value() * (PI / 180.0f);

    // NEW CONTROLS
    float damp = m_controls.m_dampModel.value() / 100.0f; // 0..1
    bool useTriangle = (m_controls.m_shapeModel.value() > 0.5f);
    float cross = m_controls.m_crossModel.value() / 100.0f;

    float lfoInc = (2.0f * PI * rateHz) / sRate;

    for (fpp_t i = 0; i < frames; ++i) {
        
        // 1. LFO SHAPE LOGIC
        float lfoRawL, lfoRawR;
        
        // Phase for Right channel
        float phaseR = m_lfoPhase + stereoOffsetRad;
        if (phaseR > 2.0f * PI) phaseR -= 2.0f * PI;

        if (useTriangle) {
            // Triangle Wave Approximation: 
            // Normalized phase 0..1 -> 2 * abs(2x - 1) - 1
            float normL = m_lfoPhase / (2.0f * PI);
            float normR = phaseR / (2.0f * PI);
            lfoRawL = 2.0f * std::abs(2.0f * normL - 1.0f) - 1.0f;
            lfoRawR = 2.0f * std::abs(2.0f * normR - 1.0f) - 1.0f;
        } else {
            // Standard Sine
            lfoRawL = std::sin(m_lfoPhase);
            lfoRawR = std::sin(phaseR);
        }
        
        m_lfoPhase += lfoInc;
        if (m_lfoPhase > 2.0f * PI) m_lfoPhase -= 2.0f * PI;

        // 2. DELAY CALCULATION
        float delaySamplesL = (baseDelayMs + depthMs * lfoRawL) * (sRate / 1000.0f);
        float delaySamplesR = (baseDelayMs + depthMs * lfoRawR) * (sRate / 1000.0f);

        if (delaySamplesL < 1.0f) delaySamplesL = 1.0f;
        if (delaySamplesR < 1.0f) delaySamplesR = 1.0f;
        
        float readPosL = m_writePos - delaySamplesL;
        float readPosR = m_writePos - delaySamplesR;
        
        if (readPosL < 0) readPosL += MAX_DELAY_SAMPLES;
        if (readPosR < 0) readPosR += MAX_DELAY_SAMPLES;

        float wetL = interpolate(m_bufferL, readPosL);
        float wetR = interpolate(m_bufferR, readPosR);

        // 3. DAMPING (Low Pass Filter on Wet Signal)
        // Simple One-Pole Filter: out = out + coeff * (in - out)
        // 'damp' is 0..1. We invert it so knob 100% means heavy filtering (low cutoff)
        float filterCoeff = 1.0f - damp; 
        
        m_filterStateL += filterCoeff * (wetL - m_filterStateL);
        m_filterStateR += filterCoeff * (wetR - m_filterStateR);
        
        float filteredL = m_filterStateL;
        float filteredR = m_filterStateR;

        // 4. CROSS FEEDBACK
        // Mix Left Feedback into Right channel and vice versa based on Cross knob
        float feedL = (filteredL * (1.0f - cross)) + (filteredR * cross);
        float feedR = (filteredR * (1.0f - cross)) + (filteredL * cross);

        float inL = buf[i][0];
        float inR = buf[i][1];
        
        m_bufferL[m_writePos] = inL + (feedL * feedback);
        m_bufferR[m_writePos] = inR + (feedR * feedback);

        // Mix Output (using unfiltered wet for brighter sound, or filtered for darker)
        // Usually Flangers mix the FILTERED signal to output.
        buf[i][0] = (inL * (1.0f - mix)) + (filteredL * mix);
        buf[i][1] = (inR * (1.0f - mix)) + (filteredR * mix);

        m_writePos++;
        if (m_writePos >= MAX_DELAY_SAMPLES) m_writePos = 0;
    }

    return Effect::ProcessStatus::Continue;
}

extern "C" {
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data) {
    return new DelayChorusComboEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}
}

} // namespace lmms