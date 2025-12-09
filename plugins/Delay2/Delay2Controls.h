#ifndef LMMS_DELAY2_CONTROLS_H
#define LMMS_DELAY2_CONTROLS_H

#include "EffectControls.h"
#include "Delay2ControlDialog.h"

namespace lmms {

class Delay2Effect; 

class Delay2Controls : public EffectControls {
    Q_OBJECT

public:
    Delay2Controls(Delay2Effect* effect);
    ~Delay2Controls() override = default;

    void saveSettings(QDomDocument& doc, QDomElement& parent) override;
    void loadSettings(const QDomElement& parent) override;

    inline QString nodeName() const override { return "Delay2Controls"; }

    gui::EffectControlDialog* createView() override { return new gui::Delay2ControlDialog(this); }
    int controlCount() override { return 8; }

    // INPUT
    FloatModel m_inputVolModel;  // Input Drive
    
    // FEEDBACK
    FloatModel m_feedbackModel;  // Feedback Level
    FloatModel m_cutoffModel;    // Low-pass filter on echoes
    FloatModel m_pingPongModel;  // 0 = Normal, 1 = PingPong
    
    // TIME
    FloatModel m_timeModel;      // Base Delay Time (ms)
    FloatModel m_offsetModel;    // Stereo Offset (ms) - FL Style "Stereo" knob
    
    // MIX
    FloatModel m_dryModel;
    FloatModel m_wetModel;

private:
    Delay2Effect* m_effect;
};

} // namespace lmms

#endif // LMMS_DELAY2_CONTROLS_H