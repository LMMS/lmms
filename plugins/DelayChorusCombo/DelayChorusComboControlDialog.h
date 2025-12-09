#ifndef LMMS_GUI_DELAY_CHORUS_COMBO_CONTROL_DIALOG_H
#define LMMS_GUI_DELAY_CHORUS_COMBO_CONTROL_DIALOG_H

#include "EffectControlDialog.h" // Use quotes if your system prefers them!

namespace lmms {
class DelayChorusComboControls;

namespace gui {

class DelayChorusComboControlDialog : public EffectControlDialog {
    Q_OBJECT
public:
    DelayChorusComboControlDialog(DelayChorusComboControls* controls);
    ~DelayChorusComboControlDialog() override = default;
};

} // namespace gui
} // namespace lmms

#endif // LMMS_GUI_DELAY_CHORUS_COMBO_CONTROL_DIALOG_H