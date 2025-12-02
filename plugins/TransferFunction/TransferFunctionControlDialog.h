

#ifndef LMMS_GUI_TransferFunction_CONTROL_DIALOG_H
#define LMMS_GUI_TransferFunction_CONTROL_DIALOG_H

#include "EffectControlDialog.h"

namespace lmms
{

class TransferFunctionControls;

namespace gui
{

class TransferFunctionControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	TransferFunctionControlDialog(TransferFunctionControls* controls);
	~TransferFunctionControlDialog() override = default;
};

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_TransferFunction_CONTROL_DIALOG_H
