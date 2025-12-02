// TransferFunction.h

#ifndef LMMS_TRANSFERFUNCTION_H

#define LMMS_TRANSFERFUNCTION_H



#include "Effect.h"

#include "TransferFunctionControls.h"

#include <vector>

#include <complex>

#include <string>



namespace lmms {



typedef std::complex<float> Complex;



class TransferFunctionEffect : public Effect {

public:

    TransferFunctionEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);

    ~TransferFunctionEffect() override = default;



    ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;



    EffectControls* controls() override { return &m_ampControls; }



private:

    TransferFunctionControls m_ampControls;



    std::vector<float> m_history;

    std::vector<float> m_window;

    std::vector<float> m_overlapAdd;



    Complex evalFormula(const std::string& expr, float freq);

    friend class TransferFunctionControls;

};



} // namespace lmms



#endif // LMMS_TRANSFERFUNCTION_H
