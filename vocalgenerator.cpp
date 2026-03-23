#include "vocalgenerator.h"

VocalGeneratorTab::VocalGeneratorTab(QWidget *parent) : QWidget(parent) {
    setupUI();

    connect(morphTimeSlider, &QSlider::valueChanged, this, &VocalGeneratorTab::updateLabels);
    connect(decaySlider, &QSlider::valueChanged, this, &VocalGeneratorTab::updateLabels);
    connect(vibratoRateSlider, &QSlider::valueChanged, this, &VocalGeneratorTab::updateLabels);
    connect(vibratoDepthSlider, &QSlider::valueChanged, this, &VocalGeneratorTab::updateLabels);
    connect(phaseSlider, &QSlider::valueChanged, this, &VocalGeneratorTab::updateLabels);

    connect(targetVowelCombo, &QComboBox::currentIndexChanged, this, &VocalGeneratorTab::generateExpressions);
    connect(nightlyCheckBox, &QCheckBox::toggled, this, &VocalGeneratorTab::generateExpressions);
    connect(generateButton, &QPushButton::clicked, this, &VocalGeneratorTab::generateExpressions);

    updateLabels();
    generateExpressions();
}

void VocalGeneratorTab::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *controlsGroup = new QGroupBox("Vocal Parameters", this);
    QVBoxLayout *controlsLayout = new QVBoxLayout(controlsGroup);


    QHBoxLayout *vowelLayout = new QHBoxLayout();
    vowelLayout->addWidget(new QLabel("Target Vowel (Morphs from 'Ee'):", this));
    targetVowelCombo = new QComboBox(this);
    targetVowelCombo->addItem("The 'Ahhh' Vowel (Open/Bright)");
    targetVowelCombo->addItem("The 'Ohhh' Vowel (Dark/Rounded)");
    vowelLayout->addWidget(targetVowelCombo);
    controlsLayout->addLayout(vowelLayout);


    QHBoxLayout *morphLayout = new QHBoxLayout();
    morphTimeLabel = new QLabel("Morph Time:", this);
    morphTimeSlider = new QSlider(Qt::Horizontal, this);
    morphTimeSlider->setRange(1, 40);
    morphTimeSlider->setValue(15);
    morphLayout->addWidget(morphTimeLabel);
    morphLayout->addWidget(morphTimeSlider);
    controlsLayout->addLayout(morphLayout);


    QHBoxLayout *decayLayout = new QHBoxLayout();
    decayLabel = new QLabel("Breath Decay Speed:", this);
    decaySlider = new QSlider(Qt::Horizontal, this);
    decaySlider->setRange(5, 50);
    decaySlider->setValue(20);
    decayLayout->addWidget(decayLabel);
    decayLayout->addWidget(decaySlider);
    controlsLayout->addLayout(decayLayout);


    QHBoxLayout *vRateLayout = new QHBoxLayout();
    vibratoRateLabel = new QLabel("Vibrato Rate (Hz):", this);
    vibratoRateSlider = new QSlider(Qt::Horizontal, this);
    vibratoRateSlider->setRange(20, 90);
    vibratoRateSlider->setValue(60);
    vRateLayout->addWidget(vibratoRateLabel);
    vRateLayout->addWidget(vibratoRateSlider);
    controlsLayout->addLayout(vRateLayout);


    QHBoxLayout *vDepthLayout = new QHBoxLayout();
    vibratoDepthLabel = new QLabel("Vibrato Depth (Pitch):", this);
    vibratoDepthSlider = new QSlider(Qt::Horizontal, this);
    vibratoDepthSlider->setRange(0, 30);
    vibratoDepthSlider->setValue(15);
    vDepthLayout->addWidget(vibratoDepthLabel);
    vDepthLayout->addWidget(vibratoDepthSlider);
    controlsLayout->addLayout(vDepthLayout);


    QHBoxLayout *phaseLayout = new QHBoxLayout();
    phaseLabel = new QLabel("Global Phase Offset:", this);
    phaseSlider = new QSlider(Qt::Horizontal, this);
    phaseSlider->setRange(0, 360);
    phaseSlider->setValue(0);
    phaseLayout->addWidget(phaseLabel);
    phaseLayout->addWidget(phaseSlider);
    controlsLayout->addLayout(phaseLayout);


    QHBoxLayout *btnLayout = new QHBoxLayout();
    nightlyCheckBox = new QCheckBox("Nightly Build (ExprTk - radians)", this);
    nightlyCheckBox->setChecked(true);
    generateButton = new QPushButton("Generate Vocal Expressions", this);
    btnLayout->addWidget(nightlyCheckBox);
    btnLayout->addWidget(generateButton);
    controlsLayout->addLayout(btnLayout);

    mainLayout->addWidget(controlsGroup);

    QGroupBox *outputGroup = new QGroupBox("Paste These Into LMMS Xpressive", this);
    QVBoxLayout *outputLayout = new QVBoxLayout(outputGroup);

    outputLayout->addWidget(new QLabel("W1 Field (Target Vowel Formants - Defines 1 Cycle):"));
    w1Output = new QTextEdit(this);
    w1Output->setMaximumHeight(45);
    w1Output->setReadOnly(true);
    outputLayout->addWidget(w1Output);

    outputLayout->addWidget(new QLabel("W2 Field (Start Vowel 'Ee' Formants - Defines 1 Cycle):"));
    w2Output = new QTextEdit(this);
    w2Output->setMaximumHeight(45);
    w2Output->setReadOnly(true);
    outputLayout->addWidget(w2Output);

    outputLayout->addWidget(new QLabel("O1 Field (Passes integration into W1/W2 & Morphs):"));
    o1Output = new QTextEdit(this);
    o1Output->setMaximumHeight(65);
    o1Output->setReadOnly(true);
    outputLayout->addWidget(o1Output);

    mainLayout->addWidget(outputGroup);


    QGroupBox *fxGroup = new QGroupBox("Required 90s FX Chain (Add to Mixer)", this);
    QVBoxLayout *fxLayout = new QVBoxLayout(fxGroup);
    QLabel *fxLabel = new QLabel(
        "1. <b>Bitcrusher:</b> Downsample to ~16kHz to emulate Akai S950 grit.<br>"
        "2. <b>Chorus:</b> Slow, multi-voice chorus to widen into a 'choir'.<br>"
        "3. <b>EQ:</b> High-Pass at 250Hz (remove mud), Boost at 3kHz (singer's formant).<br>"
        "4. <b>Delay & Reverb:</b> Tempo-synced ping-pong feeding into a 4.0s reverb.<br>"
        "5. <b>Sidechain:</b> Duck the reverb tail to your TR-909 Kick drum.", this);
    fxLabel->setTextFormat(Qt::RichText);
    fxLayout->addWidget(fxLabel);
    mainLayout->addWidget(fxGroup);
}

void VocalGeneratorTab::updateLabels() {
    double morph = morphTimeSlider->value() / 10.0;
    morphTimeLabel->setText(QString("Morph Time: %1s").arg(morph));
    double decay = decaySlider->value() / 10.0;
    decayLabel->setText(QString("Breath Decay Factor: %1").arg(decay));
    double rate = vibratoRateSlider->value() / 10.0;
    vibratoRateLabel->setText(QString("Vibrato Rate: %1 Hz").arg(rate));
    int depth = vibratoDepthSlider->value();
    vibratoDepthLabel->setText(QString("Vibrato Depth: +/- %1 Hz").arg(depth));
    int phase = phaseSlider->value();
    phaseLabel->setText(QString("Global Phase Offset: %1°").arg(phase));
}

QString VocalGeneratorTab::buildHarmonicSeries(double amps[], int count, bool nightly) {
    QString result;

    for (int i = 0; i < count; i++) {
        if (amps[i] == 0.0) continue;

        int harmonic = i + 1;
        QString sineCall;

        if (nightly) {
            sineCall = QString("sin(6.28318 * t * %1)").arg(harmonic);
        } else {
            sineCall = QString("sinew(t * %1)").arg(harmonic);
        }

        if (!result.isEmpty()) result += " + ";
        result += QString("%1 * %2").arg(amps[i]).arg(sineCall);
    }
    return result;
}

void VocalGeneratorTab::generateExpressions() {
    bool nightly = nightlyCheckBox->isChecked();
    double vibDepth = vibratoDepthSlider->value();
    double vibRate = vibratoRateSlider->value() / 10.0;
    double morphTime = morphTimeSlider->value() / 10.0;
    double decayFactor = decaySlider->value() / 10.0;
    double phaseOffset = phaseSlider->value();


    double w2Amps[] = {1.0, 0.15, 0.05, 0.30, 0.70, 0.50, 0.10, 0.00};
    double w1Amps[8];
    if (targetVowelCombo->currentIndex() == 0) {
        double ahAmps[] = {1.0, 0.85, 0.65, 0.15, 0.10, 0.25, 0.30, 0.10};
        std::copy(std::begin(ahAmps), std::end(ahAmps), std::begin(w1Amps));
    } else {
        double ohAmps[] = {1.0, 0.90, 0.10, 0.05, 0.05, 0.20, 0.10, 0.00};
        std::copy(std::begin(ohAmps), std::end(ohAmps), std::begin(w1Amps));
    }

    QString w1Str = buildHarmonicSeries(w1Amps, 8, nightly);
    QString w2Str = buildHarmonicSeries(w2Amps, 8, nightly);


    QString phaseStr;
    if (nightly) {
        double phaseRad = phaseOffset * 6.28318 / 360.0;
        phaseStr = QString("integrate(f + %1 * sin(6.28318 * t * %2)) + %3")
                       .arg(vibDepth).arg(vibRate).arg(phaseRad);
    } else {
        double phaseNorm = phaseOffset / 360.0;
        phaseStr = QString("integrate(f + %1 * sinew(t * %2)) + %3")
                       .arg(vibDepth).arg(vibRate).arg(phaseNorm);
    }


    QString mState = QString("clamp(0.0, t / %1, 1.0)").arg(morphTime);
    QString o1Str = QString("clamp(-1.0, (W2(%1) * (1.0 - %2) + W1(%1) * %2) * exp(-t * %3), 1.0)")
                        .arg(phaseStr)
                        .arg(mState)
                        .arg(decayFactor);


    w1Output->setText(w1Str);
    w2Output->setText(w2Str);
    o1Output->setText(o1Str);
}
