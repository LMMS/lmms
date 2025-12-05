#include "StepGateControls.h"
v
#include "StepGateControlDialog.h"

#include "StepGate.h"

#include <QDomElement>



namespace lmms

{



StepGateControls::StepGateControls(StepGateEffect* effect) :

    EffectControls(effect),



    // Pattern (0–3)

    m_patternModel(0.0f, 0.0f, 3.0f, 1.0f, this, tr("Pattern")),



    // Gate shaping

    m_smoothModel(0.0f, 0.0f, 1.0f, 0.01f, this, tr("Smooth")),

    m_swingModel(0.0f, 0.0f, 1.0f, 0.01f, this, tr("Swing")),

    m_speedModel(1.0f, 0.5f, 4.0f, 0.5f, this, tr("Speed")),



    // Delay parameters

    // Delay time is a *selector*, not continuous → 0,1,2,3

    m_delayTimeModel(2.0f, 0.0f, 3.0f, 1.0f, this, tr("Delay Time")), // default: 3/16 (index 2)

    m_feedbackModel(0.3f, 0.0f, 1.0f, 0.01f, this, tr("Feedback")),

    m_wetModel(0.4f, 0.0f, 1.0f, 0.01f, this, tr("Wet"))

{

    // Allocate 4 patterns of 16 steps each

    m_patterns.resize(4, std::vector<bool>(16, false));



    // Default patterns

    for(int i = 0; i < 16; ++i) m_patterns[0][i] = (i % 2 == 0);  // A: straight

    for(int i = 0; i < 16; ++i) m_patterns[1][i] = (i % 4 == 0);  // B: 4 on the floor

    for(int i = 0; i < 16; ++i) m_patterns[2][i] = true;         // C: all on

    for(int i = 0; i < 16; ++i) m_patterns[3][i] = (i % 2 != 0); // D: offbeat

}



gui::EffectControlDialog* StepGateControls::createView()

{

    return new gui::StepGateControlDialog(this);

}



void StepGateControls::setStep(int patternIdx, int stepIdx, bool active)

{

    if(patternIdx >= 0 && patternIdx < 4 && stepIdx >= 0 && stepIdx < 16)

        m_patterns[patternIdx][stepIdx] = active;

}



bool StepGateControls::getStep(int patternIdx, int stepIdx) const

{

    if(patternIdx >= 0 && patternIdx < 4 && stepIdx >= 0 && stepIdx < 16)

        return m_patterns[patternIdx][stepIdx];

    return false;

}



bool StepGateControls::getCurrentStep(int stepIdx) const

{

    int currentPat = (int)m_patternModel.value();

    return getStep(currentPat, stepIdx);

}



void StepGateControls::setRunIndex(int index)

{

    if(index != m_runIndex)

    {

        m_runIndex = index;

        emit runIndexChanged(index);

    }

}



// -----------------------------------------------------------------

// SAVE SETTINGS

// -----------------------------------------------------------------

void StepGateControls::saveSettings(QDomDocument& doc, QDomElement& parent)

{

    m_patternModel.saveSettings(doc, parent, "active_pattern");

    m_smoothModel.saveSettings(doc, parent, "smooth");

    m_swingModel.saveSettings(doc, parent, "swing");

    m_speedModel.saveSettings(doc, parent, "speed");



    m_delayTimeModel.saveSettings(doc, parent, "delay_time");

    m_feedbackModel.saveSettings(doc, parent, "delay_feedback");

    m_wetModel.saveSettings(doc, parent, "delay_wet");



    QString labels[4] = { "seqA", "seqB", "seqC", "seqD" };



    for(int p = 0; p < 4; ++p)

    {

        QString seq = "";

        for(bool stepVal : m_patterns[p])

            seq += (stepVal ? "1" : "0");

        parent.setAttribute(labels[p], seq);

    }

}



// -----------------------------------------------------------------

// LOAD SETTINGS

// -----------------------------------------------------------------

void StepGateControls::loadSettings(const QDomElement& parent)

{

    m_patternModel.loadSettings(parent, "active_pattern");

    m_smoothModel.loadSettings(parent, "smooth");

    m_swingModel.loadSettings(parent, "swing");

    m_speedModel.loadSettings(parent, "speed");



    m_delayTimeModel.loadSettings(parent, "delay_time");

    m_feedbackModel.loadSettings(parent, "delay_feedback");

    m_wetModel.loadSettings(parent, "delay_wet");



    QString labels[4] = { "seqA", "seqB", "seqC", "seqD" };



    for(int p = 0; p < 4; ++p)

    {

        if(parent.hasAttribute(labels[p]))

        {

            QString seq = parent.attribute(labels[p]);

            for(int i = 0; i < 16 && i < seq.length(); ++i)

                m_patterns[p][i] = (seq[i] == '1');

        }

    }



    emit stepsChanged();

}



} // namespace lmms

