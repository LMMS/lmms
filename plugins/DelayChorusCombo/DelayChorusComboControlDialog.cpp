#include "DelayChorusComboControlDialog.h"
#include "DelayChorusComboControls.h"
#include "Knob.h"
#include <QGridLayout>

namespace lmms::gui {

DelayChorusComboControlDialog::DelayChorusComboControlDialog(DelayChorusComboControls* controls)
    : EffectControlDialog(controls) {

    setMinimumSize(400, 300); // Increased height for 3rd row
    QGridLayout* layout = new QGridLayout(this);

    auto makeKnob = [this](const QString& label, const QString& unit, FloatModel* model) {
        Knob* knob = new Knob(KnobType::Bright26, label, this);
        knob->setHintText(label, unit);
        knob->setModel(model);
        return knob;
    };

    // Row 1: LFO
    layout->addWidget(makeKnob("Rate", "Hz", &controls->m_rateModel), 0, 0);
    layout->addWidget(makeKnob("Depth", "ms", &controls->m_depthModel), 0, 1);
    layout->addWidget(makeKnob("Shape", "Sin/Tri", &controls->m_shapeModel), 0, 2); // NEW

    // Row 2: Delay & Feedback
    layout->addWidget(makeKnob("Delay", "ms", &controls->m_delayModel), 1, 0);
    layout->addWidget(makeKnob("Feed", "%", &controls->m_feedbackModel), 1, 1);
    layout->addWidget(makeKnob("Damp", "%", &controls->m_dampModel), 1, 2); // NEW

    // Row 3: Output & Stereo
    layout->addWidget(makeKnob("Phase", "Deg", &controls->m_stereoModel), 2, 0);
    layout->addWidget(makeKnob("Cross", "%", &controls->m_crossModel), 2, 1); // NEW
    layout->addWidget(makeKnob("Mix", "%", &controls->m_mixModel), 2, 2);
}

} // namespace lmms::gui