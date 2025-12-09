#include "PitchShifter.h"
#include "Engine.h" // Correct Sample Rate!
#include "embed.h"
#include "plugin_export.h"
#include <cmath>
#include <algorithm>

namespace lmms {

extern "C" {
Plugin::Descriptor PLUGIN_EXPORT PitchShifter_plugin_descriptor = {
    "PitchShifter",
    "PitchShifter",
    QT_TRANSLATE_NOOP("PluginBrowser", "Real-time Pitch Shifter"),
    "Your Name",
    0x0100,
    Plugin::Type::Effect,
    new PixmapLoader("artwork.svg"),
    nullptr,
    nullptr,
};
}

PitchShifterEffect::PitchShifterEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key)
    : Effect(&PitchShifter_plugin_descriptor, parent, key), 
      m_controls(this),
      m_writePos(0),
      m_phasor(0.0)
{
    // 200ms buffer is plenty for real-time
    m_bufferSize = 192000; 
    m_bufferL.resize(m_bufferSize, 0.0f);
    m_bufferR.resize(m_bufferSize, 0.0f);
}

// Linear Interpolation Helper
inline float interpolate(const std::vector<float>& buf, float index, int size) {
    int i = static_cast<int>(index);
    float frac = index - i;
    int i2 = i + 1;
    if (i2 >= size) i2 = 0;
    return buf[i] * (1.0f - frac) + buf[i2] * frac;
}

Effect::ProcessStatus PitchShifterEffect::processImpl(SampleFrame* buf, const fpp_t frames) {
    // 1. Get Correct Sample Rate
    float sRate = (float)Engine::audioEngine()->outputSampleRate();
    if (sRate <= 0.0f) sRate = 44100.0f;

    float semi = m_controls.m_semitoneModel.value();
    float cents = m_controls.m_centModel.value();
    float grainMs = m_controls.m_grainModel.value();
    float mix = m_controls.m_mixModel.value() / 100.0f;

    // 2. Calculate Pitch Ratio
    // Formula: 2 ^ (semitones / 12)
    float totalSemi = semi + (cents / 100.0f);
    float ratio = std::pow(2.0f, totalSemi / 12.0f);

    // 3. Calculate Grain Window
    // The "phasor" moves based on the difference between Play Speed and Record Speed
    // If ratio = 1.0 (no shift), speed = 0 (grains don't move)
    // If ratio = 2.0 (octave up), we eat through buffer faster
    float windowSizeSamples = (grainMs / 1000.0f) * sRate;
    if (windowSizeSamples < 100.0f) windowSizeSamples = 100.0f; // Safety minimum

    float speed = (1.0f - ratio) / windowSizeSamples;

    for (fpp_t i = 0; i < frames; ++i) {
        // A. Write to Circular Buffer
        float inL = buf[i][0];
        float inR = buf[i][1];

        m_bufferL[m_writePos] = inL;
        m_bufferR[m_writePos] = inR;

        // B. Calculate Read Positions from Phasor
        // We use TWO taps (Tap A and Tap B) offset by 0.5 (180 degrees)
        // This allows us to crossfade: when Tap A wraps around (click), Tap B is at max volume.
        
        float tapA_offset = m_phasor * windowSizeSamples;
        float tapB_offset = std::fmod(m_phasor + 0.5f, 1.0f) * windowSizeSamples;

        // "Look back" into buffer
        float readIdxA = m_writePos - tapA_offset;
        float readIdxB = m_writePos - tapB_offset;

        // Wrap indices
        while (readIdxA < 0) readIdxA += m_bufferSize;
        while (readIdxA >= m_bufferSize) readIdxA -= m_bufferSize;
        while (readIdxB < 0) readIdxB += m_bufferSize;
        while (readIdxB >= m_bufferSize) readIdxB -= m_bufferSize;

        // C. Read Audio
        float sampleLA = interpolate(m_bufferL, readIdxA, m_bufferSize);
        float sampleLB = interpolate(m_bufferL, readIdxB, m_bufferSize);
        float sampleRA = interpolate(m_bufferR, readIdxA, m_bufferSize);
        float sampleRB = interpolate(m_bufferR, readIdxB, m_bufferSize);

        // D. Calculate Window Gain (Triangle Shape)
        // Phasor 0.0 -> Gain 0
        // Phasor 0.5 -> Gain 1
        // Phasor 1.0 -> Gain 0
        float gainA = 1.0f - std::abs(2.0f * m_phasor - 1.0f);
        
        // Tap B is offset by 0.5, so we calculate its phase relative to that
        float phaseB = std::fmod(m_phasor + 0.5f, 1.0f);
        float gainB = 1.0f - std::abs(2.0f * phaseB - 1.0f);
        
        // Normalize gain so volume doesn't drop
        // (Simple crossfade sum is usually consistent enough for grains)
        
        float wetL = (sampleLA * gainA) + (sampleLB * gainB);
        float wetR = (sampleRA * gainA) + (sampleRB * gainB);

        // E. Output
        buf[i][0] = (inL * (1.0f - mix)) + (wetL * mix);
        buf[i][1] = (inR * (1.0f - mix)) + (wetR * mix);

        // F. Advance Pointers
        m_writePos++;
        if (m_writePos >= m_bufferSize) m_writePos = 0;

        m_phasor += speed;
        // Wrap Phasor 0..1
        if (m_phasor >= 1.0f) m_phasor -= 1.0f;
        if (m_phasor < 0.0f)  m_phasor += 1.0f;
    }

    return Effect::ProcessStatus::Continue;
}

extern "C" {
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data) {
    return new PitchShifterEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}
}

} // namespace lmms