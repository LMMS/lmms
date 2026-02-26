#include "StepGateControlDialog.h"
#include "StepGateControls.h"
#include "Knob.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QVariant>
namespace lmms
{
namespace gui
{
StepGateControlDialog::StepGateControlDialog(StepGateControls* controls) :
    EffectControlDialog(controls),
    m_controls(controls)
{
    setMinimumWidth(700);
    setMinimumHeight(260);
    setWindowTitle("StepGate - Trance Gate + Delay");
    // MAIN LAYOUT: Vertical Stack
    // Row 1: Knobs
    // Row 2: Buttons
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20); 
    mainLayout->setContentsMargins(20, 20, 20, 20);
    // ------------------------------------------------------------
    // TOP ROW: KNOBS (Horizontal Layout)
    // ------------------------------------------------------------
   QHBoxLayout* knobLayout = new QHBoxLayout();
    knobLayout->setSpacing(15); 
    mainLayout->addLayout(knobLayout);
    // Pattern (A–D)
    Knob* patKnob = new Knob(KnobType::Bright26, "Pattern", this);
    patKnob->setModel(&m_controls->m_patternModel);
    patKnob->setHintText("Pattern", "Select Pattern A–D");
    knobLayout->addWidget(patKnob, 0, Qt::AlignCenter);
    // Speed
    Knob* speedKnob = new Knob(KnobType::Bright26, "Speed", this);
    speedKnob->setModel(&m_controls->m_speedModel);
    speedKnob->setHintText("Speed", "Pattern Speed (0.5x, 1x, 2x, 4x)");
    knobLayout->addWidget(speedKnob, 0, Qt::AlignCenter);
    // Smooth
    Knob* smoothKnob = new Knob(KnobType::Bright26, "Smooth", this);
    smoothKnob->setModel(&m_controls->m_smoothModel);
    smoothKnob->setHintText("Smooth", "Gate Smoothing");
    knobLayout->addWidget(smoothKnob, 0, Qt::AlignCenter);
    // Swing
    Knob* swingKnob = new Knob(KnobType::Bright26, "Swing", this);
    swingKnob->setModel(&m_controls->m_swingModel);
    swingKnob->setHintText("Swing", "Rhythmic Swing");
    knobLayout->addWidget(swingKnob, 0, Qt::AlignCenter);
    // Delay Time
    Knob* delayTimeKnob = new Knob(KnobType::Bright26, "Delay", this);
    delayTimeKnob->setModel(&m_controls->m_delayTimeModel);
    delayTimeKnob->setHintText("Delay Time", "1/16, 1/8, 3/16, 1/4");
    knobLayout->addWidget(delayTimeKnob, 0, Qt::AlignCenter);
    // Feedback
    Knob* feedbackKnob = new Knob(KnobType::Bright26, "Feedback", this);
    feedbackKnob->setModel(&m_controls->m_feedbackModel);
    feedbackKnob->setHintText("Feedback", "Delay Feedback");
    knobLayout->addWidget(feedbackKnob, 0, Qt::AlignCenter);
    // Wet
    Knob* wetKnob = new Knob(KnobType::Bright26, "Wet", this);
    wetKnob->setModel(&m_controls->m_wetModel);
    wetKnob->setHintText("Wet", "Delay Wet Mix");
    knobLayout->addWidget(wetKnob, 0, Qt::AlignCenter);
    // Center knobs if window is wide
    knobLayout->addStretch();
    knobLayout->insertStretch(0);
    // ------------------------------------------------------------
    // GATE BUTTON GRID (Horizontal Layout)
    // ------------------------------------------------------------
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(6); 
    mainLayout->addLayout(buttonLayout);
    for(int i = 0; i < 16; ++i)
    {
        QPushButton* btn = new QPushButton(QString::number(i + 1), this);
        btn->setCheckable(true);
        btn->setFixedSize(32, 42);
        btn->setProperty("stepIndex", QVariant(i));
        // default styles
        btn->setStyleSheet(
            "QPushButton { background-color:#800000; color:#fff; "
            "border:1px solid #400; border-radius:3px; }"
            "QPushButton:hover { background-color:#aa0000; }"
            "QPushButton:checked { background-color:#00FF00; color:#000; "
            "font-weight:bold; border:1px solid #0A0; }"
            "QPushButton:checked:hover { background-color:#55FF55; }"
        );
        connect(btn, &QPushButton::clicked,
                this, &StepGateControlDialog::onButtonClicked);
        m_stepButtons.push_back(btn);
        buttonLayout->addWidget(btn);
    }
    // SIGNAL CONNECTIONS
    connect(m_controls, &StepGateControls::stepsChanged,
            this, &StepGateControlDialog::updateButtonsFromModel);
    connect(&m_controls->m_patternModel, &FloatModel::dataChanged,
            this, &StepGateControlDialog::updateButtonsFromModel);
    connect(m_controls, &StepGateControls::runIndexChanged,
            this, &StepGateControlDialog::onRunIndexChanged);
    updateButtonsFromModel();
}
void StepGateControlDialog::onButtonClicked()
{
   QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if(!btn) return;
    int idx = btn->property("stepIndex").toInt();
    int pat = (int)m_controls->m_patternModel.value();
    m_controls->setStep(pat, idx, btn->isChecked());
}
void StepGateControlDialog::updateButtonsFromModel()
{
    int pat = (int)m_controls->m_patternModel.value();
    for(int i = 0; i < 16; ++i)
    {
        m_stepButtons[i]->blockSignals(true);
        m_stepButtons[i]->setChecked(m_controls->getStep(pat, i));
        m_stepButtons[i]->blockSignals(false);
    }
}
void StepGateControlDialog::onRunIndexChanged(int index)
{
    for(int i = 0; i < 16; ++i)
    {
        if(i == index)
        {
            // ALWAYS VISIBLE LED
            m_stepButtons[i]->setStyleSheet(
                "QPushButton { "
                "background-color:#00BFFF; "   
                "color:#000; "
                "border:3px solid #FFFFFF; "   
                "border-radius:3px; "
                "font-weight:bold; "
                "} "
            );
        }
        else
        {
            // normal appearance
            if(m_stepButtons[i]->isChecked())
            {
                m_stepButtons[i]->setStyleSheet(
                    "QPushButton { background-color:#00FF00; color:#000; "
                    "border:1px solid #0A0; border-radius:3px; }"
                );
            }
            else
            {
                m_stepButtons[i]->setStyleSheet(
                    "QPushButton { background-color:#800000; color:#fff; "
                    "border:1px solid #400; border-radius:3px; }"
                );
            }
        }
    }
}
} // namespace gui
} // namespace lmms
