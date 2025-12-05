#ifndef LMMS_GUI_STEPGATE_CONTROL_DIALOG_H

#define LMMS_GUI_STEPGATE_CONTROL_DIALOG_H

d

#include "EffectControlDialog.h"

#include <vector>



class QPushButton;



namespace lmms

{

class StepGateControls;



namespace gui

{



class StepGateControlDialog : public EffectControlDialog

{

    Q_OBJECT



public:

    StepGateControlDialog(StepGateControls* controls);

    ~StepGateControlDialog() override = default;



public slots:

    // Update 16-step toggle buttons when pattern changes

    void updateButtonsFromModel();



    // When a step button is clicked

    void onButtonClicked();



    // Running LED highlight

    void onRunIndexChanged(int index);



private:

    StepGateControls* m_controls;



    // 16 buttons for sequencer steps

    std::vector<QPushButton*> m_stepButtons;

};



} // namespace gui

} // namespace lmms



#endif // LMMS_GUI_STEPGATE_CONTROL_DIALOG_H

