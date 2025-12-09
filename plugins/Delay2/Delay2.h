#ifndef LMMS_DELAY2_H
#define LMMS_DELAY2_H

#include "Effect.h"
#include "Delay2Controls.h"
#include <vector>

namespace lmms {

class Delay2Effect : public Effect {
public:
    Delay2Effect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
    ~Delay2Effect() override = default;

    ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;
    EffectControls* controls() override { return &m_controls; }

private:
    Delay2Controls m_controls;
    
    // Stereo Circular Buffer
    std::vector<float> m_bufferL;
    std::vector<float> m_bufferR;
    int m_writePos;
    int m_bufferSize;

    // Filter State (for Feedback Cutoff)
    float m_filterStateL;
    float m_filterStateR;
};

} // namespace lmms

#endif // LMMS_DELAY2_H