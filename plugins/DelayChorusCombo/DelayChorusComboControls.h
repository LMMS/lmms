#ifndef LMMS_DELAY_CHORUS_COMBO_CONTROLS_H
#define LMMS_DELAY_CHORUS_COMBO_CONTROLS_H

#include "EffectControls.h"
#include "DelayChorusComboControlDialog.h"

namespace lmms {

class DelayChorusComboEffect; 

class DelayChorusComboControls : public EffectControls {
    Q_OBJECT

public:
    DelayChorusComboControls(DelayChorusComboEffect* effect);
    ~DelayChorusComboControls() override = default;

    void saveSettings(QDomDocument& doc, QDomElement& parent) override;
    void loadSettings(const QDomElement& parent) override;

    inline QString nodeName() const override { return "DelayChorusComboControls"; }

    gui::EffectControlDialog* createView() override { return new gui::DelayChorusComboControlDialog(this); }
    
    // Updated count: 6 + 3 new knobs = 9
    int controlCount() override { return 9; }

    FloatModel m_delayModel;
    FloatModel m_depthModel;
    FloatModel m_rateModel;
    FloatModel m_feedbackModel;
    FloatModel m_stereoModel;
    FloatModel m_mixModel;
    
    // NEW FEATURES
    FloatModel m_dampModel;  // Feedback Filter
    FloatModel m_shapeModel; // Sine vs Triangle
    FloatModel m_crossModel; // Cross-channel mixing

private:
    DelayChorusComboEffect* m_effect;
};

} // namespace lmms

#endif // LMMS_DELAY_CHORUS_COMBO_CONTROLS_H