#include "PitchShifterControlDialog.h"
#include "PitchShifterControls.h"
#include "Knob.h"
#include <QGridLayout>
#include <QLabel>

namespace lmms::gui {

PitchShifterControlDialog::PitchShifterControlDialog(PitchShifterControls* controls)
    : EffectControlDialog(controls) {

    setMinimumSize(350, 150);
    QGridLayout* layout = new QGridLayout(this);

    auto makeKnob = [this](const QString& label, const QString& unit, FloatModel* model) {
        Knob* knob = new Knob(KnobType::Bright26, label, this);
        knob->setHintText(label, unit);
        knob->setModel(model);
        return knob;
    };

    layout->addWidget(makeKnob("Pitch", "semi", &controls->m_semitoneModel), 0, 0);
    layout->addWidget(makeKnob("Fine", "cts", &controls->m_centModel), 0, 1);
    layout->addWidget(makeKnob("Grain", "ms", &controls->m_grainModel), 0, 2);
    layout->addWidget(makeKnob("Mix", "%", &controls->m_mixModel), 0, 3);

    QLabel* info = new QLabel("Real-time Granular Pitch Shifter", this);
    info->setStyleSheet("color: #aaa; font-style: italic; margin-top: 10px;");
    layout->addWidget(info, 1, 0, 1, 4, Qt::AlignCenter);
}

} // namespace lmms::gui