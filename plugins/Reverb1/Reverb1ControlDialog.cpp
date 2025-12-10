#include "Reverb1ControlDialog.h"

#include "Reverb1Controls.h"

#include "Knob.h"

#include <QGridLayout>

#include <QGroupBox>

#include <QComboBox>

#include <QLabel>



namespace lmms::gui {



Reverb1ControlDialog::Reverb1ControlDialog(Reverb1Controls* controls)

    : EffectControlDialog(controls) {



    setMinimumSize(400, 200); // Made slightly larger

    QGridLayout* mainLayout = new QGridLayout(this);



    auto makeKnob = [this](const QString& label, const QString& unit, FloatModel* model) {

        Knob* knob = new Knob(KnobType::Bright26, label, this);

        knob->setHintText(label, unit);

        knob->setModel(model);

        return knob;

    };



    // 0. PRESETS Selection

    QGroupBox* grpPreset = new QGroupBox("Presets", this);

    QGridLayout* lPreset = new QGridLayout(grpPreset);

    

    QComboBox* combo = new QComboBox(this);

    QStringList presets = {

        "Small Bathroom", "Drum Booth", "Living Room", "Small Club", "Large Club",

        "Small Hall", "Large Hall", "Church", "Cathedral", "Tunnel",

        "Gymnasium", "Plate", "Spring", "Ambient Wash", "Outer Space"

    };

    combo->addItems(presets);

    

    // Connect ComboBox to IntModel manually

    // We cannot use standard setModel for QComboBox easily in this context without a wrapper

    // So we use standard signals

    combo->setCurrentIndex(controls->m_presetModel.value());

    

    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), 

            [controls](int index){ controls->m_presetModel.setValue(index); });

            

    // If the model changes (e.g. loading project), update box

    connect(&controls->m_presetModel, &IntModel::dataChanged,

            [combo, controls](){ combo->setCurrentIndex(controls->m_presetModel.value()); });



    lPreset->addWidget(combo, 0, 0);

    mainLayout->addWidget(grpPreset, 0, 0, 1, 2); // Span top



    // 1. SPACE Section

    QGroupBox* grpSpace = new QGroupBox("Space", this);

    QGridLayout* lSpace = new QGridLayout(grpSpace);

    lSpace->addWidget(makeKnob("Size", "%", &controls->m_roomSizeModel), 0, 0);

    lSpace->addWidget(makeKnob("Damp", "%", &controls->m_dampingModel), 0, 1);

    lSpace->addWidget(makeKnob("Pre", "ms", &controls->m_preDelayModel), 0, 2);

    mainLayout->addWidget(grpSpace, 1, 0);



    // 2. MIX Section

    QGroupBox* grpMix = new QGroupBox("Mix", this);

    QGridLayout* lMix = new QGridLayout(grpMix);

    lMix->addWidget(makeKnob("Width", "%", &controls->m_widthModel), 0, 0);

    lMix->addWidget(makeKnob("Dry", "%", &controls->m_dryModel), 0, 1);

    lMix->addWidget(makeKnob("Wet", "%", &controls->m_wetModel), 0, 2);

    mainLayout->addWidget(grpMix, 1, 1);

}



} // namespace lmms::gui
