#ifndef LMMS_STEPGATE_CONTROLS_H
g
#define LMMS_STEPGATE_CONTROLS_H



#include "EffectControls.h"

#include <vector>



namespace lmms

{



class StepGateEffect;



class StepGateControls : public EffectControls

{

    Q_OBJECT



public:

    StepGateControls(StepGateEffect* effect);

    ~StepGateControls() override = default;



    void saveSettings(QDomDocument& doc, QDomElement& parent) override;

    void loadSettings(const QDomElement& parent) override;



    gui::EffectControlDialog* createView() override;



    // Step Management

    void setStep(int patternIdx, int stepIdx, bool active);

    bool getStep(int patternIdx, int stepIdx) const;

    bool getCurrentStep(int stepIdx) const;



    // -------------------------------

    // Control Models (Knobs)

    // -------------------------------

    FloatModel m_patternModel;   // Pattern A–D

    FloatModel m_smoothModel;    // Gate smoothing

    FloatModel m_swingModel;     // Swing

    FloatModel m_speedModel;     // Pattern speed (0.5–4)



    // Delay

    FloatModel m_delayTimeModel; // 0=1/16, 1=1/8, 2=3/16, 3=1/4

    FloatModel m_feedbackModel;  // 0–1.0

    FloatModel m_wetModel;       // 0–1.0



    // Pattern Data: 4 patterns × 16 steps

    std::vector<std::vector<bool>> m_patterns;



    inline QString nodeName() const override { return "StepGateControls"; }



    // Total number of controls

    int controlCount() override { return 7; }



    // Running LED index

    void setRunIndex(int index);

    int runIndex() const { return m_runIndex; }



signals:

    void stepsChanged();

    void runIndexChanged(int index);



private:

    int m_runIndex = -1;



    friend class StepGateEffect;

};



} // namespace lmms



#endif // LMMS_STEPGATE_CONTROLS_H

