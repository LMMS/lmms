#ifndef LMMS_GUI_REVERB1_CONTROL_DIALOG_H
#define LMMS_GUI_REVERB1_CONTROL_DIALOG_H

#include "EffectControlDialog.h" 

namespace lmms {
class Reverb1Controls;

namespace gui {

class Reverb1ControlDialog : public EffectControlDialog {
    Q_OBJECT
public:
    Reverb1ControlDialog(Reverb1Controls* controls);
    ~Reverb1ControlDialog() override = default;
};

} // namespace gui
} // namespace lmms

#endif
