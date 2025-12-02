#ifndef LMMS_TRANSFERFUNCTION_H
#define LMMS_TRANSFERFUNCTION_H

#include "Effect.h"
#include "TransferFunctionControls.h"
#include <vector>

namespace lmms {

class TransferFunctionEffect : public Effect {
public:
    TransferFunctionEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
    ~TransferFunctionEffect() override = default;

    ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;

    EffectControls* controls() override { return &m_ampControls; }

private:
    TransferFunctionControls m_ampControls;

    // State buffers for overlap-add processing
    std::vector<float> m_history;     // Previous input half-block
    std::vector<float> m_window;      // Hann window coefficients
    std::vector<float> m_overlapAdd;  // Overlap-add buffer for output
    friend class TransferFunctionControls;
};

} // namespace lmms

#endif // LMMS_TRANSFERFUNCTION_H
