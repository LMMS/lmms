/* TransferFunction.h */
#ifndef LMMS_TRANSFERFUNCTION_H
#define LMMS_TRANSFERFUNCTION_H

#include "Effect.h"
#include "TransferFunctionControls.h"
#include <vector> // <--- Added

namespace lmms
{

class TransferFunctionEffect : public Effect
{
public:
	TransferFunctionEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~TransferFunctionEffect() override = default;

	ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;

	EffectControls* controls() override
	{
		return &m_ampControls;
	}

private:
	TransferFunctionControls m_ampControls;
	
	// MEMORY FOR OVERLAP-SAVE
	std::vector<float> m_history;

	friend class TransferFunctionControls;
};

} // namespace lmms

#endif // LMMS_TRANSFERFUNCTION_H