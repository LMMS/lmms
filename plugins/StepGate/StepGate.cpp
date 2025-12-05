#include "StepGate.h"
;
#include "embed.h"

#include "plugin_export.h"



#include "Engine.h"

#include "AudioEngine.h"

#include "Song.h"



#include <cmath>



namespace lmms

{



extern "C" {

Plugin::Descriptor PLUGIN_EXPORT StepGate_plugin_descriptor = {

    QT_STRINGIFY(PLUGIN_NAME),

    "StepGate",

    QT_TRANSLATE_NOOP("PluginBrowser", "16-Step Trance Gate"),

    "Your Name",

    0x0100,

    Plugin::Type::Effect,

    new PixmapLoader("lmms-plugin-logo"),

    nullptr,

    nullptr,

};

}



// =============================================================

// Constructor

// =============================================================

StepGateEffect::StepGateEffect(Model* parent,

                               const Descriptor::SubPluginFeatures::Key* key)

    : Effect(&StepGate_plugin_descriptor, parent, key),

      m_controls(this),

      m_phase(0.0),

      m_currentGain(0.0f)

{

}



// =============================================================

// Initialize delay buffer when needed

// =============================================================

void StepGateEffect::initDelayBuffer(float sampleRate)

{

    float maxDelaySeconds = 2.0f;

    m_delayBufferSize = (int)(maxDelaySeconds * sampleRate);



    m_delayBufferL.assign(m_delayBufferSize, 0.0f);

    m_delayBufferR.assign(m_delayBufferSize, 0.0f);



    m_delayWritePos = 0;

}



// =============================================================

// Circular buffer reader helper

// =============================================================

float StepGateEffect::readDelaySample(const std::vector<float>& buf, int pos, int bufferSize)

{

    if(pos < 0) pos += bufferSize;

    if(pos >= bufferSize) pos -= bufferSize;

    return buf[pos];

}



// =============================================================

// MAIN DSP — Process Audio

// =============================================================

Effect::ProcessStatus StepGateEffect::processImpl(SampleFrame* buf,

                                                  const fpp_t frames)

{

    // -------------------------

    // Host timing

    // -------------------------

    float bpm  = Engine::getSong()->getTempo();

    float rate = Engine::audioEngine()->baseSampleRate();



    // Init delay buffer if needed

    if(m_delayBufferSize == 0)

        initDelayBuffer(rate);



    float speed       = m_controls.m_speedModel.value();

    float smoothAmount = m_controls.m_smoothModel.value();

    float swingAmount  = m_controls.m_swingModel.value();



    // Delay controls

    int   delayIndex  = (int)m_controls.m_delayTimeModel.value(); // 0..3

    float feedback    = m_controls.m_feedbackModel.value();

    float wet         = m_controls.m_wetModel.value();

    float dry         = 1.0f - wet;



    // tempo-synced delay table

    // Values in beats:

    // 1/16 = 0.25 beats

    // 1/8  = 0.5 beats

    // 3/16 = 0.75 beats

    // 1/4  = 1.0 beats

    static const float delayBeatValues[4] =

    {

        0.25f,

        0.50f,

        0.75f,

        1.00f

    };



    float delayBeats = delayBeatValues[delayIndex];

    float delaySeconds = (60.0f / bpm) * delayBeats;

    int delaySamples = (int)(delaySeconds * rate);



    if(delaySamples >= m_delayBufferSize)

        delaySamples = m_delayBufferSize - 1;



    // -------------------------

    // Timing for gate

    // -------------------------

    double samplesPerBar = (240.0 * rate) / bpm;

    double samplesPerStep = (samplesPerBar / 16.0) / speed;



    double swingOffset = samplesPerStep * swingAmount * 0.75;

    float inertia      = smoothAmount * 0.995f;



    // -------------------------

    // Compute current step index ONCE per block for LED

    // -------------------------

    double pairLength     = samplesPerStep * 2.0;

    double positionInPair = fmod(m_phase, pairLength);

    int    pairIndex      = (int)(m_phase / pairLength);

    double swingThreshold = samplesPerStep + swingOffset;



    int stepIndexInPair =

        (positionInPair < swingThreshold) ? 0 : 1;



    int blockStepIndex = (pairIndex * 2) + stepIndexInPair;

    blockStepIndex %= 16;



    m_controls.setRunIndex(blockStepIndex);



    // -------------------------

    // MAIN AUDIO LOOP

    // -------------------------

    for (fpp_t i = 0; i < frames; ++i)

    {

        // recalc pair position per sample

        double positionInPair_i = fmod(m_phase, pairLength);

        int pairIndex_i = (int)(m_phase / pairLength);



        int stepIndexInPair_i =

            (positionInPair_i < swingThreshold) ? 0 : 1;



        int stepIndex = (pairIndex_i * 2) + stepIndexInPair_i;

        stepIndex %= 16;



        bool isOpen  = m_controls.getCurrentStep(stepIndex);

        float target = isOpen ? 1.f : 0.f;



        // smoothing

        if (smoothAmount < 0.001f)

            m_currentGain = target;

        else

            m_currentGain =

                (m_currentGain * inertia) + (target * (1.f - inertia));



        // apply gate

        float inL = buf[i][0] * m_currentGain;

        float inR = buf[i][1] * m_currentGain;



        // ---- Delay DSP ----

        int readPos = m_delayWritePos - delaySamples;

        if(readPos < 0) readPos += m_delayBufferSize;



        float delayedL = readDelaySample(m_delayBufferL, readPos, m_delayBufferSize);

        float delayedR = readDelaySample(m_delayBufferR, readPos, m_delayBufferSize);



        // write new sample into circular buffer

        float fbL = delayedL * feedback;

        float fbR = delayedR * feedback;



        m_delayBufferL[m_delayWritePos] = inL + fbL;

        m_delayBufferR[m_delayWritePos] = inR + fbR;



        // increment write pos

        m_delayWritePos++;

        if(m_delayWritePos >= m_delayBufferSize)

            m_delayWritePos = 0;



        // output mix

        buf[i][0] = (inL * dry) + (delayedL * wet);

        buf[i][1] = (inR * dry) + (delayedR * wet);



        // advance gate phase

        m_phase += 1.0;

        if (m_phase >= samplesPerBar)

            m_phase -= samplesPerBar;

    }



    return Effect::ProcessStatus::Continue;

}



extern "C" {

PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data)

{

    return new StepGateEffect(

        parent,

        static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));

}

}



} // namespace lmms

