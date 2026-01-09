#ifndef LMMS_TRANSFERFUNCTION_H
#define LMMS_TRANSFERFUNCTION_H

#include "Effect.h"
#include "TransferFunctionControls.h"
#include <vector>
#include <complex>
#include <string>

namespace lmms
{

typedef std::complex<float> Complex;

class TransferFunctionEffect : public Effect
{
public:
    TransferFunctionEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
    ~TransferFunctionEffect() override = default;

    ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;
    EffectControls* controls() override { return &m_ampControls; }

private:
    TransferFunctionControls m_ampControls;

    // STEREO FFT BUFFERS
    std::vector<float> m_historyL;
    std::vector<float> m_historyR;
    
    std::vector<float> m_overlapAddL;
    std::vector<float> m_overlapAddR;

    // Shared Window 
    std::vector<float> m_window;

    // --- NEW: TIME DOMAIN EFFECT BUFFERS ---
    std::vector<float> m_delayBufferL;
    std::vector<float> m_delayBufferR;
    int m_delayPos;

    friend class TransferFunctionControls;
};

} // namespace lmms

#endif // LMMS_TRANSFERFUNCTION_H
