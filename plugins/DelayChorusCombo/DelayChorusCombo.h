#ifndef LMMS_DELAY_CHORUS_COMBO_H
#define LMMS_DELAY_CHORUS_COMBO_H

#include "Effect.h"
#include "DelayChorusComboControls.h"
#include <vector>

namespace lmms {

class DelayChorusComboEffect : public Effect {
public:
    DelayChorusComboEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
    ~DelayChorusComboEffect() override = default;

    ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;
    EffectControls* controls() override { return &m_controls; }

private:
    DelayChorusComboControls m_controls;
    
    std::vector<float> m_bufferL;
    std::vector<float> m_bufferR;
    int m_writePos;
    float m_lfoPhase;

    // NEW: Memory for the Damping Filters (Low Pass)
    float m_filterStateL;
    float m_filterStateR;
};

} // namespace lmms

#endif // LMMS_DELAY_CHORUS_COMBO_H