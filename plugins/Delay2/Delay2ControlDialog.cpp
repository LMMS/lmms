#include "Delay2ControlDialog.h"
#include "Delay2Controls.h"
#include "Knob.h"
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>

namespace lmms::gui {

Delay2ControlDialog::Delay2ControlDialog(Delay2Controls* controls)
    : EffectControlDialog(controls) {

    setMinimumSize(450, 200);
    QGridLayout* mainLayout = new QGridLayout(this);

    auto makeKnob = [this](const QString& label, const QString& unit, FloatModel* model) {
        Knob* knob = new Knob(KnobType::Bright26, label, this);
        knob->setHintText(label, unit);
        knob->setModel(model);
        return knob;
    };

    // 1. INPUT Section
    QGroupBox* grpInput = new QGroupBox("Input", this);
    QGridLayout* lInput = new QGridLayout(grpInput);
    lInput->addWidget(makeKnob("Vol", "%", &controls->m_inputVolModel), 0, 0);
    lInput->addWidget(makeKnob("Dry", "%", &controls->m_dryModel), 0, 1);
    mainLayout->addWidget(grpInput, 0, 0);

    // 2. FEEDBACK Section
    QGroupBox* grpFeed = new QGroupBox("Feedback", this);
    QGridLayout* lFeed = new QGridLayout(grpFeed);
    lFeed->addWidget(makeKnob("Vol", "%", &controls->m_feedbackModel), 0, 0);
    lFeed->addWidget(makeKnob("Cut", "Hz", &controls->m_cutoffModel), 0, 1);
    lFeed->addWidget(makeKnob("P.Pong", "Off/On", &controls->m_pingPongModel), 0, 2);
    mainLayout->addWidget(grpFeed, 0, 1);

    // 3. TIME Section
    QGroupBox* grpTime = new QGroupBox("Time", this);
    QGridLayout* lTime = new QGridLayout(grpTime);
    lTime->addWidget(makeKnob("Time", "ms", &controls->m_timeModel), 0, 0);
    lTime->addWidget(makeKnob("Stereo", "ms", &controls->m_offsetModel), 0, 1);
    lTime->addWidget(makeKnob("Wet", "%", &controls->m_wetModel), 0, 2);
    mainLayout->addWidget(grpTime, 0, 2);
}

} // namespace lmms::gui