#ifndef LMMS_GUI_DELAY2_CONTROL_DIALOG_H
#define LMMS_GUI_DELAY2_CONTROL_DIALOG_H

#include "EffectControlDialog.h" 

namespace lmms {
class Delay2Controls;

namespace gui {

class Delay2ControlDialog : public EffectControlDialog {
    Q_OBJECT
public:
    Delay2ControlDialog(Delay2Controls* controls);
    ~Delay2ControlDialog() override = default;
};

} // namespace gui
} // namespace lmms

#endif // LMMS_GUI_DELAY2_CONTROL_DIALOG_H