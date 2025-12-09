#ifndef LMMS_GUI_PITCH_SHIFTER_CONTROL_DIALOG_H
#define LMMS_GUI_PITCH_SHIFTER_CONTROL_DIALOG_H

#include "EffectControlDialog.h" 

namespace lmms {
class PitchShifterControls;

namespace gui {

class PitchShifterControlDialog : public EffectControlDialog {
    Q_OBJECT
public:
    PitchShifterControlDialog(PitchShifterControls* controls);
    ~PitchShifterControlDialog() override = default;
};

} // namespace gui
} // namespace lmms

#endif // LMMS_GUI_PITCH_SHIFTER_CONTROL_DIALOG_H