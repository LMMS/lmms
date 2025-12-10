#ifndef LMMS_REVERB1_H

#define LMMS_REVERB1_H



#include "Effect.h"

#include "Reverb1Controls.h"

#include <vector>

#include <memory>



namespace lmms {



class Comb {

public:

    Comb(int size);

    float process(float input);

    void setFeedback(float val) { m_feedback = val; }

    void setDamping(float val) { m_damp = val; }

private:

    std::vector<float> m_buffer;

    int m_bufSize;

    int m_pos;

    float m_feedback;

    float m_filterStore;

    float m_damp;

};



class Allpass {

public:

    Allpass(int size);

    float process(float input);

private:

    std::vector<float> m_buffer;

    int m_bufSize;

    int m_pos;

    float m_feedback;

};



class Reverb1Effect : public Effect {

public:

    Reverb1Effect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);

    ~Reverb1Effect() override = default;



    ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;

    EffectControls* controls() override { return &m_controls; }



private:

    Reverb1Controls m_controls;

    

    // Filter Networks

    std::vector<std::unique_ptr<Comb>> m_combsL;

    std::vector<std::unique_ptr<Comb>> m_combsR;

    std::vector<std::unique_ptr<Allpass>> m_allpassesL;

    std::vector<std::unique_ptr<Allpass>> m_allpassesR;



    // Pre-Delay Buffer

    std::vector<float> m_preDelayBuffer;

    int m_preDelayWritePos;

    int m_preDelaySize;

};



} // namespace lmms



#endif // LMMS_REVERB1_H
