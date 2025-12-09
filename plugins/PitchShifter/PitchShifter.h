#ifndef LMMS_PITCH_SHIFTER_H
#define LMMS_PITCH_SHIFTER_H

#include "Effect.h"
#include "PitchShifterControls.h"
#include <vector>

namespace lmms {

class PitchShifterEffect : public Effect {
public:
    PitchShifterEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
    ~PitchShifterEffect() override = default;

    ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;
    EffectControls* controls() override { return &m_controls; }

private:
    PitchShifterControls m_controls;
    
    // Circular Buffer
    std::vector<float> m_bufferL;
    std::vector<float> m_bufferR;
    int m_writePos;
    int m_bufferSize;
    
    // Phasor for Grain Position (0.0 to 1.0)
    double m_phasor;
};

} // namespace lmms

#endif // LMMS_PITCH_SHIFTER_H