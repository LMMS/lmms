#ifndef LMMS_PITCH_SHIFTER_CONTROLS_H
#define LMMS_PITCH_SHIFTER_CONTROLS_H

#include "EffectControls.h"
#include "PitchShifterControlDialog.h"

namespace lmms {

class PitchShifterEffect; 

class PitchShifterControls : public EffectControls {
    Q_OBJECT

public:
    PitchShifterControls(PitchShifterEffect* effect);
    ~PitchShifterControls() override = default;

    void saveSettings(QDomDocument& doc, QDomElement& parent) override;
    void loadSettings(const QDomElement& parent) override;

    inline QString nodeName() const override { return "PitchShifterControls"; }

    gui::EffectControlDialog* createView() override { return new gui::PitchShifterControlDialog(this); }
    int controlCount() override { return 4; }

    // Models
    FloatModel m_semitoneModel; // Coarse Pitch (-12 to +12)
    FloatModel m_centModel;     // Fine Pitch (-100 to +100)
    FloatModel m_grainModel;    // Window Size (ms)
    FloatModel m_mixModel;      // Dry/Wet

private:
    PitchShifterEffect* m_effect;
};

} // namespace lmms

#endif // LMMS_PITCH_SHIFTER_CONTROLS_H