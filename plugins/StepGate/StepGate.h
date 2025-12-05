#ifndef LMMS_STEPGATE_H

#define LMMS_STEPGATE_H



#include "Effect.h"

#include "StepGateControls.h"

#include <vector>



namespace lmms

{



class StepGateEffect : public Effect

{

public:

    StepGateEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);

    ~StepGateEffect() override = default;



    ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;

    EffectControls* controls() override { return &m_controls; }



private:

    StepGateControls m_controls;



    // Gate DSP

    double m_phase;

    float m_currentGain;



    // -------------------------

    // Delay DSP

    // -------------------------

    float m_prevFeedback = 0.0f;

    float m_prevWet = 0.0f;



    int m_delayBufferSize = 0;

    int m_delayWritePos = 0;



    std::vector<float> m_delayBufferL;

    std::vector<float> m_delayBufferR;



    void initDelayBuffer(float sampleRate);

    float readDelaySample(const std::vector<float>& buf, int readPos, int bufferSize);



    friend class StepGateControls;

};



} // namespace lmms



#endif // LMMS_STEPGATE_H

