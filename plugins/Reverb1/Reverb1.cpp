#include "Reverb1.h"

#include "Engine.h"

#include "embed.h"

#include "plugin_export.h"

#include <cmath>



namespace lmms {



// --- HELPER IMPLEMENTATIONS ---

Comb::Comb(int size) : m_bufSize(size), m_pos(0), m_feedback(0.5f), m_filterStore(0.0f), m_damp(0.2f) {

    m_buffer.resize(size, 0.0f);

}



float Comb::process(float input) {

    float output = m_buffer[m_pos];

    m_filterStore = (output * (1.0f - m_damp)) + (m_filterStore * m_damp);

    m_buffer[m_pos] = input + (m_filterStore * m_feedback);

    m_pos++;

    if (m_pos >= m_bufSize) m_pos = 0;

    return output;

}



Allpass::Allpass(int size) : m_bufSize(size), m_pos(0), m_feedback(0.5f) {

    m_buffer.resize(size, 0.0f);

}



float Allpass::process(float input) {

    float buffered = m_buffer[m_pos];

    float output = -input + buffered;

    m_buffer[m_pos] = input + (buffered * m_feedback);

    m_pos++;

    if (m_pos >= m_bufSize) m_pos = 0;

    return output;

}



// --- PLUGIN CORE ---



extern "C" {

Plugin::Descriptor PLUGIN_EXPORT Reverb1_plugin_descriptor = {

    "Reverb1",

    "Reverb1",

    QT_TRANSLATE_NOOP("PluginBrowser", "Algorithmic Reverb (Schroeder/Freeverb)"),

    "Your Name",

    0x0100,

    Plugin::Type::Effect,

    new PixmapLoader("artwork.svg"), 

    nullptr,

    nullptr,

};

}



const int COMB_TUNINGS[] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };

const int ALLPASS_TUNINGS[] = { 556, 441, 341, 225 };

const int STEREO_SPREAD = 23;



Reverb1Effect::Reverb1Effect(Model* parent, const Descriptor::SubPluginFeatures::Key* key)

    : Effect(&Reverb1_plugin_descriptor, parent, key),

      m_controls(this),

      m_preDelayWritePos(0)

{

    // Max Pre-Delay of ~500ms at 192kHz is safe (approx 96k samples)

    m_preDelaySize = 96000; 

    m_preDelayBuffer.resize(m_preDelaySize, 0.0f);



    for (int tune : COMB_TUNINGS) {

        m_combsL.push_back(std::make_unique<Comb>(tune));

        m_combsR.push_back(std::make_unique<Comb>(tune + STEREO_SPREAD));

    }

    for (int tune : ALLPASS_TUNINGS) {

        m_allpassesL.push_back(std::make_unique<Allpass>(tune));

        m_allpassesR.push_back(std::make_unique<Allpass>(tune + STEREO_SPREAD));

    }

}



Effect::ProcessStatus Reverb1Effect::processImpl(SampleFrame* buf, const fpp_t frames) {

    float sRate = (float)Engine::audioEngine()->outputSampleRate();

    if (sRate <= 0) sRate = 44100.0f;



    // 1. Fetch Controls

    float roomSize = m_controls.m_roomSizeModel.value();

    float damp = m_controls.m_dampingModel.value();

    float width = m_controls.m_widthModel.value();

    float wet = m_controls.m_wetModel.value();

    float dry = m_controls.m_dryModel.value();

    float preDelayMs = m_controls.m_preDelayModel.value();



    float feedback = 0.7f + (0.28f * roomSize); 



    // Update Filters

    for (auto& c : m_combsL) { c->setFeedback(feedback); c->setDamping(damp); }

    for (auto& c : m_combsR) { c->setFeedback(feedback); c->setDamping(damp); }



    // Calculate Pre-Delay read offset

    int preDelaySamples = (int)((preDelayMs / 1000.0f) * sRate);

    if (preDelaySamples > m_preDelaySize - 1) preDelaySamples = m_preDelaySize - 1;



    for (fpp_t i = 0; i < frames; ++i) {

        // Mono Sum Input

        float input = (buf[i][0] + buf[i][1]) * 0.015f; 



        // --- PRE-DELAY LOGIC ---

        m_preDelayBuffer[m_preDelayWritePos] = input;

        

        int readPos = m_preDelayWritePos - preDelaySamples;

        if (readPos < 0) readPos += m_preDelaySize;

        

        float delayedInput = m_preDelayBuffer[readPos];

        

        m_preDelayWritePos++;

        if (m_preDelayWritePos >= m_preDelaySize) m_preDelayWritePos = 0;

        // -----------------------



        float outL = 0.0f;

        float outR = 0.0f;



        // Process Combs (using delayedInput)

        for (auto& c : m_combsL) outL += c->process(delayedInput);

        for (auto& c : m_combsR) outR += c->process(delayedInput);



        // Process Allpasses

        for (auto& a : m_allpassesL) outL = a->process(outL);

        for (auto& a : m_allpassesR) outR = a->process(outR);



        // Stereo Width & Output Mix

        float wet1 = wet * (width / 2.0f + 0.5f);

        float wet2 = wet * ((1.0f - width) / 2.0f);

        

        float finalL = outL * wet1 + outR * wet2;

        float finalR = outR * wet1 + outL * wet2;



        buf[i][0] = (buf[i][0] * dry) + (outL * wet1 + outR * wet2);

        buf[i][1] = (buf[i][1] * dry) + (outR * wet1 + outL * wet2);

    }



    return Effect::ProcessStatus::Continue;

}



extern "C" {

PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data) {

    return new Reverb1Effect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));

}

}



} // namespace lmms
