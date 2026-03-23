// =========================================================
// INCLUDES & DEPENDENCIES
// =========================================================
#include "ModularSynth.h"
#include "mainwindow.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QFile>
#include <QScrollArea>
#include <cmath>
#include <algorithm>
#include <functional>
#include <cstdlib>
#include <QRegularExpression>
#include <QProgressDialog>
#include <QtXml/QDomDocument>
#include "pcmeditortab.h"
#include "melodyrenderertab.h"
#include "effectstab.h"
#include "vocalgenerator.h"

// =========================================================
// MAIN CONSTRUCTOR
// =========================================================
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    initSamLibrary(); // InitialiSe the new SAM library
    m_ghostSynth = new SynthEngine(this);
    setupUI();
    setWindowTitle("Xpressive Sound Design Suite - Ewan Pettigrew - Uses parts of SAM phonetics code for text to speech test");


}

// =========================================================
// CUSTOM WIDGET IMPLEMENTATIONS
// =========================================================

void WaveformDisplay::updateData(const std::vector<SidSegment>& segments) {
    m_segments = segments;
    update();
}

void WaveformDisplay::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height(), midY = h / 2;
    painter.setPen(QColor(200, 200, 200, 100));
    painter.drawLine(0, midY, w, midY);
    if (m_segments.empty()) return;
    double totalDur = 0;
    for (const auto& s : m_segments) totalDur += s.duration->value();
    double xPos = 0;
    for (const auto& s : m_segments) {
        double dur = s.duration->value();
        double segWidth = (dur / (totalDur > 0 ? totalDur : 1.0)) * w;
        QString type = s.waveType->currentText();
        painter.setPen(QPen(QColor(0, 120, 215), 2));
        QPolygonF poly;
        for (int x = 0; x <= (int)segWidth; ++x) {
            double localX = (double)x / (segWidth > 0 ? segWidth : 1.0);
            double env = std::exp(-localX * dur * s.decay->value());
            double phase = x * 0.2;
            double osc = (type.contains("square") || type.contains("PWM")) ? ((std::sin(phase) > 0) ? 1 : -1) :
                             (type.contains("saw") ? (fmod(phase, 6.28) / 3.14) - 1.0 :
                                  (type.contains("triangle") ? (asin(sin(phase)) * (2.0/3.14159)) :
                                       (type.contains("randv") ? ((double)std::rand() / RAND_MAX) * 2 - 1 : std::sin(phase))));
            poly << QPointF(xPos + x, midY + (osc * env * (midY - 20)));
        }
        painter.drawPolyline(poly);
        xPos += segWidth;
    }
}
// =========================================================
// UI INITIALISATION & SETUP
// =========================================================
void MainWindow::setupUI() {
    auto *centralWidget = new QWidget(this);
    auto *mainHLayout = new QHBoxLayout(centralWidget);
    auto *sidebarGroup = new QGroupBox("Modulation & Arps");
    auto *sideScroll = new QScrollArea();
    auto *sideContent = new QWidget();
    auto *sideLayout = new QVBoxLayout(sideContent);
    QStringList mults = {"0.5x", "1x", "2x", "4x"};

    for (int i = 0; i < 5; ++i) {
        auto *mGroup = new QGroupBox(QString(i < 3 ? "Mod %1" : "PWM Mod %1").arg(i + 1));
        auto *mForm = new QFormLayout(mGroup);
        mods[i].shape = new QComboBox(); mods[i].shape->addItems({"sinew", "saww", "squarew", "trianglew"});
        mods[i].rate = new QDoubleSpinBox(); mods[i].rate->setRange(0.1, 100.0);
        mods[i].depth = new QDoubleSpinBox(); mods[i].depth->setRange(0, 1.0);
        mods[i].sync = new QCheckBox("Sync to Tempo");
        mods[i].multiplier = new QComboBox(); mods[i].multiplier->addItems(mults);
        mForm->addRow("Rate:", mods[i].rate); mForm->addRow(mods[i].sync); mForm->addRow("Mult:", mods[i].multiplier); mForm->addRow("Depth:", mods[i].depth);
        sideLayout->addWidget(mGroup);
    }
    for (int i = 0; i < 2; ++i) {
        auto *aGroup = new QGroupBox(QString("Arp %1").arg(i + 1));
        auto *aForm = new QFormLayout(aGroup);
        arps[i].wave = new QComboBox(); arps[i].wave->addItems({"squarew", "trianglew", "saww"});
        arps[i].chord = new QComboBox(); arps[i].chord->addItems({"Major", "Minor", "Dim", "Aug"});
        arps[i].speed = new QDoubleSpinBox(); arps[i].speed->setRange(1.0, 128.0); arps[i].speed->setValue(16.0);
        arps[i].sync = new QCheckBox("Sync to Tempo");
        arps[i].multiplier = new QComboBox(); arps[i].multiplier->addItems(mults);
        aForm->addRow("Wave:", arps[i].wave); aForm->addRow("Chord:", arps[i].chord); aForm->addRow("Speed:", arps[i].speed); aForm->addRow(arps[i].sync); aForm->addRow("Mult:", arps[i].multiplier);
        sideLayout->addWidget(aGroup);
    }

    chaosSlider = new QSlider(Qt::Horizontal); chaosSlider->setRange(0, 100);

    sideScroll->setWidget(sideContent); sideScroll->setWidgetResizable(true);
    auto *containerLayout = new QVBoxLayout(sidebarGroup); containerLayout->addWidget(sideScroll);
    mainHLayout->addWidget(sidebarGroup, 1);

    auto *rightLayout = new QVBoxLayout();
    modeTabs = new QTabWidget(this);
    mainHLayout->addLayout(rightLayout, 3);
    rightLayout->addWidget(modeTabs);

    // ------------------------------------
    // TAB 0: ABOUT / INFO TAB
    // ------------------------------------
    QWidget *tab00 = new QWidget();
    auto *layout00 = new QVBoxLayout(tab00);

    QTextEdit *infoText = new QTextEdit();
    infoText->setReadOnly(true);
    infoText->setHtml(
        "<h2 style='color:#2c3e50;'>Xpressive Companion</h2>"
        "<p><b>For LMMS Xpressive</b></p>"
        "<hr>"
        "<p><b>Author:</b> Ewan Pettigrew<br>"
        "<b></b> <br>"
        "<b></b> </p>"
        "<p><i>This software is 100% Freeware. Do whatever you wish with the code and results.</i></p>"
        "<br>"

        "<h3>Project Background</h3>"
        "<p>Xpressive allows users to program a synthesiser "
        "or instrument like an arbitrary waveform generator.</p>"
        "<p>Please save your project each time before pasting an expression in as bad expressions can cause crashes"
        "</p>"

        "<div style='background-color:#f0f8ff; padding:10px; border:1px solid #b0c4de;'>"
        "<b></b><br>"
        " "
        " "
        ""
        ""
        "</div>"

        "<h3>Status & Disclaimers</h3>"
        "<ul>"
        "<li><b>Status:</b> Work In Progress (WIP). Updated regularly. May contain bugs which I may be aware of.</li>"
        "<li><b>Compatibility:</b> Tested on Windows only so far.</li>"
        "<li><b>Audio Accuracy:</b> The audio previews are not working properly yet which is on the to do list.</li>"
        "</ul>"
        );

    layout00->addWidget(infoText);
    modeTabs->addTab(tab00, "00");

    // ------------------------------------
    // TAB 1: SID ARCHITECT
    // ------------------------------------
    QWidget *sidTab = new QWidget(); auto *sidLayout = new QVBoxLayout(sidTab);
    auto *sidOptGrid = new QGridLayout();
    buildModeSid = new QComboBox(); buildModeSid->addItems({"Modern", "Legacy"});
    sidOptGrid->addWidget(new QLabel("Build Mode:"), 0, 0); sidOptGrid->addWidget(buildModeSid, 0, 1);
    sidLayout->addLayout(sidOptGrid);
    waveVisualizer = new WaveformDisplay(); sidLayout->addWidget(waveVisualizer);
    auto *legend = new QLabel("LEGEND: [Wave] [Dur(s)] [Freq Off] [Decay]");
    legend->setStyleSheet("font-weight: bold; background: #eee; padding: 5px;");
    sidLayout->addWidget(legend);
    auto *scroll = new QScrollArea(); auto *scrollW = new QWidget();
    sidSegmentsLayout = new QVBoxLayout(scrollW); scroll->setWidget(scrollW); scroll->setWidgetResizable(true);
    sidLayout->addWidget(scroll);
    auto *sidBtns = new QHBoxLayout();
    auto *btnAdd = new QPushButton("Add (+)");
    auto *btnClear = new QPushButton("Clear All");
    auto *btnSaveSid = new QPushButton("Export SID Chain");
    sidBtns->addWidget(btnAdd); sidBtns->addWidget(btnClear); sidBtns->addWidget(btnSaveSid);
    sidLayout->addLayout(sidBtns);
    modeTabs->addTab(sidTab, "SID Architect");

    // ------------------------------------
    // TAB 2: PCM SAMPLER
    // ------------------------------------
    QWidget *pcmTab = new QWidget();
    auto *pcmLayout = new QVBoxLayout(pcmTab);

    // VISUALISER
    pcmScope = new UniversalScope();
    pcmLayout->addWidget(pcmScope);

    // ZOOM SLIDER
    auto *pcmZoomLay = new QHBoxLayout();
    pcmZoomLay->addWidget(new QLabel("Scope Zoom:"));
    pcmZoomSlider = new QSlider(Qt::Horizontal);
    pcmZoomSlider->setRange(0, 100);
    pcmZoomSlider->setValue(10);
    pcmZoomLay->addWidget(pcmZoomSlider);
    pcmLayout->addLayout(pcmZoomLay);

    //DISCLAIMER (The Warning)
    pcmDisclaimer = new QLabel("\n"
                               "\n"
                               "");
    pcmDisclaimer->setStyleSheet("QLabel { color: red; font-weight: bold; font-size: 14px; border: 2px solid red; padding: 10px; background-color: #ffeeee; }");
    pcmDisclaimer->setAlignment(Qt::AlignCenter);
    pcmDisclaimer->setFixedHeight(80);
    pcmLayout->addWidget(pcmDisclaimer);



    // CONTROLS
        auto *btnLoad = new QPushButton("Load WAV");
        btnSave = new QPushButton("Generate String");
        btnCopy = new QPushButton("Copy Clipboard");
        pcmLayout->addWidget(btnLoad);
        pcmLayout->addWidget(btnSave);
        pcmLayout->addWidget(btnCopy);

        auto *pcmGrid = new QGridLayout();
        buildModeCombo = new QComboBox(); buildModeCombo->addItems({"Modern", "Legacy"});


        sampleRateCombo = new QComboBox(); sampleRateCombo->addItems({"16000", "8000", "4000", "2000"});


        bitDepthCombo = new QComboBox(); bitDepthCombo->addItems({"4-bit (Gritty)", "8-bit (Clean)", "Uncompressed (Raw)"});

        maxDurSpin = new QDoubleSpinBox(); maxDurSpin->setRange(0.01, 600.0); maxDurSpin->setValue(2.0);


        normalizeCheck = new QCheckBox("Normalize Audio"); normalizeCheck->setChecked(true);

        pcmGrid->addWidget(new QLabel("Build Mode:"), 0, 0); pcmGrid->addWidget(buildModeCombo, 0, 1);
        pcmGrid->addWidget(new QLabel("Rate:"), 1, 0); pcmGrid->addWidget(sampleRateCombo, 1, 1);
        pcmGrid->addWidget(new QLabel("Bit Depth:"), 2, 0); pcmGrid->addWidget(bitDepthCombo, 2, 1);
        pcmGrid->addWidget(new QLabel("Max(s):"), 3, 0); pcmGrid->addWidget(maxDurSpin, 3, 1);
        pcmLayout->addLayout(pcmGrid);
        pcmLayout->addWidget(normalizeCheck);

        modeTabs->addTab(pcmTab, "PCM Sampler");

    // SCOPE ZOOM LOGIC
    connect(pcmZoomSlider, &QSlider::valueChanged, [=]() {
        if(originalData.empty()) return;
        double dur = (double)originalData.size() / fileFs;
        double zoom = pcmZoomSlider->value() / 100.0;
        pcmScope->updateScope([=](double t){
            int idx = (int)(t * fileFs);
            if(idx >= 0 && idx < originalData.size()) return originalData[idx];
            return 0.0;
        }, dur, zoom);
    });

    // ------------------------------------
    // TAB 3 : CONSOLE LAB
    // ------------------------------------
    QWidget *consoleTab = new QWidget();
    auto *consoleLayout = new QVBoxLayout(consoleTab);

    //OSCILLOSCOPE
    consoleScope = new UniversalScope();
    consoleScope->setMinimumHeight(150);
    consoleLayout->addWidget(consoleScope);

    //CONTROLS
    auto *cForm = new QFormLayout();
    buildModeConsole = new QComboBox(); buildModeConsole->addItems({"Modern", "Legacy"});
    consoleWaveType = new QComboBox();
    consoleWaveType->addItems({
        "00. NES Triangle (Pseudo-Tri)",
        "01. NES Pulse 12.5% (Thin)",
        "02. NES Pulse 25% (Classic)",
        "03. NES Pulse 50% (Square)",
        "04. Gameboy Soft Saw (No DC)",
        "05. Gameboy Noise (Polynomial)",
        "06. C64 Sawtooth (Ramp)",
        "07. C64 Triangle (Analogue)",
        "08. C64 Pulse (Width Sweep)",
        "09. C64 Ring Mod (Bell)",
        "10. VRC6 Sawtooth (Expanded)",
        "11. VRC6 Pulse (Duty Cycle)",
        "12. Famicom Disk System (Mod)",
        "13. Master System Tone (Flat)",
        "14. PC Engine Organ (50/50)",
        "15. 2600 Pitched Noise",
        "16. 2600 Pure Square",
        "17. Konami SCC (Wavetable Sim)",
        "18. Namco 163 (4-bit Sine)",
        "19. Sega Genesis (FM Bass)",
        "20. Sega Genesis (FM Bell)",
        "21. Amiga 500 (Sample Hold)"
    });

    consoleSteps = new QDoubleSpinBox(); consoleSteps->setValue(16);

    cForm->addRow("Build Mode:", buildModeConsole);
    cForm->addRow("Type:", consoleWaveType);
    cForm->addRow("Quantize Steps:", consoleSteps);
    consoleLayout->addLayout(cForm);

    //PLAY / GENERATE BUTTONS
    auto *cBtnLayout = new QHBoxLayout();

    auto *btnPlayConsole = new QPushButton("▶ Play A-5 (880Hz)");
    btnPlayConsole->setCheckable(true);
    btnPlayConsole->setStyleSheet("background-color: #335533; color: white; font-weight: bold;");

    auto *btnGenConsole = new QPushButton("Generate String");

    cBtnLayout->addWidget(btnPlayConsole);
    cBtnLayout->addWidget(btnGenConsole);
    consoleLayout->addLayout(cBtnLayout);

    consoleLayout->addStretch();
    modeTabs->addTab(consoleTab, "Console Lab");

    //LOGIC EXTRACTION
    auto updateConsole = [=]() {
        int type = consoleWaveType->currentIndex();
        double steps = consoleSteps->value();
        double f = 880.0; // A-5 Frequency

        // THE MATH ROUTINE
        std::function<double(double)> audioRoutine = [=](double t) {
            double signal = 0.0;
            double pi = 3.14159;
            double ph = t * f; // Basic Phase

            //Pulse Width
            auto pulse = [&](double w) { return std::sin(2.0*pi*ph) > w ? 1.0 : -1.0; };
            auto tri = [&]() { return (2.0/pi)*std::asin(std::sin(2.0*pi*ph)); };
            auto saw = [&]() { return 2.0 * (ph - std::floor(0.5 + ph)); };

            switch(type) {
            case 0: signal = tri(); break; // NES Tri
            case 1: signal = pulse(0.75); break; // 12.5%
            case 2: signal = pulse(0.5); break;  // 25%
            case 3: signal = pulse(0.0); break;  // 50%
            case 4: signal = saw() * 0.8; break; // Soft Saw
            case 5: signal = ((std::rand() % 100) / 50.0) - 1.0; break; // Noise
            case 6: signal = saw(); break; // Sharp Saw
            case 7: signal = tri() + (0.1 * saw()); break; // Dirty Tri
            case 8: signal = pulse(std::sin(t * 3.0)); break; // PWM Sweep
            case 9: signal = pulse(0.0) * pulse(0.95); break; // Ring Mod
            case 10: signal = (saw() * 0.5) + (pulse(0.0) * 0.5); break; // VRC Saw
            case 11: signal = pulse(0.8); break; // VRC Pulse
            case 12: signal = std::sin(2.0*pi*ph + std::sin(2.0*pi*t*6.0)); break; // FDS
            case 13: signal = pulse(-0.2); break; // Master System
            case 14: signal = (pulse(0.0) + pulse(0.1)) * 0.5; break; // PC Engine
            case 15: signal = ((std::rand()%100 > 50 ? 1 : -1) * pulse(0.0)); break; // 2600
            case 16: signal = pulse(0.0); break; // 2600 Square
            case 17: signal = saw() * (tri() > 0 ? 1 : -1); break; // SCC
            case 18: signal = std::sin(2.0*pi*ph); break; // Namco Sine
            case 19: signal = std::sin(2.0*pi*ph + 3.0*std::sin(2.0*pi*ph*0.5)); break; // Genesis Bass
            case 20: signal = std::sin(2.0*pi*ph) * std::exp(-t*10.0); break; // Genesis Bell
            case 21: signal = saw(); break; // Amiga
            }

            // Bit-Crush
            if (steps > 0) signal = std::floor(signal * steps) / steps;

            return signal;
        };

        //Update VisualiSer
        // Slower "fake" frequency for the scope so the wave shape is visible
        std::function<double(double)> visRoutine = [=](double t) {
            double ratio = 80.0 / f; // Scale 880Hz down to 80Hz for viewing
            return audioRoutine(t * ratio);
        };
        consoleScope->updateScope(visRoutine, 1.0, 0.1);

        // Update Audio (Only if playing)
        if(btnPlayConsole->isChecked()) {
            m_ghostSynth->setAudioSource(audioRoutine);
        }
    };

    // CONNECTIONS
    connect(consoleWaveType, QOverload<int>::of(&QComboBox::currentIndexChanged), updateConsole);
    connect(consoleSteps, &QDoubleSpinBox::valueChanged, updateConsole);

    // PLAY BUTTON LOGIC
    connect(btnPlayConsole, &QPushButton::toggled, [=](bool checked){
        if(!checked) {
            m_ghostSynth->setAudioSource([](double){ return 0.0; });
            m_ghostSynth->stop();
            btnPlayConsole->setText("▶ Play A-5 (880Hz)");
            btnPlayConsole->setStyleSheet("background-color: #335533; color: white;");
        } else {
            btnPlayConsole->setText("⏹ Stop");
            btnPlayConsole->setStyleSheet("background-color: #338833; color: white;");
            m_ghostSynth->start();
            // Trigger the update logic to start the sound
            updateConsole();
        }
    });

    connect(btnGenConsole, &QPushButton::clicked, this, &MainWindow::generateConsoleWave);

    // Initial Trigger to draw the default wave
    QTimer::singleShot(100, updateConsole);

    // ------------------------------------
    // TAB 4: SFX MACRO
    // ------------------------------------
    QWidget *sfxTab = new QWidget();
    auto *sfxLayout = new QVBoxLayout(sfxTab);

    // OSCILLOSCOPE
    sfxScope = new UniversalScope();
    sfxScope->setMinimumHeight(150);
    sfxLayout->addWidget(sfxScope);

    // CONTROLS
    auto *sfxForm = new QFormLayout();
    buildModeSFX = new QComboBox(); buildModeSFX->addItems({"Modern", "Legacy"});
    sfxStartFreq = new QDoubleSpinBox(); sfxStartFreq->setRange(20, 20000); sfxStartFreq->setValue(880);
    sfxEndFreq = new QDoubleSpinBox(); sfxEndFreq->setRange(20, 20000); sfxEndFreq->setValue(110);
    sfxDur = new QDoubleSpinBox(); sfxDur->setValue(0.2);

    sfxForm->addRow("Build Mode:", buildModeSFX);
    sfxForm->addRow("Start (Hz):", sfxStartFreq);
    sfxForm->addRow("End (Hz):", sfxEndFreq);
    sfxForm->addRow("Dur (s):", sfxDur);
    sfxLayout->addLayout(sfxForm);

    // PLAY / GENERATE BUTTONS
    auto *sfxBtnLay = new QHBoxLayout();

    auto *btnPlaySFX = new QPushButton("▶ Play SFX Loop");
    btnPlaySFX->setCheckable(true);
    btnPlaySFX->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");

    auto *btnGenSFX = new QPushButton("Generate SFX String");
    btnGenSFX->setStyleSheet("height: 40px;");

    sfxBtnLay->addWidget(btnPlaySFX);
    sfxBtnLay->addWidget(btnGenSFX);
    sfxLayout->addLayout(sfxBtnLay);
    sfxLayout->addStretch();

    modeTabs->addTab(sfxTab, "SFX Macro");

    auto updateSFX = [=]() {
        double fStart = sfxStartFreq->value();
        double fEnd   = sfxEndFreq->value();
        double dur    = sfxDur->value();

        std::function<double(double)> sfxAlgo = [=](double t) {
            // Silence after duration
            if (t > dur) return 0.0;
            if (t < 0) return 0.0;
            if (dur <= 0.001) return 0.0;

            // Calculate 'k' for the exponential curve: f(t) = fStart * exp(k*t)
            // If fEnd < fStart, k will be negative (decay)
            double k = std::log(fEnd / fStart) / dur;

            // Handle constant pitch (k ~= 0) to avoid divide-by-zero
            if (std::abs(k) < 0.0001) {
                return std::sin(t * fStart * 2.0 * 3.14159);
            }

            // Integral of f(t) is (1/k) * f(t)
            double phase = (fStart / k) * (std::exp(k * t) - 1.0);
            return std::sin(phase * 2.0 * 3.14159);
        };

        // Update Scope
        sfxScope->updateScope(sfxAlgo, dur * 1.2, 1.0);

        // Update Audio (If Playing)
        if (btnPlaySFX->isChecked()) {
            // Create a loop with a small silence gap so you can hear the attack again
            double loopLen = dur + 0.5;
            m_ghostSynth->setAudioSource([=](double t){
                return sfxAlgo(std::fmod(t, loopLen));
            });
        }
    };

    // CONNECTIONS
    connect(sfxStartFreq, &QDoubleSpinBox::valueChanged, updateSFX);
    connect(sfxEndFreq, &QDoubleSpinBox::valueChanged, updateSFX);
    connect(sfxDur, &QDoubleSpinBox::valueChanged, updateSFX);

    connect(btnPlaySFX, &QPushButton::toggled, [=](bool checked){
        if(!checked) {
            m_ghostSynth->setAudioSource([](double){ return 0.0; });
            m_ghostSynth->stop();
            btnPlaySFX->setText("▶ Play SFX Loop");
            btnPlaySFX->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");
        } else {
            m_ghostSynth->start();
            btnPlaySFX->setText("⏹ Stop");
            btnPlaySFX->setStyleSheet("background-color: #338833; color: white; font-weight: bold; height: 40px;");
            updateSFX(); // Trigger the sound immediately
        }
    });

    connect(btnGenSFX, &QPushButton::clicked, this, &MainWindow::generateSFXMacro);

    QTimer::singleShot(200, updateSFX);

    // ------------------------------------
    // TAB 5: ARP ANIMATOR
    // ------------------------------------
    QWidget *arpTab = new QWidget();
    auto *arpLayout = new QVBoxLayout(arpTab);

    // VISUALISER
    arpScope = new UniversalScope();
    arpScope->setMinimumHeight(150);
    arpLayout->addWidget(arpScope);

    // OSCILLATOR
    auto *oscGroup = new QGroupBox("SID Oscillator");
    auto *oscForm = new QFormLayout(oscGroup);

    buildModeArp = new QComboBox();
    buildModeArp->addItems({"Nightly (Nested - Clean)", "Legacy (Additive)"});

    arpWave = new QComboBox();
    arpWave->addItems({"Pulse (Classic)", "Sawtooth", "Triangle", "Noise (Percussion)", "Metal (Ring Mod)"});

    arpPwmSlider = new QSlider(Qt::Horizontal);
    arpPwmSlider->setRange(1, 99);
    arpPwmSlider->setValue(50); // Square wave default

    oscForm->addRow("Build Mode:", buildModeArp);
    oscForm->addRow("Waveform:", arpWave);
    oscForm->addRow("Pulse Width:", arpPwmSlider);

    arpLayout->addWidget(oscGroup);

    // THE CHORD
    auto *seqGroup = new QGroupBox("Chord Sequence (0 -> Step 2 -> Step 3)");
    auto *seqForm = new QFormLayout(seqGroup);

    QStringList intervals = {
            "0 (Root)", "+3 (Minor 3rd)", "+4 (Major 3rd)", "+5 (4th)",
            "+7 (Perfect 5th)", "+12 (Octave)", "-12 (Sub Octave)",
            "+19 (Octave+5th)", "+24 (2 Octaves)"
    };

    arpInterval1 = new QComboBox(); arpInterval1->addItems(intervals);
    arpInterval1->setCurrentIndex(2); // Major 3rd default

    arpInterval2 = new QComboBox(); arpInterval2->addItems(intervals);
    arpInterval2->setCurrentIndex(4); // Perfect 5th default

    seqForm->addRow("Step 2 Note:", arpInterval1);
    seqForm->addRow("Step 3 Note:", arpInterval2);

    arpLayout->addWidget(seqGroup);

    //SPEED & SYNC ---
    auto *spdGroup = new QGroupBox("Speed / Tempo");
    auto *spdForm = new QFormLayout(spdGroup);

    arpBpmSync = new QCheckBox("Sync to BPM");
    arpBpmSync->setChecked(true);

    arpBpmVal = new QDoubleSpinBox();
    arpBpmVal->setRange(40, 300);
    arpBpmVal->setValue(125); // Classic Techno/Chiptune tempo

    arpSpeedDiv = new QComboBox();
    arpSpeedDiv->addItems({"1/16 (Standard)", "1/32 (Fast)", "1/48 (Triplets)", "1/64 (Hubbard Speed)", "50Hz (PAL Frame)"});
    arpSpeedDiv->setCurrentIndex(3); // Default to "Hubbard Speed"


    arpSpeed = new QDoubleSpinBox();
    arpSpeed->setRange(0.1, 1000);
    arpSpeed->setValue(50);
    arpSpeed->setVisible(false);

    spdForm->addRow(arpBpmSync);
    spdForm->addRow("Song BPM:", arpBpmVal);
    spdForm->addRow("Grid Size:", arpSpeedDiv);
    spdForm->addRow("Manual Hz:", arpSpeed);

    arpLayout->addWidget(spdGroup);

    // BUTTONS LAYOUT
    auto *arpBtnLay = new QHBoxLayout();

    btnPlayArp = new QPushButton("▶ Play Arp");
    btnPlayArp->setCheckable(true);
    btnPlayArp->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");

    auto *btnGenArp = new QPushButton("GENERATE C64 ARP");
    btnGenArp->setStyleSheet("font-weight: bold; background-color: #444; color: white; height: 40px;");

    arpBtnLay->addWidget(btnPlayArp);
    arpBtnLay->addWidget(btnGenArp);
    arpLayout->addLayout(arpBtnLay);

    // LOGIC TO TOGGLE INPUTS
    connect(arpBpmSync, &QCheckBox::toggled, [=](bool checked){
            arpBpmVal->setVisible(checked);
            arpSpeedDiv->setVisible(checked);
            arpSpeed->setVisible(!checked);
    });

    modeTabs->addTab(arpTab, "Arp Animator");

    // LIVE PREVIEW LOGIC
    auto updateArpPreview = [=]() {
            // Calculate Frequency (Speed)
            double hz = 0;
            if (arpBpmSync->isChecked()) {
                double bpm = arpBpmVal->value();
                int divIdx = arpSpeedDiv->currentIndex();
                if (divIdx == 4) hz = 50.0; // PAL Frame
                else {
                    double multiplier = (divIdx == 0) ? 4.0 : (divIdx == 1) ? 8.0 : (divIdx == 2) ? 12.0 : 16.0;
                    hz = (bpm / 60.0) * multiplier;
                }
            } else {
                hz = arpSpeed->value();
        }

        // Get Intervals
        auto getSemi = [](QString s) { return s.split(" ")[0].toInt(); };
        int semi1 = 0;
        int semi2 = getSemi(arpInterval1->currentText());
        int semi3 = getSemi(arpInterval2->currentText());

        double p1 = 1.0;
        double p2 = std::pow(2.0, semi2 / 12.0);
        double p3 = std::pow(2.0, semi3 / 12.0);

        // Get Waveform Parameters
        int wType = arpWave->currentIndex(); // 0=Pulse, 1=Saw, 2=Tri, 3=Noise, 4=Metal
        double pwm = (arpPwmSlider->value() / 100.0) * 2.0 - 1.0; // Map -1 to 1

        // Build Lambda
        std::function<double(double)> arpFunc = [=](double t) {
            // Determine Step (0, 1, 2)
            // Using a slightly offset time to ensure step 0 starts immediately
            int step = (int)(t * hz) % 3;
            double mult = (step == 0) ? p1 : (step == 1 ? p2 : p3);

            double f = 220.0; // Preview Pitch (A3)
            double pi = 3.14159;
            double ph = t * f * mult; // Phase

            // Oscillators
            if (wType == 0) { // Pulse
                return (std::sin(ph * 2 * pi) > pwm) ? 1.0 : -1.0;
            }
            else if (wType == 1) { // Saw
                double x = std::fmod(ph, 1.0);
                return 2.0 * x - 1.0;
            }
            else if (wType == 2) { // Triangle
                return (2.0 / pi) * std::asin(std::sin(ph * 2 * pi));
            }
            else if (wType == 3) { // Noise
                return ((double)std::rand() / RAND_MAX) * 2.0 - 1.0;
            }
            else { // Metal (Ring Mod)
                // Square * Detuned Square (approx 2.41 ratio)
                double sq1 = (std::sin(ph * 2 * pi) > 0) ? 1.0 : -1.0;
                double sq2 = (std::sin(ph * 2.41 * 2 * pi) > 0) ? 1.0 : -1.0;
                return sq1 * sq2;
            }
        };

        // Update Scope (Show 4 beats worth or enough to see pattern)
        double viewDur = (hz > 0) ? (4.0 / hz) : 1.0;
        arpScope->updateScope(arpFunc, viewDur, 1.0);

        // Update Audio
        if(btnPlayArp->isChecked()) {
             m_ghostSynth->setAudioSource(arpFunc);
        }
    };

    // --- CONNECTIONS ---
    connect(btnGenArp, &QPushButton::clicked, this, &MainWindow::generateArpAnimator);

    // Connect Play Button
    connect(btnPlayArp, &QPushButton::toggled, [=](bool checked){
        if(!checked) {
            m_ghostSynth->setAudioSource([](double){ return 0.0; });
            m_ghostSynth->stop();
            btnPlayArp->setText("▶ Play Arp");
            btnPlayArp->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");
        } else {
            m_ghostSynth->start();
            btnPlayArp->setText("⏹ Stop");
            btnPlayArp->setStyleSheet("background-color: #338833; color: white; font-weight: bold; height: 40px;");
            updateArpPreview();
        }
    });

    // Connect Inputs to Update Logic
    QList<QWidget*> arpWidgets = {
        arpWave, arpInterval1, arpInterval2, arpSpeedDiv, arpBpmSync,
        arpBpmVal, arpSpeed, arpPwmSlider
    };
    for(auto *w : arpWidgets) {
        if(auto *cb = qobject_cast<QComboBox*>(w)) connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), updateArpPreview);
        if(auto *sb = qobject_cast<QDoubleSpinBox*>(w)) connect(sb, &QDoubleSpinBox::valueChanged, updateArpPreview);
        if(auto *sl = qobject_cast<QSlider*>(w)) connect(sl, &QSlider::valueChanged, updateArpPreview);
        if(auto *chk = qobject_cast<QCheckBox*>(w)) connect(chk, &QCheckBox::toggled, updateArpPreview);
    }

    // Initial Trigger
    QTimer::singleShot(200, updateArpPreview);

    // ------------------------------------
    // TAB 6: WAVETABLE FORGE
    // ------------------------------------
    QWidget *wtTab = new QWidget();
    auto *wtLayout = new QVBoxLayout(wtTab);

    // SCOPE
    wtScope = new UniversalScope();
    wtScope->setMinimumHeight(150);
    wtLayout->addWidget(wtScope);

    // HEADER
    auto *wtHeader = new QHBoxLayout();
    wtHeader->addWidget(new QLabel("Build Mode:"));
    buildModeWavetable = new QComboBox();
    buildModeWavetable->addItems({"Nightly (Nested)", "Legacy (Additive)"});
    wtHeader->addWidget(buildModeWavetable);

    wtHeader->addWidget(new QLabel("| Master Library:"));
    wtPresetCombo = new QComboBox();
    QStringList presets;
    presets << "--- INIT ---" << "00. Empty / Init";
    presets << "--- ROB HUBBARD ---" << "01. Commando Bass (Glissando)" << "02. Monty Lead (Pulse+Vib)" << "03. Delta Snare (Tri-Noise)" << "04. Zoids Metal (Ring Mod)" << "05. Ace 2 Kick (Deep)" << "06. Crazy Comets (Echo)";
    presets << "--- MARTIN GALWAY ---" << "07. Wizball Arp (Bubble)" << "08. Parallax Bass (Slap)" << "09. Comic Bakery (Lead)" << "10. Arkanoid (Dotted Echo)" << "11. Green Beret (Military Snare)";
    presets << "--- JEROEN TEL ---" << "12. Cybernoid Metal Drum" << "13. Supremacy Lead (Vibrato)" << "14. Turbo Outrun (Bass)" << "15. RoboCop 3 (Title Arp)";
    presets << "--- CHRIS HUELSBECK ---" << "16. Turrican I (Huge Arp)" << "17. Turrican II (Pad)" << "18. Katakis (Space Lead)" << "19. Great Giana (Bass)";
    presets << "--- TIM FOLLIN ---" << "20. Solstice (Intro Lead)" << "21. Ghouls'n'Ghosts (Rain)" << "22. Silver Surfer (Arp)" << "23. LED Storm (Bass)";
    presets << "--- BEN DAGLISH ---" << "24. Last Ninja (Dark Bass)" << "25. Deflektor (Lead)" << "26. Trap (Fast Arp)";
    presets << "--- DAVID WHITTAKER ---" << "27. Glider Rider (Square)" << "28. Lazy Jones (Laser)";
    presets << "--- YM / ATARI ST MASTERS ---" << "29. YM Buzzer Envelope" << "30. YM Metal Bass" << "31. YM 3-Voice Chord" << "32. Digi-Drum (SID-Style)";
    presets << "--- FX / DRUMS (Utility) ---" << "33. Coin (Mario Style)" << "34. Power Up" << "35. Explosion (Noise Decay)" << "36. Laser (Pew Pew)" << "37. 8-Bit Hi-Hat (Closed)" << "38. 8-Bit Hi-Hat (Open)" << "39. Fake Chord (Minor)" << "40. Fake Chord (Major)" << "--- SID DRUMS EXPANSION ---" << "41. Heavy SID Kick (Square Drop)" << "42. Snappy Snare (Tri+Noise)" << "43. Tech Kick (Metal+Pulse)" << "44. Glitch Snare (Ring Mod)";
    wtPresetCombo->addItems(presets);
    wtHeader->addWidget(wtPresetCombo);

    wtLoopCheck = new QCheckBox("Loop Sequence");
    wtHeader->addWidget(wtLoopCheck);
    wtLayout->addLayout(wtHeader);

    // TABLE
    wtTrackerTable = new QTableWidget();
    wtTrackerTable->setColumnCount(4);
    wtTrackerTable->setHorizontalHeaderLabels({"Waveform", "Pitch (+/-)", "PWM %", "Dur (s)"});
    wtTrackerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    wtTrackerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    wtLayout->addWidget(wtTrackerTable);

    // BUTTONS
    auto *wtBtnLayout = new QHBoxLayout();
    auto *btnAddStep = new QPushButton("Add Step (+)");
    auto *btnRemStep = new QPushButton("Remove Step (-)");
    auto *btnPlayWT = new QPushButton("▶ Play Sequence");
    btnPlayWT->setCheckable(true);
    btnPlayWT->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");
    auto *btnGenWT = new QPushButton("GENERATE SEQUENCE");
    btnGenWT->setStyleSheet("font-weight: bold; background-color: #444; color: white; height: 40px;");

    wtBtnLayout->addWidget(btnPlayWT);
    wtBtnLayout->addWidget(btnAddStep);
    wtBtnLayout->addWidget(btnRemStep);
    wtLayout->addLayout(wtBtnLayout);
    wtLayout->addWidget(btnGenWT);
    modeTabs->addTab(wtTab, "Wavetable Forge");

    // UPDATE LOGIC
    auto updateWavetable = [=]() {
        int rows = wtTrackerTable->rowCount();
        if (rows == 0) return;

        struct StepData { int type; double pitchMult; double pwm; double duration; };
        std::vector<StepData> sequence;
        double totalDur = 0;

        for(int i=0; i<rows; ++i) {
            if (!wtTrackerTable->item(i, 0) || !wtTrackerTable->item(i, 1) ||
                !wtTrackerTable->item(i, 2) || !wtTrackerTable->item(i, 3))
                continue;

            QString tStr = wtTrackerTable->item(i, 0)->text().toLower();
            int pVal = wtTrackerTable->item(i, 1)->text().toInt();
            double pwmVal = wtTrackerTable->item(i, 2)->text().toDouble() / 100.0;
            double durVal = wtTrackerTable->item(i, 3)->text().toDouble();

            totalDur += durVal;

            int type = 0;
            if (tStr.contains("tri") && tStr.contains("noise")) type = 5;
            else if (tStr.contains("tri")) type = 1;
            else if (tStr.contains("pulse")) type = 2;
            else if (tStr.contains("noise")) type = 3;
            else if (tStr.contains("metal")) type = 4;
            else if (tStr.contains("sync")) type = 6;

            sequence.push_back({type, std::pow(2.0, pVal/12.0), pwmVal, durVal});
        }

        bool loop = wtLoopCheck->isChecked();

        // Audio Function
        std::function<double(double)> audioRoutine = [=](double t) {
            double localT = t;
            if (loop && totalDur > 0) localT = std::fmod(t, totalDur);
            else if (t > totalDur) return 0.0;

            double scanT = 0;
            int activeStep = -1;
            for(size_t i=0; i<sequence.size(); ++i) {
                if (localT >= scanT && localT < (scanT + sequence[i].duration)) {
                    activeStep = i; break;
                }
                scanT += sequence[i].duration;
            }
            if (activeStep == -1) return 0.0;

            const StepData& s = sequence[activeStep];
            double f = 440.0; double pi = 3.14159;
            double ph = localT * f * s.pitchMult;

            auto saw = [](double x) { return 2.0 * (x - std::floor(0.5 + x)); };
            auto tri = [&](double x) { return (2.0/pi)*std::asin(std::sin(2.0*pi*x)); };
            auto pulse = [&](double x, double w) { return std::sin(2.0*pi*x) > w ? 1.0 : -1.0; };
            // Thread-Safe Noise
            auto noise = [](double x) { return std::sin(x * 43758.5453) * std::sin(x * 12.9898); };

            switch(s.type) {
            case 0: return saw(ph);
            case 1: return tri(ph);
            case 2: return pulse(ph, (s.pwm * 2.0) - 1.0);
            case 3: return noise(localT * 10000.0);
            case 4: return pulse(ph, 0.0) * pulse(ph * 2.41, 0.0);
            case 5: return tri(ph) + (0.5 * noise(localT * 10000.0));
            case 6: return saw(ph) * saw(ph * 0.5);
            }
            return 0.0;
        };

        wtScope->updateScope(audioRoutine, totalDur > 0 ? totalDur : 1.0, 1.0);
        if(btnPlayWT->isChecked()) {
            m_ghostSynth->setAudioSource(audioRoutine);
        }
    };

    // CONNECTIONS
    connect(wtPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int i){
        loadWavetablePreset(i);
        // Force the scope to update with the new data
        updateWavetable();
    });

    connect(btnGenWT, &QPushButton::clicked, this, &MainWindow::generateWavetableForge);
    connect(wtTrackerTable, &QTableWidget::cellChanged, updateWavetable);

    connect(btnPlayWT, &QPushButton::toggled, [=](bool checked){
        if(!checked) {
            m_ghostSynth->setAudioSource([](double){ return 0.0; });
            m_ghostSynth->stop();
            btnPlayWT->setText("▶ Play Sequence");
            btnPlayWT->setStyleSheet("background-color: #335533; color: white;");
        } else {
            m_ghostSynth->start();
            btnPlayWT->setText("⏹ Stop");
            btnPlayWT->setStyleSheet("background-color: #338833; color: white;");
            updateWavetable();
        }
    });

    connect(btnAddStep, &QPushButton::clicked, [=](){
        int row = wtTrackerTable->rowCount();
        wtTrackerTable->insertRow(row);
        wtTrackerTable->setItem(row, 0, new QTableWidgetItem("Saw"));
        wtTrackerTable->setItem(row, 1, new QTableWidgetItem("0"));
        wtTrackerTable->setItem(row, 2, new QTableWidgetItem("50"));
        wtTrackerTable->setItem(row, 3, new QTableWidgetItem("0.05"));
        updateWavetable();
    });

    connect(btnRemStep, &QPushButton::clicked, [=](){
        if (wtTrackerTable->rowCount() > 0) {
            wtTrackerTable->removeRow(wtTrackerTable->rowCount()-1);
            updateWavetable();
        }
    });

    // Initial Load
    loadWavetablePreset(1);
    QTimer::singleShot(200, updateWavetable);

    // ------------------------------------
    // TAB 7: BESSEL FM
    // ------------------------------------
    QWidget *besselTab = new QWidget(); auto *besselLayout = new QVBoxLayout(besselTab);
    besselSpectrum = new UniversalSpectrum();
    besselLayout->addWidget(besselSpectrum);
    auto *bForm = new QFormLayout();
    buildModeBessel = new QComboBox(); buildModeBessel->addItems({"Modern", "Legacy"});
    besselPresetCombo = new QComboBox();
    besselPresetCombo->addItems({
        "-- CATEGORY: KEYS --", "01. DX7 Electric Piano", "02. Glass Tines", "03. Dig-it-al Harp", "04. 80s FM Organ", "05. Toy Piano", "06. Celestial Keys", "07. Polished Brass", "08. Log Drum Keys",
        "-- CATEGORY: BASS --", "09. LatelyBass (TX81Z)", "10. Solid Bass", "11. Rubber Bass", "12. Wood Bass", "13. Slap FM Bass", "14. Sub-Thump", "15. Metallic Drone", "16. Acid FM",
        "-- CATEGORY: BELLS/PERC --", "17. Tubular Bells", "18. Gamelan", "19. Marimba FM", "20. Cowbell (808 style)", "21. FM Snare Crack", "22. Metallic Tom", "23. Wind Chimes", "24. Ice Bell", "25. Church Bell",
        "-- CATEGORY: PADS/LEAD --", "26. Arctic Pad", "27. FM Flute", "28. Oboe-ish", "29. Sync-Lead FM", "30. Space Reed", "31. Harmonic Swell", "32. Thin Pulse Lead", "33. Bottle Blow",
        "-- CATEGORY: FX/NOISE --", "34. Laser Harp", "35. Sci-Fi Computer", "36. Industrial Clang", "37. Digital Rain", "38. Alarm Pulse", "39. Glitch Burst", "40. 8-Bit Explosion"
    });
    besselCarrierWave = new QComboBox(); besselCarrierWave->addItems({"sinew", "saww", "squarew", "trianglew"});
    besselModWave = new QComboBox(); besselModWave->addItems({"sinew", "saww", "squarew", "trianglew"});
    besselCarrierMult = new QDoubleSpinBox(); besselCarrierMult->setRange(0, 64); besselCarrierMult->setValue(1.0);
    besselModMult = new QDoubleSpinBox(); besselModMult->setRange(0, 64); besselModMult->setValue(2.0);
    besselModIndex = new QDoubleSpinBox(); besselModIndex->setRange(0, 100); besselModIndex->setValue(2.0);
    bForm->addRow("Build Mode:", buildModeBessel);
    bForm->addRow("80s LIBRARY:", besselPresetCombo);
    bForm->addRow("Carrier Wave:", besselCarrierWave);
    bForm->addRow("Carrier Mult (C):", besselCarrierMult);
    bForm->addRow("Modulator Wave:", besselModWave);
    bForm->addRow("Modulator Mult (M):", besselModMult);
    bForm->addRow("Mod Index (I):", besselModIndex);
    besselLayout->addLayout(bForm);
    auto *btnBes = new QPushButton("Generate Bessel FM");
    besselLayout->addWidget(btnBes);

    // FM math for the visualiser
    auto updateBesselVisual = [=]() {
        if (!besselSpectrum) return;

        // Get values from the UI
        double cMult = besselCarrierMult->value();
        double mMult = besselModMult->value();
        double index = besselModIndex->value();

        // Define the Math (Standard FM Synthesis)
        // t = time, f = frequency (we use 220Hz for visualization)
        std::function<double(double)> fmAlgo = [=](double t) {
            double f = 220.0;
            double twoPi = 6.283185;

            // Simple Sine FM: sin( c*t + I*sin(m*t) )
            double modulator = index * std::sin(t * f * mMult * twoPi);
            double carrier   = std::sin((t * f * cMult * twoPi) + modulator);

            return carrier;
        };

        // Send to Spectrum (Sample Rate 44100 for high detail)
        besselSpectrum->updateSpectrum(fmAlgo, 44100.0);
    };

    // Connect Sliders so it updates instantly
    connect(besselCarrierMult, &QDoubleSpinBox::valueChanged, updateBesselVisual);
    connect(besselModMult, &QDoubleSpinBox::valueChanged, updateBesselVisual);
    connect(besselModIndex, &QDoubleSpinBox::valueChanged, updateBesselVisual);

    // Run once to initialize
    QTimer::singleShot(100, updateBesselVisual);


    modeTabs->addTab(besselTab, "Bessel FM");

    // ------------------------------------
    // TAB 8: HARMONIC LAB
    // ------------------------------------
    QWidget *harTab = new QWidget();
    auto *harLayout = new QVBoxLayout(harTab);


    harmonicSpectrum = new UniversalSpectrum();
    harLayout->addWidget(harmonicSpectrum);

    buildModeHarmonic = new QComboBox();
    buildModeHarmonic->addItems({"Modern", "Legacy"});
    harLayout->addWidget(new QLabel("Build Mode:"));
    harLayout->addWidget(buildModeHarmonic);

    auto *harGrid = new QGridLayout();


    auto updateHarmonicVisual = [=]() {
        if (!harmonicSpectrum) return;

        std::function<double(double)> additiveAlgo = [=](double t) {
            double signal = 0.0;
            double f = 220.0; // Base frequency (A3) for visualization
            double twoPi = 6.283185;

            // Loop through all 16 sliders to sum the harmonics
            for (int i = 0; i < 16; ++i) {
                // Get value 0.0 to 1.0
                double amp = harmonicSliders[i]->value() / 100.0;

                if (amp > 0.0) {
                    double harmonic = (double)(i + 1); // 1st, 2nd, 3rd harmonic...
                    // sin( 2pi * f * harmonic * t )
                    signal += amp * std::sin(t * f * harmonic * twoPi);
                }
            }
            // Normalise slightly to prevent massive clipping on the visualizer
            return signal * 0.3;
        };

        // 2. Send to Spectrum (44100 Hz resolution)
        harmonicSpectrum->updateSpectrum(additiveAlgo, 44100.0);
    };

    // GENERATE SLIDERS & CONNECT THEM ---
    for(int i=0; i<16; ++i) {
        harmonicSliders[i] = new QSlider(Qt::Vertical);
        harmonicSliders[i]->setRange(0, 100);

        // Default: Set 1st Harmonic (Fundamental) to 100%
        if(i == 0) harmonicSliders[i]->setValue(100);

        harGrid->addWidget(new QLabel(QString("H%1").arg(i+1)), 0, i);
        harGrid->addWidget(harmonicSliders[i], 1, i);

        // Connect every slider to the visualizer routine
        connect(harmonicSliders[i], &QSlider::valueChanged, updateHarmonicVisual);
    }

    harLayout->addLayout(harGrid);
    auto *btnHar = new QPushButton("Generate Harmonic Wave");
    harLayout->addWidget(btnHar);
    modeTabs->addTab(harTab, "Harmonic Lab");

    // Connect the generation button
    connect(btnHar, &QPushButton::clicked, this, &MainWindow::generateHarmonicLab);

    // Trigger once to draw the initial state
    QTimer::singleShot(200, updateHarmonicVisual);

    // ------------------------------------
    // TAB 9: DRUM DESIGNER
    // ------------------------------------
    QWidget *drumWidget = new QWidget();
    QVBoxLayout *drumLayout = new QVBoxLayout(drumWidget);

    // ADD OSCILLOSCOPE (Visualizer)
    drumScope = new UniversalScope();
    drumScope->setMinimumHeight(150);
    drumLayout->addWidget(drumScope);

    // Disclaimer Label
    drumDisclaimer = new QLabel("⚠️ NOTICE: Panning must be set manually. Filters are simulated math approximations.");
    drumDisclaimer->setStyleSheet("color: red; font-weight: bold; border: 1px solid red; padding: 5px; background-color: #ffeeee;");
    drumDisclaimer->setAlignment(Qt::AlignCenter);
    drumLayout->addWidget(drumDisclaimer);

    // CONTROLS SETUP
    drumTypeCombo = new QComboBox();
    drumTypeCombo->addItems({"Kick (LPF)", "Snare (BPF)", "Hi-Hat (HPF)", "Tom (LPF)", "Cowbell (BPF)", "Rimshot (HPF)", "Clap (BPF)"});

    drumWaveCombo = new QComboBox();
    drumWaveCombo->addItems({"Sine", "Triangle", "Square", "Sawtooth"});

    QFormLayout *fLayout = new QFormLayout();

    // Initialise Sliders
    drumPitchSlider = new QSlider(Qt::Horizontal); drumPitchSlider->setRange(20, 150); drumPitchSlider->setValue(40);
    drumDecaySlider = new QSlider(Qt::Horizontal); drumDecaySlider->setRange(1, 200); drumDecaySlider->setValue(40);
    drumPitchDropSlider = new QSlider(Qt::Horizontal); drumPitchDropSlider->setRange(0, 500); drumPitchDropSlider->setValue(350);
    drumToneSlider = new QSlider(Qt::Horizontal);  drumToneSlider->setRange(100, 14000); drumToneSlider->setValue(1000);
    drumSnapSlider = new QSlider(Qt::Horizontal);  drumSnapSlider->setRange(10, 100); drumSnapSlider->setValue(50);
    drumNoiseSlider = new QSlider(Qt::Horizontal); drumNoiseSlider->setRange(0, 100); drumNoiseSlider->setValue(0);
    drumPWMSlider = new QSlider(Qt::Horizontal);   drumPWMSlider->setRange(0, 100); drumPWMSlider->setValue(50);
    drumExpSlider = new QSlider(Qt::Horizontal);   drumExpSlider->setRange(1, 10); drumExpSlider->setValue(2);

    // Add rows to Form Layout
    fLayout->addRow("Body Waveform:", drumWaveCombo);
    fLayout->addRow("Base Pitch:", drumPitchSlider);
    fLayout->addRow("Decay Speed:", drumDecaySlider);
    fLayout->addRow("Exponential Curve:", drumExpSlider);
    fLayout->addRow("Pitch Punch (Drop):", drumPitchDropSlider);
    fLayout->addRow("Filter Cutoff (Sim):", drumToneSlider);
    fLayout->addRow("Filter Res (Snap):", drumSnapSlider);
    fLayout->addRow("Noise Mix:", drumNoiseSlider);
    fLayout->addRow("Pulse Width:", drumPWMSlider);

    drumLayout->addWidget(new QLabel("<b>Internal Filter Drum Designer</b>"));
    drumLayout->addLayout(fLayout);
    drumLayout->addWidget(drumTypeCombo);

    // BUTTONS
    auto *dBtnLay = new QHBoxLayout();

    btnPlayDrum = new QPushButton("▶ Play Drum Loop");
    btnPlayDrum->setCheckable(true);
    btnPlayDrum->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");

    btnGenerateDrum = new QPushButton("Copy XPF to Clipboard");
    btnSaveDrumXpf = new QPushButton("Save Drum as .XPF File");

    dBtnLay->addWidget(btnPlayDrum);
    dBtnLay->addWidget(btnGenerateDrum);
    dBtnLay->addWidget(btnSaveDrumXpf);
    drumLayout->addLayout(dBtnLay);
    drumLayout->addStretch();

    modeTabs->addTab(drumWidget, "Drum Designer");

    // AUDIO & VISUAL LOGIC (The Math)
    auto updateDrum = [=]() {
    // Gather Values from UI
    int waveIdx = drumWaveCombo->currentIndex(); // 0=Sin, 1=Tri, 2=Sqr, 3=Saw
    double baseFreq = drumPitchSlider->value();
    double decayFactor = drumDecaySlider->value();
    double pitchDrop = drumPitchDropSlider->value();
    double noiseMix = drumNoiseSlider->value() / 100.0;
    double expCurve = drumExpSlider->value();

    // Calculate a loop length that fits the decay so it doesn't click
    double loopLen = 0.5 + (200.0 / (decayFactor > 0 ? decayFactor : 1.0));

    // The Audio Generation Lambda
    std::function<double(double)> drumAlgo = [=](double t) {
        // Loop logic (retrigger)
        double localT = std::fmod(t, loopLen);
        if(localT < 0) return 0.0;

        //Pitch Envelope (Exponential Drop)
        // f(t) = Base + Drop * exp(-t * decay/2)
        double instFreq = baseFreq + (pitchDrop * std::exp(-localT * (decayFactor / 2.0)));

        //Oscillator Generation
        double phase = localT * instFreq * 6.283185; // 2*PI
        double osc = 0.0;

        if (waveIdx == 0) osc = std::sin(phase);
        else if (waveIdx == 1) osc = (2.0/3.14159) * std::asin(std::sin(phase)); // Triangle
        else if (waveIdx == 2) osc = (std::sin(phase) > 0 ? 1.0 : -1.0); // Square
        else osc = 2.0 * (std::fmod(localT * instFreq, 1.0)) - 1.0; // Saw

        //Noise Generation
        double noise = ((double)rand() / RAND_MAX) * 2.0 - 1.0;

        //Mix Osc and Noise
        double signal = (osc * (1.0 - noiseMix)) + (noise * noiseMix);

        //Volume Envelope (Exponential Decay)
        double env = std::exp(-localT * decayFactor * expCurve);

        return signal * env;
    };

    // Update Visualizer (Zoomed in to 0.2s to see the hit clearly)
    drumScope->updateScope(drumAlgo, 0.2, 1.0);

    // Update Audio Engine if Playing
    if (btnPlayDrum->isChecked()) {
        m_ghostSynth->setAudioSource(drumAlgo);
    }
};

    // CONNECTIONS

    // Connect sliders to live update
    connect(drumPitchSlider, &QSlider::valueChanged, updateDrum);
    connect(drumDecaySlider, &QSlider::valueChanged, updateDrum);
    connect(drumPitchDropSlider, &QSlider::valueChanged, updateDrum);
    connect(drumExpSlider, &QSlider::valueChanged, updateDrum);
    connect(drumNoiseSlider, &QSlider::valueChanged, updateDrum);
    connect(drumWaveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), updateDrum);

    // PRESET LOGIC: Sets sliders when you choose a drum type
    connect(drumTypeCombo, &QComboBox::currentIndexChanged, [=](int idx){
    // Stop audio briefly to prevent glitches while setting sliders
    bool wasPlaying = btnPlayDrum->isChecked();
    if(wasPlaying) m_ghostSynth->stop();

    // Block signals so we don't trigger 10 updates in a row
    drumPitchSlider->blockSignals(true);
    drumDecaySlider->blockSignals(true);

    switch(idx) {
    case 0: // Kick
            drumWaveCombo->setCurrentText("Sine");
            drumPitchSlider->setValue(40);
            drumPitchDropSlider->setValue(350);
            drumDecaySlider->setValue(40);
            drumExpSlider->setValue(2);
            drumToneSlider->setValue(1000);
            drumSnapSlider->setValue(50);
            drumNoiseSlider->setValue(0);
            break;
    case 1: // Snare
            drumWaveCombo->setCurrentText("Triangle");
            drumPitchSlider->setValue(60);
            drumPitchDropSlider->setValue(200);
            drumNoiseSlider->setValue(70);
            drumToneSlider->setValue(1200);
            drumDecaySlider->setValue(80);
            drumExpSlider->setValue(4);
            break;
    case 2: // Hi-Hat
            drumWaveCombo->setCurrentText("Square");
            drumPitchSlider->setValue(80);
            drumPitchDropSlider->setValue(50);
            drumDecaySlider->setValue(160);
            drumNoiseSlider->setValue(100);
            drumToneSlider->setValue(8000);
            drumExpSlider->setValue(8);
            break;
    case 3: // Tom
            drumWaveCombo->setCurrentText("Sine");
            drumPitchSlider->setValue(50);
            drumPitchDropSlider->setValue(150);
            drumDecaySlider->setValue(60);
            drumNoiseSlider->setValue(10);
            drumToneSlider->setValue(800);
            drumExpSlider->setValue(3);
            break;
    case 4: // Cowbell
            drumWaveCombo->setCurrentText("Square");
            drumPitchSlider->setValue(80);
            drumPitchDropSlider->setValue(0);
            drumToneSlider->setValue(3000);
            drumExpSlider->setValue(3);
            drumDecaySlider->setValue(100);
            drumNoiseSlider->setValue(0);
            break;
    case 5: // Rimshot
            drumWaveCombo->setCurrentText("Square");
            drumPitchSlider->setValue(95);
            drumPitchDropSlider->setValue(20);
            drumNoiseSlider->setValue(20);
            drumToneSlider->setValue(5000);
            drumExpSlider->setValue(8);
            drumDecaySlider->setValue(30);
            break;
    case 6: // Clap
            drumWaveCombo->setCurrentText("Sawtooth");
            drumPitchSlider->setValue(70);
            drumPitchDropSlider->setValue(50);
            drumNoiseSlider->setValue(90);
            drumDecaySlider->setValue(120);
            drumToneSlider->setValue(1000);
            drumExpSlider->setValue(5);
            break;
        }

    // Unblock signals
    drumPitchSlider->blockSignals(false);
    drumDecaySlider->blockSignals(false);

    updateDrum(); // Force one update with new values

    if(wasPlaying) m_ghostSynth->start();
    });

    // Play Button Logic
    connect(btnPlayDrum, &QPushButton::toggled, [=](bool checked){
        if(!checked) {
            m_ghostSynth->setAudioSource([](double){ return 0.0; });
            m_ghostSynth->stop();
            btnPlayDrum->setText("▶ Play Drum Loop");
            btnPlayDrum->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");
        } else {
            m_ghostSynth->start();
            btnPlayDrum->setText("⏹ Stop");
            btnPlayDrum->setStyleSheet("background-color: #338833; color: white; font-weight: bold; height: 40px;");
            updateDrum();
        }
    });

    // Generator Buttons
    connect(btnGenerateDrum, &QPushButton::clicked, this, &MainWindow::generateDrumXpf);
    connect(btnSaveDrumXpf, &QPushButton::clicked, this, &MainWindow::generateDrumXpf);

    // Initial Trigger to draw the default wave
    QTimer::singleShot(200, updateDrum);

    // ------------------------------------
    // TAB 10: VELOCILOGIC
    // ------------------------------------
    QWidget *velTab = new QWidget();
    auto *velLayout = new QVBoxLayout(velTab);

    // Disclaimer
    velDisclaimer = new QLabel("⚠ VELOCILOGIC: DYNAMIC LAYERING.\n"
                               "Checked working with Legacy only.\n"
                               "");
    velDisclaimer->setStyleSheet("QLabel { color: blue; font-weight: bold; font-size: 14px; border: 2px solid blue; padding: 10px; background-color: #eeeeff; }");
    velDisclaimer->setAlignment(Qt::AlignCenter);
    velDisclaimer->setFixedHeight(80);
    velLayout->addWidget(velDisclaimer);

    // Options
    auto *velOptLayout = new QHBoxLayout();
    velOptLayout->addWidget(new QLabel("Build Mode:"));
    velMapMode = new QComboBox();
    velMapMode->addItems({"Nightly (Nested Ternary)", "Legacy (Additive)"});
    velOptLayout->addWidget(velMapMode);
    velOptLayout->addStretch();
    velLayout->addLayout(velOptLayout);

    // The Zone Table
    velMapTable = new QTableWidget();
    velMapTable->setColumnCount(2);
    velMapTable->setHorizontalHeaderLabels({"Upper Velocity Limit (0-127)", "Expression (Code)"});
    velMapTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    velLayout->addWidget(velMapTable);

    // Buttons
    auto *velBtnLayout = new QHBoxLayout();
    auto *btnAddVel = new QPushButton("Add Velocity Zone");
    auto *btnRemVel = new QPushButton("Remove Zone");
    velBtnLayout->addWidget(btnAddVel);
    velBtnLayout->addWidget(btnRemVel);
    velLayout->addLayout(velBtnLayout);

    auto *btnGenVel = new QPushButton("GENERATE VELOCITY MAP");
    btnGenVel->setStyleSheet("font-weight: bold; height: 40px; background-color: #444; color: white;");
    velLayout->addWidget(btnGenVel);

    modeTabs->addTab(velTab, "Velocilogic");

    // Connections
    connect(btnGenVel, &QPushButton::clicked, this, &MainWindow::generateVelocilogic);

    auto addVelZone = [=](int limit, QString code) {
        int r = velMapTable->rowCount();
        velMapTable->insertRow(r);
        velMapTable->setItem(r, 0, new QTableWidgetItem(QString::number(limit)));
        velMapTable->setItem(r, 1, new QTableWidgetItem(code));
    };

    connect(btnAddVel, &QPushButton::clicked, [=](){ addVelZone(100, "pulse(t*f)"); });
    connect(btnRemVel, &QPushButton::clicked, [=](){
        if(velMapTable->rowCount()>0) velMapTable->removeRow(velMapTable->rowCount()-1);
    });

    // Default "Ghost Note" Setup
    // 0-40: Soft Sine (Ghost note)
    // 41-100: Standard Saw
    // 101-127: Hard Square with Noise (Accent)
    addVelZone(40, "sinew(t*f)*0.5");
    addVelZone(100, "saww(t*f)");
    addVelZone(127, "squarew(t*f) + (randv(t)*0.2)");

    // ------------------------------------
    // TAB 11: NOISE FORGE
    // ------------------------------------
    QWidget *noiseTab = new QWidget(); auto *noiseLayout = new QFormLayout(noiseTab);
    buildModeNoise = new QComboBox(); buildModeNoise->addItems({"Modern", "Legacy"});
    noiseRes = new QDoubleSpinBox(); noiseRes->setRange(100, 44100); noiseRes->setValue(8000);
    noiseLayout->addRow("Build Mode:", buildModeNoise);
    noiseLayout->addRow("Sample-Rate:", noiseRes);
    auto *btnNoise = new QPushButton("Generate Noise Forge");
    noiseLayout->addRow(btnNoise);
    modeTabs->addTab(noiseTab, "Noise Forge");

    // ------------------------------------
    // TAB 12: XPF PACKAGER
    // ------------------------------------
    QWidget *xpfTab = new QWidget();
    auto *xpfLayout = new QVBoxLayout(xpfTab);

    // Disclaimer (O1 Editing Mode)
    xpfDisclaimer = new QLabel("⚠ NOTICE: O1 EDITING MODE.\n"
                               "This tab is a placeholder the tab logic is not complete or tested\n"
                               "This tool packages your code into Oscillator 1 (O1) only.\n"
                               "O2, Filters, and Wavetables (W1) are disabled by default.\n"
                               "Panning is centered.");
    xpfDisclaimer->setStyleSheet("QLabel { color: red; font-weight: bold; font-size: 14px; border: 2px solid red; padding: 10px; background-color: #ffeeee; }");
    xpfDisclaimer->setAlignment(Qt::AlignCenter);
    xpfDisclaimer->setFixedHeight(80);
    xpfLayout->addWidget(xpfDisclaimer);

    // Input Area
    auto *xpfGroup = new QGroupBox("Expression Source");
    auto *xpfForm = new QVBoxLayout(xpfGroup);

    xpfInput = new QTextEdit();
    xpfInput->setPlaceholderText("Paste your generated Legacy or Nightly code here...");
    xpfForm->addWidget(xpfInput);
    xpfLayout->addWidget(xpfGroup);

    // Save Button
    btnSaveXpf = new QPushButton("Save as Instrument (.xpf)...");
    btnSaveXpf->setStyleSheet("font-weight: bold; height: 50px; font-size: 14px;");
    xpfLayout->addWidget(btnSaveXpf);

    modeTabs->addTab(xpfTab, "XPF Packager");

    // Connect
    connect(btnSaveXpf, &QPushButton::clicked, this, &MainWindow::saveXpfInstrument);

    // ------------------------------------
    // TAB 13: FILTER FORGE
    // ------------------------------------
    initFilterForgeTab();
    modeTabs->addTab(filterForgeTab, "Filter Forge");
    // ------------------------------------
    // TAB 14: LEAD STACKER
    // ------------------------------------
    QWidget *leadTab = new QWidget();
    QVBoxLayout *leadLayout = new QVBoxLayout(leadTab);

    // OSCILLOSCOPE
    leadScope = new UniversalScope();
    leadScope->setMinimumHeight(150);
    leadLayout->addWidget(leadScope);

    // CONTROLS
    // Group A: Oscillator Core
    QGroupBox *leadOscGroup = new QGroupBox("1. Core Voice");
    QHBoxLayout *oscLay = new QHBoxLayout(leadOscGroup);

    leadWaveType = new QComboBox();
    leadWaveType->addItems({"Sawtooth (Classic)", "Square (Pulse)", "Triangle (Soft)", "Sine (Pure)"});

    oscLay->addWidget(new QLabel("Wave:"));
    oscLay->addWidget(leadWaveType);

    oscLay->addWidget(new QLabel("Sub Osc:"));
    leadSubSlider = new QSlider(Qt::Horizontal); leadSubSlider->setRange(0, 100); leadSubSlider->setValue(0);
    oscLay->addWidget(leadSubSlider);

    oscLay->addWidget(new QLabel("Noise:"));
    leadNoiseSlider = new QSlider(Qt::Horizontal); leadNoiseSlider->setRange(0, 100); leadNoiseSlider->setValue(0);
    oscLay->addWidget(leadNoiseSlider);

    leadLayout->addWidget(leadOscGroup);

    // Unison Engine
    QGroupBox *leadUniGroup = new QGroupBox("2. Super Unison Stack");
    QGridLayout *uniGrid = new QGridLayout(leadUniGroup);

    leadUnisonCount = new QSpinBox();
    leadUnisonCount->setRange(1, 16); // Up to 16 voices!
    leadUnisonCount->setValue(5);

    leadDetuneSlider = new QSlider(Qt::Horizontal);
    leadDetuneSlider->setRange(0, 100);
    leadDetuneSlider->setValue(25);

    leadWidthSlider = new QSlider(Qt::Horizontal);
    leadWidthSlider->setRange(0, 100);
    leadWidthSlider->setValue(50);

    leadVibeSlider = new QSlider(Qt::Horizontal);
    leadVibeSlider->setRange(0, 100);
    leadVibeSlider->setValue(10);

    uniGrid->addWidget(new QLabel("Voice Count:"), 0, 0);
    uniGrid->addWidget(leadUnisonCount, 0, 1);

    uniGrid->addWidget(new QLabel("Detune (Spread):"), 1, 0);
    uniGrid->addWidget(leadDetuneSlider, 1, 1);

    uniGrid->addWidget(new QLabel("Stereo Width:"), 2, 0);
    uniGrid->addWidget(leadWidthSlider, 2, 1);

    uniGrid->addWidget(new QLabel("Vintage Drift:"), 3, 0);
    uniGrid->addWidget(leadVibeSlider, 3, 1);

    leadLayout->addWidget(leadUniGroup);

    // BUTTONS
    QHBoxLayout *leadBtnLay = new QHBoxLayout();

    btnPlayLead = new QPushButton("▶ Play Stack");
    btnPlayLead->setCheckable(true);
    btnPlayLead->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");

    auto *btnGenLead = new QPushButton("GENERATE SUPER STACK");
    btnGenLead->setStyleSheet("font-weight: bold; background-color: #004488; color: white; height: 40px;");

    leadBtnLay->addWidget(btnPlayLead);
    leadBtnLay->addWidget(btnGenLead);
    leadLayout->addLayout(leadBtnLay);

    // PREVIEW LOGIC
    auto updateLeadPreview = [=]() {
        int voices = leadUnisonCount->value();
        double detune = leadDetuneSlider->value() / 500.0; // Scaled for audio
        double sub = leadSubSlider->value() / 100.0;
        double noise = leadNoiseSlider->value() / 100.0;
        double drift = leadVibeSlider->value() / 2000.0;
        int waveIdx = leadWaveType->currentIndex(); // 0=Saw, 1=Sqr, 2=Tri, 3=Sin

        std::function<double(double)> leadAlgo = [=](double t) {
            double baseF = 110.0; // A2
            double signal = 0.0;
            double driftVal = (drift > 0) ? (std::sin(t * 3.0) * drift) : 0.0;

            // Voice Loop
            for(int i=0; i<voices; ++i) {
                // Calculate spread ratio (-0.5 to 0.5)
                double ratio = (voices > 1) ? ((double)i / (voices - 1)) - 0.5 : 0.0;
                double f = baseF * (1.0 + (ratio * detune) + driftVal);

                // Oscillator
                double phase = std::fmod(t * f, 1.0);
                double osc = 0.0;
                if(waveIdx == 0) osc = 2.0 * phase - 1.0; // Saw
                else if(waveIdx == 1) osc = (phase > 0.5) ? 1.0 : -1.0; // Square
                else if(waveIdx == 2) osc = 2.0 * std::abs(2.0 * phase - 1.0) - 1.0; // Tri
                else osc = std::sin(phase * 6.28318);

                signal += osc;
            }

            // Normalize
            signal /= (double)voices;

            // Add Sub Osc (Octave down, Square)
            if(sub > 0) {
                double subPhase = std::fmod(t * baseF * 0.5, 1.0);
                double subOsc = (subPhase > 0.5) ? 1.0 : -1.0;
                signal += subOsc * sub;
            }

            // Add Noise
            if(noise > 0) {
                signal += ((double)rand()/RAND_MAX * 2.0 - 1.0) * noise;
            }

            return signal;
        };

        // Update Scope (Zoom out slightly to see the beating)
        leadScope->updateScope(leadAlgo, 0.1, 1.0);

        // Update Audio
        if(btnPlayLead->isChecked()) {
            m_ghostSynth->setAudioSource(leadAlgo);
        }
    };

    // Connections
    connect(leadUnisonCount, &QSpinBox::valueChanged, updateLeadPreview);
    connect(leadDetuneSlider, &QSlider::valueChanged, updateLeadPreview);
    connect(leadSubSlider, &QSlider::valueChanged, updateLeadPreview);
    connect(leadNoiseSlider, &QSlider::valueChanged, updateLeadPreview);
    connect(leadVibeSlider, &QSlider::valueChanged, updateLeadPreview);
    connect(leadWaveType, QOverload<int>::of(&QComboBox::currentIndexChanged), updateLeadPreview);

    connect(btnPlayLead, &QPushButton::toggled, [=](bool checked){
        if(!checked) {
            m_ghostSynth->setAudioSource([](double){ return 0.0; });
            m_ghostSynth->stop();
            btnPlayLead->setText("▶ Play Stack");
            btnPlayLead->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");
        } else {
            m_ghostSynth->start();
            btnPlayLead->setText("⏹ Stop");
            btnPlayLead->setStyleSheet("background-color: #338833; color: white; font-weight: bold; height: 40px;");
            updateLeadPreview();
        }
    });

    connect(btnGenLead, &QPushButton::clicked, this, &MainWindow::generateLeadStack);

    // Initial Trigger
    QTimer::singleShot(300, updateLeadPreview);

    leadLayout->addStretch();
    modeTabs->addTab(leadTab, "Lead Stacker");

    // ------------------------------------
    // TAB 15: RANDOMISER
    // ------------------------------------
    QWidget *randTab = new QWidget();
    auto *randLayout = new QVBoxLayout(randTab);

    // Visualizer
    randScope = new UniversalScope();
    randScope->setMinimumHeight(150);
    randLayout->addWidget(randScope);

    // Controls
    randLayout->addWidget(new QLabel("Chaos Level (Randomness):"));
    randLayout->addWidget(chaosSlider);

    // Buttons
    auto *randBtnLay = new QHBoxLayout();

    btnPlayRand = new QPushButton("▶ Play Result");
    btnPlayRand->setCheckable(true);
    btnPlayRand->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 50px;");

    auto *btnRand = new QPushButton("GENERATE CHAOS");
    btnRand->setStyleSheet("background-color: #444; color: white; font-weight: bold; height: 50px;");

    randBtnLay->addWidget(btnPlayRand);
    randBtnLay->addWidget(btnRand);
    randLayout->addLayout(randBtnLay);

    randLayout->addStretch();
    modeTabs->addTab(randTab, "Randomiser");

    // Connection for Play Button
    connect(btnPlayRand, &QPushButton::toggled, [=](bool checked){
        if(!checked) {
            m_ghostSynth->setAudioSource([](double){ return 0.0; });
            m_ghostSynth->stop();
            btnPlayRand->setText("▶ Play Result");
            btnPlayRand->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 50px;");
        } else {
            // Ensure we have a valid function to play
            if(currentRandFunc) m_ghostSynth->setAudioSource(currentRandFunc);
            m_ghostSynth->start();
            btnPlayRand->setText("⏹ Stop");
            btnPlayRand->setStyleSheet("background-color: #338833; color: white; font-weight: bold; height: 50px;");
        }
    });

    // ------------------------------------
    // TAB 16: PHONETIC LAB
    // ------------------------------------
    QWidget *phoneticTab = new QWidget();
    auto *pLayout = new QVBoxLayout(phoneticTab);

    pLayout->addWidget(new QLabel("<b>Phonetic Input:</b> Space separated. Use numbers 1-8 for stress/pitch (e.g., IY4)."));
    phoneticInput = new QTextEdit();
    // Default example: "SAM IS HERE" with stress markers
    phoneticInput->setPlaceholderText("S* A*4 M* space IY5 Z* space H IY4 R*");
    phoneticInput->setMaximumHeight(120);
    pLayout->addWidget(phoneticInput);

    // Settings Bar
    auto *pModeLayout = new QHBoxLayout();
    pModeLayout->addWidget(new QLabel("Render Mode:"));
    parserModeCombo = new QComboBox();
    parserModeCombo->addItems({"High Quality (Smooth)", "Retro (8-bit Grit)"});
    pModeLayout->addWidget(parserModeCombo);

    pModeLayout->addWidget(new QLabel("  Parsing Engine:"));
    parsingStyleCombo = new QComboBox();
    parsingStyleCombo->addItems({"Legacy (Xpressive)", "Nightly Build (Experimental)"});
    pModeLayout->addWidget(parsingStyleCombo);

    pLayout->addLayout(pModeLayout);

    btnGenPhonetic = new QPushButton("Generate Formula to Clipboard");
    pLayout->addWidget(btnGenPhonetic);

    pLayout->addWidget(new QLabel("<b>Phoneme Reference:</b>"));
    QScrollArea *pScroll = new QScrollArea();
    phonemeRefLabel = new QLabel(samLibrary.keys().join(" | "));
    phonemeRefLabel->setWordWrap(true);
    pScroll->setWidget(phonemeRefLabel);
    pScroll->setWidgetResizable(true);
    pLayout->addWidget(pScroll);

    modeTabs->addTab(phoneticTab, "Phonetic Lab");
    connect(btnGenPhonetic, &QPushButton::clicked, this, &MainWindow::generatePhoneticFormula);

    // ------------------------------------
    // TAB 17: LOGIC CONVERTER
    // ------------------------------------
    QWidget *convTab = new QWidget();

    // CHANGE: Main layout is now VERTICAL to stack the warning on top
    auto *mainConvLayout = new QVBoxLayout(convTab);

    // NEW: The Red Disclaimer Label
    convDisclaimer = new QLabel(""
                                ""
                                "");
    convDisclaimer->setStyleSheet("QLabel { color: red; font-weight: bold; font-size: 14px; border: 2px solid red; padding: 10px; background-color: #ffeeee; }");
    convDisclaimer->setAlignment(Qt::AlignCenter);
    convDisclaimer->setFixedHeight(80); // Fixed height to ensure it's visible

    mainConvLayout->addWidget(convDisclaimer); // Add it to the top

    // The Columns (Input - Buttons - Output)
    auto *hLayout = new QHBoxLayout(); // This holds the boxes side-by-side

    // LEFT SIDE (Input)
    auto *leftGroup = new QGroupBox("Input Formula");
    auto *leftLay = new QVBoxLayout(leftGroup);
    convInput = new QTextEdit();
    convInput->setPlaceholderText("Paste Legacy or Nightly code here...");
    leftLay->addWidget(convInput);

    // MIDDLE (Buttons)
    auto *midLayout = new QVBoxLayout();
    btnToNightly = new QPushButton("Legacy\n-->\nNightly");
    btnToLegacy = new QPushButton("Nightly\n-->\nLegacy");

    midLayout->addStretch();
    midLayout->addWidget(btnToNightly);
    midLayout->addWidget(new QLabel(" ")); // Spacer
    midLayout->addWidget(btnToLegacy);
    midLayout->addStretch();

    // RIGHT SIDE (Output)
    auto *rightGroup = new QGroupBox("Converted Result");
    auto *rightLay = new QVBoxLayout(rightGroup);
    convOutput = new QTextEdit();
    convOutput->setReadOnly(true);
    rightLay->addWidget(convOutput);

    // Add columns to the sub-layout
    hLayout->addWidget(leftGroup, 2);
    hLayout->addLayout(midLayout, 0);
    hLayout->addWidget(rightGroup, 2);

    // Add the sub-layout to the main vertical layout
    mainConvLayout->addLayout(hLayout);

    modeTabs->addTab(convTab, "Logic Converter");

    // Connect the buttons (Using the new Universal Logic)
    connect(btnToNightly, &QPushButton::clicked, [=](){
        convOutput->setText(convertLegacyToNightly(convInput->toPlainText()));
    });

    connect(btnToLegacy, &QPushButton::clicked, [=](){
        convOutput->setText(convertNightlyToLegacy(convInput->toPlainText()));
    });

    // ------------------------------------
    // TAB 18: KEY MAPPER
    // ------------------------------------
    QWidget *keyTab = new QWidget();
    auto *keyLayout = new QVBoxLayout(keyTab);

    // Disclaimer
    keyMapDisclaimer = new QLabel("⚠ DISCLAIMER: EXPERIMENTAL KEY MAPPING.\n"
                                  "This feature allows splitting logic across the keyboard.\n"
                                  "Requires further development.\n"
                                  "Only tested with legacy");
    keyMapDisclaimer->setStyleSheet("QLabel { color: red; font-weight: bold; font-size: 14px; border: 2px solid red; padding: 10px; background-color: #ffeeee; }");
    keyMapDisclaimer->setAlignment(Qt::AlignCenter);
    keyMapDisclaimer->setFixedHeight(80);
    keyLayout->addWidget(keyMapDisclaimer);

    // Options
    auto *keyOptLayout = new QHBoxLayout();
    keyOptLayout->addWidget(new QLabel("Build Mode:"));
    keyMapMode = new QComboBox();
    keyMapMode->addItems({"Nightly (Nested Ternary)", "Legacy (Additive)"});
    keyOptLayout->addWidget(keyMapMode);
    keyOptLayout->addStretch();
    keyLayout->addLayout(keyOptLayout);

    // The Zone Table
    keyMapTable = new QTableWidget();
    keyMapTable->setColumnCount(2);
    keyMapTable->setHorizontalHeaderLabels({"Upper Key Limit (0-127)", "Expression (Code)"});
    keyMapTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    keyLayout->addWidget(keyMapTable);

    // Buttons (Add/Remove/Gen)
    auto *keyBtnLayout = new QHBoxLayout();
    auto *btnAddZone = new QPushButton("Add Split Zone");
    auto *btnRemZone = new QPushButton("Remove Zone");
    keyBtnLayout->addWidget(btnAddZone);
    keyBtnLayout->addWidget(btnRemZone);
    keyLayout->addLayout(keyBtnLayout);

    auto *btnGenKey = new QPushButton("GENERATE KEY MAP");
    btnGenKey->setStyleSheet("font-weight: bold; height: 40px; background-color: #444; color: white;");
    keyLayout->addWidget(btnGenKey);

    modeTabs->addTab(keyTab, "Key Mapper");

    // Connections
    connect(btnGenKey, &QPushButton::clicked, this, &MainWindow::generateKeyMapper);

    // Logic to add rows
    auto addZone = [=](int limit, QString code) {
        int r = keyMapTable->rowCount();
        keyMapTable->insertRow(r);
        keyMapTable->setItem(r, 0, new QTableWidgetItem(QString::number(limit)));
        keyMapTable->setItem(r, 1, new QTableWidgetItem(code));
    };

    connect(btnAddZone, &QPushButton::clicked, [=](){ addZone(72, "sinew(t*f)"); });
    connect(btnRemZone, &QPushButton::clicked, [=](){
        if(keyMapTable->rowCount()>0) keyMapTable->removeRow(keyMapTable->rowCount()-1);
    });

    // Add Default Splits (Bass / Lead)
    addZone(60, "saww(t*f*0.5)"); // Bass (Below Middle C)
    addZone(128, "squarew(t*f)");   // Lead (Everything else)

    // ------------------------------------
    // TAB 19: STEP GATE
    // ------------------------------------
    QWidget *gateTab = new QWidget();
    QVBoxLayout *gateLayout = new QVBoxLayout(gateTab);

    QLabel *gateDisclaimer = new QLabel("⚠ DISCLAIMER: INCOMPLETE FEATURE.\n"
                                        "Only Legacy gate logic is currently working.");
    gateDisclaimer->setStyleSheet("QLabel { color: red; font-weight: bold; font-size: 14px; border: 2px solid red; padding: 10px; background-color: #ffeeee; }");
    gateDisclaimer->setAlignment(Qt::AlignCenter);
    gateDisclaimer->setFixedHeight(80);
    gateLayout->addWidget(gateDisclaimer);

    // Controls (Build Mode, Speed, Triplet, Mix)
    QHBoxLayout *gateCtrlLayout = new QHBoxLayout();
    gateBuildMode = new QComboBox(); gateBuildMode->addItems({"Nightly (Variables)", "Legacy (Inline)"});
    gateSpeedCombo = new QComboBox(); gateSpeedCombo->addItems({"1/2 Speed (Slow)", "1x (Synced)", "2x (Fast)", "4x (Hyper)"});
    gateSpeedCombo->setCurrentIndex(1);
    gateTripletCheck = new QCheckBox("Triplet Mode (3/2)");
    gateMixSlider = new QSlider(Qt::Horizontal); gateMixSlider->setRange(0, 100); gateMixSlider->setValue(100);

    gateCtrlLayout->addWidget(new QLabel("Build:")); gateCtrlLayout->addWidget(gateBuildMode);
    gateCtrlLayout->addWidget(new QLabel("Speed:")); gateCtrlLayout->addWidget(gateSpeedCombo);
    gateCtrlLayout->addWidget(gateTripletCheck);
    gateCtrlLayout->addWidget(new QLabel("Mix:")); gateCtrlLayout->addWidget(gateMixSlider);
    gateLayout->addLayout(gateCtrlLayout);

    // 16-Step Grid Buttons
    QGridLayout *gateGrid = new QGridLayout();
    gateGrid->setSpacing(4);
    for(int i=0; i<16; ++i) {
        gateSteps[i] = new QPushButton(QString::number(i+1));
        gateSteps[i]->setCheckable(true);
        gateSteps[i]->setFixedSize(45, 40);
        // Styling: Red (OFF) -> Green (ON)
        gateSteps[i]->setStyleSheet(
            "QPushButton { background-color: #441111; color: #ff9999; border: 1px solid #552222; border-radius: 4px; }"
            "QPushButton:checked { background-color: #00ee00; color: black; border: 1px solid #00aa00; font-weight: bold; }"
            );
        // Default pattern: (Every 3rd step roughly)
        if(i == 0 || i == 2 || i == 3 || i == 6 || i == 8 || i == 10 || i == 11 || i == 14)
            gateSteps[i]->setChecked(true);

        int row = i / 8; // 2 rows of 8
        int col = i % 8;
        gateGrid->addWidget(gateSteps[i], row, col);
    }
    gateLayout->addLayout(gateGrid);

    // Waveform Selector
    QFormLayout *gateForm = new QFormLayout();
    gateShapeCombo = new QComboBox();
    gateShapeCombo->addItems({"Square Wave (Basic)", "Sawtooth (Sharp)", "Sine Wave (Soft)", "Noise (Perc)", "Custom (Paste Below)"});
    gateCustomShape = new QTextEdit();
    gateCustomShape->setPlaceholderText("Paste custom formula here if 'Custom' selected (use 'f' for freq)...");
    gateCustomShape->setMaximumHeight(60);

    gateForm->addRow("Source Wave:", gateShapeCombo);
    gateForm->addRow("Custom Code:", gateCustomShape);
    gateLayout->addLayout(gateForm);

    // Generate Button
    QPushButton *btnGenGate = new QPushButton("GENERATE STEP GATE");
    btnGenGate->setStyleSheet("font-weight: bold; background-color: #444; color: white; height: 50px; font-size: 14px;");
    gateLayout->addWidget(btnGenGate);

    modeTabs->addTab(gateTab, "Step Gate");

    connect(btnGenGate, &QPushButton::clicked, this, &MainWindow::generateStepGate);

    // ------------------------------------
    // TAB 20: NUMBERS 1981
    // ------------------------------------
    QWidget *numTab = new QWidget();
    auto *numLayout = new QVBoxLayout(numTab);

    // Top Controls
    auto *numCtrlLayout = new QHBoxLayout();
    numModeCombo = new QComboBox(); numModeCombo->addItems({"Random Stream", "Pattern Editor"});
    numStepsCombo = new QComboBox(); numStepsCombo->addItems({"16 Steps", "32 Steps"}); numStepsCombo->setCurrentIndex(1); // Default 32
    numDuration = new QDoubleSpinBox(); numDuration->setRange(0.01, 1.0); numDuration->setValue(0.2); numDuration->setSingleStep(0.05);

    numCtrlLayout->addWidget(new QLabel("Mode:")); numCtrlLayout->addWidget(numModeCombo);
    numCtrlLayout->addWidget(new QLabel("Steps:")); numCtrlLayout->addWidget(numStepsCombo);
    numCtrlLayout->addWidget(new QLabel("Note Dur:")); numCtrlLayout->addWidget(numDuration);
    numLayout->addLayout(numCtrlLayout);

    // Pattern Editor (Table)
    numPatternTable = new QTableWidget();
    numPatternTable->setRowCount(1);
    numPatternTable->setColumnCount(32); // Max 32
    numPatternTable->setFixedHeight(70);
    numPatternTable->verticalHeader()->setVisible(false);
    numPatternTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Fill with default zeros
    for(int i=0; i<32; ++i) numPatternTable->setItem(0, i, new QTableWidgetItem("0"));

    numLayout->addWidget(new QLabel("<b>Pattern Editor:</b> (Semitones +/- 12). Only used if 'Pattern Editor' mode selected."));
    numLayout->addWidget(numPatternTable);

    // Outputs (O1 & O2)
    auto *numOutLayout = new QHBoxLayout();
    auto *g1 = new QGroupBox("O1: Pan Left (Main)"); auto *l1 = new QVBoxLayout(g1);
    numOut1 = new QTextEdit(); l1->addWidget(numOut1);

    auto *g2 = new QGroupBox("O2: Pan Right (Detuned+Vib)"); auto *l2 = new QVBoxLayout(g2);
    numOut2 = new QTextEdit(); l2->addWidget(numOut2);

    numOutLayout->addWidget(g1);
    numOutLayout->addWidget(g2);
    numLayout->addLayout(numOutLayout);

    //Generate Button
    auto *btnGenNum = new QPushButton("GENERATE NUMBERS 1981");
    btnGenNum->setStyleSheet("font-weight: bold; background-color: #444; color: white; height: 40px;");
    numLayout->addWidget(btnGenNum);

    modeTabs->addTab(numTab, "Numbers 1981");
    connect(btnGenNum, &QPushButton::clicked, this, &MainWindow::generateNumbers1981);

    // Logic to hide/show pattern table based on mode
    connect(numModeCombo, &QComboBox::currentIndexChanged, [=](int idx){
        numPatternTable->setVisible(idx == 1);
    });
    // Default visibility check
    numPatternTable->setVisible(false);

    // ------------------------------------
    // TAB 21: DELAY ARCHITECT
    // ------------------------------------
    QWidget *delayTab = new QWidget();
    auto *delayLayout = new QVBoxLayout(delayTab);

    // Disclaimer
    QLabel *delayWarn = new QLabel("⚠ DISCLAIMER: LEGACY / PLACEHOLDER\n"
                                   "This feature is not working properly yet.\n"
                                   "Note: handwritten code clamp(-1, (trianglew(integrate(f)) * exp(-t * 20))+(0.6 * last(8000) * (t > 0.2)), 1) worked");

    // Style it red
    delayWarn->setStyleSheet("QLabel { color: red; font-weight: bold; border: 2px solid red; padding: 10px; background-color: #ffeeee; }");
    delayWarn->setAlignment(Qt::AlignCenter);
    delayWarn->setFixedHeight(80);

    delayLayout->addWidget(delayWarn);

    // Controls
    auto *delayForm = new QFormLayout();

    delayWaveCombo = new QComboBox();
    delayWaveCombo->addItems({"Plucky Triangle (Default)", "Sawtooth Sweep", "Simple Square", "Custom (Below)"});

    delayCustomInput = new QTextEdit();
    delayCustomInput->setPlaceholderText("Paste custom source here (e.g., sinew(integrate(f)))...");
    delayCustomInput->setMaximumHeight(60);

    delayTimeSpin = new QDoubleSpinBox();
    delayTimeSpin->setRange(0.01, 2.0); delayTimeSpin->setValue(0.2); delayTimeSpin->setSuffix(" s");

    delayRateSpin = new QDoubleSpinBox();
    delayRateSpin->setRange(1000, 44100); delayRateSpin->setValue(8000); delayRateSpin->setSuffix(" Hz");

    delayFeedbackSpin = new QDoubleSpinBox();
    delayFeedbackSpin->setRange(0.1, 0.99); delayFeedbackSpin->setValue(0.6); delayFeedbackSpin->setSingleStep(0.1);

    delayTapsSpin = new QSpinBox();
    delayTapsSpin->setRange(1, 16); delayTapsSpin->setValue(4);

    delayForm->addRow("Source:", delayWaveCombo);
    delayForm->addRow("Custom Code:", delayCustomInput);
    delayForm->addRow("Delay Time:", delayTimeSpin);
    delayForm->addRow("Sample Rate:", delayRateSpin); // Needed to calc 'last(n)'
    delayForm->addRow("Feedback:", delayFeedbackSpin);
    delayForm->addRow("Echo Count:", delayTapsSpin);

    delayLayout->addLayout(delayForm);

    auto *btnGenDelay = new QPushButton("GENERATE DELAY CHAIN");
    btnGenDelay->setStyleSheet("font-weight: bold; height: 40px; background-color: #444; color: white;");
    delayLayout->addWidget(btnGenDelay);

    modeTabs->addTab(delayTab, "Delay Architect");

    connect(btnGenDelay, &QPushButton::clicked, this, &MainWindow::generateDelayArchitect);

    // ------------------------------------
    // TAB 22: MACRO MORPH
    // ------------------------------------
    QWidget *macroTab = new QWidget;
    QVBoxLayout *macroLayout = new QVBoxLayout(macroTab);

    // VISUALISER
    macroScope = new UniversalScope();
    macroScope->setMinimumHeight(150);
    macroLayout->addWidget(macroScope);

    // Header / Description
    QLabel *fluxDesc = new QLabel("<b>Macro Morph Engine</b><br>"
                                  "Select a Vibe, then tweak the Macro Knobs. The sliders update the code logic dynamically.");
    fluxDesc->setStyleSheet("color: #333; padding: 10px; background-color: #eee; border-radius: 5px;");
    macroLayout->addWidget(fluxDesc);

    //TOP BAR: BUILD MODE ---
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->addWidget(new QLabel("Parsing Engine:"));

    macroBuildMode = new QComboBox();
    macroBuildMode->addItems({"Nightly (Clean / Variables)", "Legacy (Inline / Safe)"});
    topBar->addWidget(macroBuildMode);
    topBar->addStretch();
    macroLayout->addLayout(topBar);

    //  PRESET SELECTOR ---
    QGroupBox *sourceGroup = new QGroupBox("1. Select Vibe (Auto-Sets Sliders)");
    sourceGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #666; margin-top: 10px; }");
    QVBoxLayout *sourceLay = new QVBoxLayout(sourceGroup);

    macroStyleCombo = new QComboBox;
    macroStyleCombo->addItems({
        "00. Super Saws (Anthemic)",
        "01. Formant Vocal Lead (Chops)",
        "02. Wobbly Cassette Keys (Lo-Fi)",
        "03. Granular Pad (Jitter)",
        "04. Hollow Bass (Deep House)",
        "05. Portamento Lead (Gliding)",
        "06. Plucky Arp (Short)",
        "07. Vinyl Atmosphere (Texture Only)",
        "08. Cyberpunk Bass (Distorted) [NEW]",
        "09. Hardstyle Kick (Punch) [NEW]",
        "10. Vaporwave E-Piano (Dreamy) [NEW]"
    });
    sourceLay->addWidget(macroStyleCombo);
    macroLayout->addWidget(sourceGroup);

    // --- GROUP 2: THE MACRO CONSOLE ---
    QGroupBox *macroGroup = new QGroupBox("2. Macro Controls");
    macroGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #008080; background-color: #f4fcfc; margin-top: 10px; }");
    QGridLayout *macroGrid = new QGridLayout(macroGroup);

    auto addMacro = [&](QString name, QSlider*& slider, int row, int col, int def) {
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 100); slider->setValue(def);
        macroGrid->addWidget(new QLabel(name), row * 2, col);
        macroGrid->addWidget(slider, row * 2 + 1, col);
    };

    // Column 1: Tonal Character
    addMacro("Color (Timbre/Cutoff)", macroColorSlider, 0, 0, 50);
    addMacro("Texture (Noise/Grain)", macroTextureSlider, 1, 0, 20);
    addMacro("Bitcrush (Res/Grit)", macroBitcrushSlider, 2, 0, 0);

    // Column 2: Space & Time
    addMacro("Time (Envelope/Decay)", macroTimeSlider, 0, 1, 50);
    addMacro("Width (Detune/Spread)", macroWidthSlider, 1, 1, 30);
    addMacro("Wonk (Swing/Sidechain)", macroWonkySlider, 2, 1, 25);

    macroLayout->addWidget(macroGroup);

    //BUTTONS ---
    QHBoxLayout *macroBtnLay = new QHBoxLayout();

    // Play Button (NEW)
    btnPlayMacro = new QPushButton("▶ Play Preview");
    btnPlayMacro->setCheckable(true);
    btnPlayMacro->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");

    // Generate Button
    QPushButton *btnGenMacro = new QPushButton("GENERATE FUTURE PATCH");
    btnGenMacro->setStyleSheet("font-weight: bold; background-color: #008080; color: white; height: 40px;");

    macroBtnLay->addWidget(btnPlayMacro);
    macroBtnLay->addWidget(btnGenMacro);
    macroLayout->addLayout(macroBtnLay);

    // LOGIC: UPDATE PREVIEW (SCOPE + AUDIO) ---
    auto updateMacroPreview = [=]() {
        int style = macroStyleCombo->currentIndex();

        // Normalize Sliders 0.0 - 1.0
        double valColor = macroColorSlider->value() / 100.0;
        double valTex   = macroTextureSlider->value() / 100.0;
        double valCrush = macroBitcrushSlider->value() / 100.0;
        double valTime  = macroTimeSlider->value() / 100.0;
        double valWidth = macroWidthSlider->value() / 100.0;
        double valWonk  = macroWonkySlider->value() / 100.0;

        // C++ Audio Routine (Approximation of the Xpressive Code)
        std::function<double(double)> audioRoutine = [=](double t) {
            double f = 110.0; // Base frequency (A2)
            double pi = 3.14159;
            double signal = 0.0;

            // GENERATE CORE SIGNAL
            switch(style) {
            case 0: // Super Saws
            {
                double d = valWidth * 0.02;
                double s1 = 2.0 * (std::fmod(t*f, 1.0)) - 1.0;
                double s2 = 2.0 * (std::fmod(t*f*(1.0+d), 1.0)) - 1.0;
                double s3 = 2.0 * (std::fmod(t*f*(1.0-d), 1.0)) - 1.0;
                signal = (s1 + s2 + s3) / 3.0;
                // Filter sim
                signal = signal * (0.2 + 0.8 * valColor);
            }
            break;
            case 1: // Formant Vocal
            {
                // FM Formant approximation
                double formant = 2.0 + (valColor * 3.0);
                double mod = std::sin(t * f * formant * 2 * pi) * (0.3 + valWidth * 0.2);
                signal = std::sin((t * f * 2 * pi) + mod);
                // LFO
                double lfo = 1.0 + (0.1 * valTime * std::sin(t * 6.0));
                signal *= lfo;
            }
            break;
            case 2: // Cassette Keys
            {
                double drift = 1.0 + (valWidth * 0.005 * std::sin(t * 2.0));
                // Triangle approximation
                double ph = std::fmod(t * f * drift, 1.0);
                signal = 2.0 * std::abs(2.0 * ph - 1.0) - 1.0;
            }
            break;
            case 3: // Granular Pad
            {
                double saw = 2.0 * (std::fmod(t*f, 1.0)) - 1.0;
                double grain = ((double)rand()/RAND_MAX * 2.0 - 1.0) * valTex;
                signal = saw * (0.8 + grain);
            }
            break;
            case 4: // Hollow Bass
            {
                double ph = std::fmod(t*f, 1.0);
                signal = (ph > 0.5) ? 1.0 : -1.0; // Square
                // Filter decay
                double env = 1.0 - (valColor * std::exp(-t * 20.0));
                signal *= env;
            }
            break;
            case 5: // Portamento (Just saw for preview)
                signal = 2.0 * (std::fmod(t*f, 1.0)) - 1.0;
                break;
            case 8: // Cyberpunk Bass
            {
                double saw = 2.0 * (std::fmod(t*f*0.5, 1.0)) - 1.0; // Sub osc
                // Distortion (Fold)
                double drive = 1.0 + (valCrush * 5.0);
                signal = std::tanh(saw * drive);
            }
            break;
            case 9: // Hardstyle Kick
            {
                // Pitch drop
                double fDrop = f * (4.0 * std::exp(-t * 20.0));
                signal = std::sin(t * fDrop * 2 * pi);
                // Clip
                if (signal > 0.5) signal = 0.5;
                if (signal < -0.5) signal = -0.5;
            }
            break;
            case 10: // Vaporwave
            {
                double fm = std::sin(t * f * 4.0 * 2 * pi) * valColor;
                signal = std::sin((t * f * 2 * pi) + fm);
            }
            break;
            default: // Fallback
                signal = std::sin(t * f * 2 * pi);
            }

            //APPLY ENVELOPE (TIME)
            if (style != 7) { // Skip for Atmosphere
                double decay = 5.0 - (valTime * 4.0); // Map 0-1 to 5-1 speed
                if (decay < 0.1) decay = 0.1;
                double env = std::exp(-std::fmod(t, 0.5) * decay * 10.0); // Repeated envelope for scope
                signal *= env;
            }

            // TEXTURE (Noise)
            if (valTex > 0) {
                double noise = ((double)rand()/RAND_MAX * 2.0 - 1.0);
                signal += noise * valTex * 0.2;
            }

            // BITCRUSH (Step)
            if (valCrush > 0) {
                double steps = 16.0 - (valCrush * 14.0);
                signal = std::floor(signal * steps) / steps;
            }

            // WONK (Sidechain)
            if (valWonk > 0) {
                double pump = 0.5 * (1.0 + std::sin(t * 8.0)); // 8Hz LFO
                signal *= (1.0 - (valWonk * pump));
            }

            return signal;
        };

        // Update Scope (Zoom based on Time slider mostly)
        macroScope->updateScope(audioRoutine, 0.2, 1.0);

        // Update Audio
        if(btnPlayMacro->isChecked()) {
            m_ghostSynth->setAudioSource(audioRoutine);
        }
    };

    // LOGIC: PRESET LOADER (MOVES SLIDERS) ---
    connect(macroStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx){
        // Helper to set all 6 sliders at once
        auto setS = [&](int c, int tx, int bc, int tm, int w, int wk) {
            macroColorSlider->setValue(c);
            macroTextureSlider->setValue(tx);
            macroBitcrushSlider->setValue(bc);
            macroTimeSlider->setValue(tm);
            macroWidthSlider->setValue(w);
            macroWonkySlider->setValue(wk);
        };

        // PRESET DEFINITIONS
        switch(idx) {
        case 0: setS(80, 10, 0, 60, 50, 0); break;  // Super Saws
        case 1: setS(30, 0, 10, 40, 20, 0); break;  // Formant
        case 2: setS(50, 40, 20, 80, 80, 30); break; // Cassette
        case 3: setS(20, 80, 0, 50, 50, 10); break; // Granular
        case 4: setS(90, 0, 0, 40, 0, 0); break;    // Hollow Bass
        case 5: setS(50, 0, 0, 100, 30, 0); break;  // Portamento
        case 6: setS(100, 10, 0, 10, 10, 0); break; // Plucky
        case 7: setS(0, 100, 50, 100, 0, 0); break; // Atmosphere
        case 8: setS(80, 20, 80, 60, 0, 0); break;  // Cyberpunk (High Crush)
        case 9: setS(20, 0, 100, 10, 0, 0); break;  // Hardstyle (Max Crush, Short Time)
        case 10: setS(60, 10, 5, 80, 90, 50); break; // Vaporwave (High Width/Wonk)
        }

        // Trigger update immediately
        updateMacroPreview();
    });

    // --- CONNECT SLIDERS ---
    connect(macroColorSlider, &QSlider::valueChanged, updateMacroPreview);
    connect(macroTextureSlider, &QSlider::valueChanged, updateMacroPreview);
    connect(macroBitcrushSlider, &QSlider::valueChanged, updateMacroPreview);
    connect(macroTimeSlider, &QSlider::valueChanged, updateMacroPreview);
    connect(macroWidthSlider, &QSlider::valueChanged, updateMacroPreview);
    connect(macroWonkySlider, &QSlider::valueChanged, updateMacroPreview);

    // PLAY BUTTON ---
    connect(btnPlayMacro, &QPushButton::toggled, [=](bool checked){
        if(!checked) {
            m_ghostSynth->setAudioSource([](double){ return 0.0; });
            m_ghostSynth->stop();
            btnPlayMacro->setText("▶ Play Preview");
            btnPlayMacro->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");
        } else {
            m_ghostSynth->start();
            btnPlayMacro->setText("⏹ Stop");
            btnPlayMacro->setStyleSheet("background-color: #338833; color: white; font-weight: bold; height: 40px;");
            updateMacroPreview();
        }
    });

    connect(btnGenMacro, &QPushButton::clicked, this, &MainWindow::generateMacroMorph);

    // Initial Trigger
    QTimer::singleShot(200, [=](){
        macroStyleCombo->setCurrentIndex(0);
        updateMacroPreview();
    });

    macroLayout->addStretch();
    modeTabs->addTab(macroTab, "Macro Morph");

    // ------------------------------------
    // TAB 23: STRING MACHINE
    // ------------------------------------
    QWidget *stringTab = new QWidget;
    QVBoxLayout *stringLayout = new QVBoxLayout(stringTab);

    // VISUALIZER (Must be created first!)
    stringScope = new UniversalScope();
    stringLayout->addWidget(stringScope);

    // ZOOM SLIDER
    auto *zoomLay = new QHBoxLayout();
    zoomLay->addWidget(new QLabel("Scope Zoom:"));
    stringZoomSlider = new QSlider(Qt::Horizontal);
    stringZoomSlider->setRange(0, 100);
    stringZoomSlider->setValue(10);
    zoomLay->addWidget(stringZoomSlider);
    stringLayout->addLayout(zoomLay);

    // DESCRIPTION
    QLabel *strDesc = new QLabel("<b>\nUse 'Age' to introduce vintage instability...</b>");
    strDesc->setStyleSheet("color: #333; padding: 10px; background-color: #e6f7ff; border-radius: 5px;");
    stringLayout->addWidget(strDesc);

    // MODEL & CHORD
    QGroupBox *strModelGroup = new QGroupBox("1. Core Sound");
    strModelGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #446688; margin-top: 10px; }");
    QFormLayout *strModelForm = new QFormLayout(strModelGroup);

    stringModelCombo = new QComboBox;
    stringModelCombo->addItems({
        "Solina String Ensemble (Classic)",
        "Crumar Performer (Brassy)",
        "Logan String Melody (Hollow)",
        "$tinkworx Aquatic Pad (Deep/PWM)",
        "Roland VP-330 (Choral)",
        "Amazing String (Saw Stack)"
    });

    stringChordCombo = new QComboBox;
    stringChordCombo->addItems({
        "OFF (Manual Play)",
        "Octave Stack (8' + 4')",
        "Fifth Stack (Power Chord)",
        "Minor 9th (Amazing Stack)",
        "$tinkworx Minor 11 (Deep)",
        "Sus4 (Spacey)"
    });

    strModelForm->addRow("Model Inspiration:", stringModelCombo);
    strModelForm->addRow("Chord Memory:", stringChordCombo);
    stringLayout->addWidget(strModelGroup);

    // EVOLUTION DASHBOARD (SLIDERS)
    QGroupBox *strDashGroup = new QGroupBox("2. Evolution & Motion");
    strDashGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #446688; background-color: #f0f8ff; margin-top: 10px; }");
    QGridLayout *strGrid = new QGridLayout(strDashGroup);

    auto addStrSlider = [&](QString name, QSlider*& slider, int row, int col, int def) {
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 100); slider->setValue(def);
        strGrid->addWidget(new QLabel(name), row * 2, col);
        strGrid->addWidget(slider, row * 2 + 1, col);
    };

    // Row 1: The "Visual Fix" and Chorus
    addStrSlider("Ensemble (Width)", stringEnsembleSlider, 0, 0, 60);
    addStrSlider("Phase Motion (Visual Fix)", stringMotionSlider, 0, 1, 20);

    // Row 2: Evolution
    addStrSlider("Attack (Vol Swell)", stringAttackSlider, 1, 0, 40);
    addStrSlider("Evolve (Filter Swell)", stringEvolveSlider, 1, 1, 50);

    // Row 3: Character
    addStrSlider("Vintage Age (Wobble)", stringAgeSlider, 2, 0, 10);
    addStrSlider("Space (Release)", stringSpaceSlider, 2, 1, 50);

    stringLayout->addWidget(strDashGroup);

    // PRESET LOGIC
    connect(stringModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index){
        auto setVals = [&](int ens, int att, int evo, int mot, int age, int spc) {
            stringEnsembleSlider->setValue(ens);
            stringAttackSlider->setValue(att);
            stringEvolveSlider->setValue(evo);
            stringMotionSlider->setValue(mot);
            stringAgeSlider->setValue(age);
            stringSpaceSlider->setValue(spc);
        };
        switch(index) {
        case 0: setVals(80, 40, 0,  20, 10, 50); break; // Solina
        case 1: setVals(60, 20, 70, 10, 5,  40); break; // Crumar
        case 2: setVals(90, 60, 30, 30, 20, 60); break; // Logan
        case 3: setVals(50, 50, 20, 80, 5,  80); break; // Aquatic
        case 4: setVals(100,30, 40, 10, 0,  40); break; // VP-330
        case 5: setVals(70, 10, 100,0,  0,  50); break; // Amazing
        }
    });

    // GENERATE BUTTON
    QPushButton *btnGenStr = new QPushButton("GENERATE STRING MACHINE");
    btnGenStr->setStyleSheet("font-weight: bold; font-size: 14px; background-color: #446688; color: white; height: 50px; margin-top: 10px;");
    connect(btnGenStr, &QPushButton::clicked, this, &MainWindow::generateStringMachine);
    stringLayout->addWidget(btnGenStr);

    // SIMULATION LOGIC
    auto updateStringScope = [=]() {
        if (!stringEnsembleSlider || !stringAttackSlider) return;

        double detune = stringEnsembleSlider->value() / 100.0;
        double attack = stringAttackSlider->value() / 100.0;
        double zoom = stringZoomSlider->value() / 100.0;

        std::function<double(double)> simFunc = [=](double t) {
            double f = 220.0;
            // Main Saw
            double phase1 = std::fmod(t * f, 1.0);
            double saw1 = (phase1 * 2.0) - 1.0;
            // Detuned Saw
            double f2 = f * (1.0 + (detune * 0.015));
            double phase2 = std::fmod(t * f2, 1.0);
            double saw2 = (phase2 * 2.0) - 1.0;
            // Mix
            double signal = (saw1 + saw2) * 0.5;
            // Attack Envelope
            if (attack > 0) {
                double attTime = 0.01 + (attack * 2.0);
                double env = (t < attTime) ? (t / attTime) : 1.0;
                signal *= env;
            }
            return signal;
        };
        stringScope->updateScope(simFunc, 2.0, zoom);
    };

    // CONNECT SIGNALS
    connect(stringEnsembleSlider, &QSlider::valueChanged, updateStringScope);
    connect(stringAttackSlider, &QSlider::valueChanged, updateStringScope);
    connect(stringZoomSlider, &QSlider::valueChanged, updateStringScope);
    connect(stringEvolveSlider, &QSlider::valueChanged, updateStringScope);

    // Trigger once to draw initial line
    QTimer::singleShot(100, [=](){ updateStringScope(); });

    stringLayout->addStretch();
    modeTabs->addTab(stringTab, "String Machine");

    // ------------------------------------
    // TAB 24: HARDWARE LAB
    // ------------------------------------
    QWidget *hwTab = new QWidget();
    QVBoxLayout *hwLayout = new QVBoxLayout(hwTab);

    QLabel *hwDisclaimer = new QLabel("⚠ HARDWARE LAB: DIRECT PARAMETER CONTROL\n"
                                      "This module maps 40 Analog-Style Presets. Legacy. WIP, testing external ADSR");
    hwDisclaimer->setStyleSheet("QLabel { color: #00ff78; font-weight: bold; font-size: 14px; border: 2px solid #00ff78; padding: 10px; background-color: #112211; }");
    hwDisclaimer->setAlignment(Qt::AlignCenter);
    hwLayout->addWidget(hwDisclaimer);

    adsrVisualizer = new EnvelopeDisplay();
    hwLayout->addWidget(adsrVisualizer);

    QFormLayout *hwForm = new QFormLayout();

    // --- HARDWARE LIBRARY ---
    hwPresetCombo = new QComboBox();
    hwPresetCombo->addItem("-- STUDIO CLASSICS --");
    hwPresetCombo->addItems({"01. Hissing Minimal (Signal)", "02. Analog Drift (Precision)", "03. Resonance Burner (Peak)",
                             "04. Metallic Tick (Percussion)", "05. PWM Rubber (Low-End)", "06. Power Saw (Lead)",
                             "07. Phase Mod (Keys)", "08. Deep Atmosphere (Pad)"});

    hwPresetCombo->addItem("-- MODULAR MINIMAL --");
    for(int i=9; i<=16; ++i) hwPresetCombo->addItem(QString("%1. Minimal Studio Tool %2").arg(i).arg(i-8));

    hwPresetCombo->addItem("-- INDUSTRIAL WAREHOUSE --");
    for(int i=17; i<=24; ++i) hwPresetCombo->addItem(QString("%1. Industrial Grit %2").arg(i).arg(i-16));

    hwPresetCombo->addItem("-- ETHEREAL DRIFT --");
    for(int i=25; i<=32; ++i) hwPresetCombo->addItem(QString("%1. Signal Drift %2").arg(i).arg(i-24));

    hwPresetCombo->addItem("-- SIGNAL GLITCH --");
    for(int i=33; i<=40; ++i) hwPresetCombo->addItem(QString("%1. Frequency Glitch %2").arg(i).arg(i-32));

    hwBaseWave = new QComboBox();
    hwBaseWave->addItems({"saww", "squarew", "trianglew", "sinew"});

    hwAttack = new QSlider(Qt::Horizontal); hwAttack->setRange(0, 100);
    hwDecay = new QSlider(Qt::Horizontal); hwDecay->setRange(0, 100);
    hwSustain = new QSlider(Qt::Horizontal); hwSustain->setRange(0, 100);
    hwRelease = new QSlider(Qt::Horizontal); hwRelease->setRange(0, 100);
    hwCutoff = new QSlider(Qt::Horizontal); hwCutoff->setRange(100, 14000);
    hwResonance = new QSlider(Qt::Horizontal); hwResonance->setRange(0, 100);

    hwPwmSpeed = new QSlider(Qt::Horizontal); hwPwmSpeed->setRange(0, 100);
    hwPwmDepth = new QSlider(Qt::Horizontal); hwPwmDepth->setRange(0, 100);
    hwVibSpeed = new QSlider(Qt::Horizontal); hwVibSpeed->setRange(0, 100);
    hwVibDepth = new QSlider(Qt::Horizontal); hwVibDepth->setRange(0, 100);
    hwNoiseMix = new QSlider(Qt::Horizontal); hwNoiseMix->setRange(0, 100);

    hwBaseNote = new QSpinBox(); hwBaseNote->setRange(0, 127); hwBaseNote->setValue(57); // Middle A
    hwPeakBoost = new QCheckBox("Resonance Peak Boost (Saturator)");

    hwForm->addRow("Preset Library:", hwPresetCombo);
    hwForm->addRow("Oscillator Wave:", hwBaseWave);
    hwForm->addRow("Attack Time:", hwAttack);
    hwForm->addRow("Decay Time:", hwDecay);
    hwForm->addRow("Sustain Level:", hwSustain);
    hwForm->addRow("Release Time:", hwRelease);
    hwForm->addRow("Filter Frequency:", hwCutoff);
    hwForm->addRow("Filter Q/Res:", hwResonance);
    hwForm->addRow("PWM LFO Speed:", hwPwmSpeed);
    hwForm->addRow("PWM LFO Depth:", hwPwmDepth);
    hwForm->addRow("Pitch Vibrato Speed:", hwVibSpeed);
    hwForm->addRow("Pitch Vibrato Depth:", hwVibDepth);
    hwForm->addRow("Signal Noise Mix:", hwNoiseMix);
    hwForm->addRow("Base MIDI Note:", hwBaseNote);
    hwForm->addRow(hwPeakBoost);

    hwLayout->addLayout(hwForm);

    QHBoxLayout *hwBtnLayout = new QHBoxLayout();
    QPushButton *btnRandHw = new QPushButton("RANDOMIZE HARDWARE");
    QPushButton *btnSaveHw = new QPushButton("SAVE PATCH .XPF");
    hwBtnLayout->addWidget(btnRandHw);
    hwBtnLayout->addWidget(btnSaveHw);
    hwLayout->addLayout(hwBtnLayout);

    modeTabs->addTab(hwTab, "Hardware Lab");

    // Live Visualiser Connection
    auto updateHwPreview = [=]() {
        adsrVisualizer->updateEnvelope(
            hwAttack->value() / 100.0,
            hwDecay->value() / 100.0,
            hwSustain->value() / 100.0,
            hwRelease->value() / 100.0
            );
    };

    connect(hwAttack, &QSlider::valueChanged, updateHwPreview);
    connect(hwDecay, &QSlider::valueChanged, updateHwPreview);
    connect(hwSustain, &QSlider::valueChanged, updateHwPreview);
    connect(hwRelease, &QSlider::valueChanged, updateHwPreview);
    connect(hwPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::loadHardwarePreset);
    connect(btnRandHw, &QPushButton::clicked, this, &MainWindow::generateRandomHardware);
    connect(btnSaveHw, &QPushButton::clicked, this, &MainWindow::generateHardwareXpf);

    // ------------------------------------
    // TAB 25: WEST COAST LAB
    // ------------------------------------
    QWidget *westTab = new QWidget();
    QVBoxLayout *westLayout = new QVBoxLayout(westTab);

    // VISUALISER
    westScope = new UniversalScope();
    westLayout->addWidget(westScope);

    // ZOOM SLIDER
    auto *westZoomLay = new QHBoxLayout();
    westZoomLay->addWidget(new QLabel("Oscilloscope Zoom:"));
    westZoomSlider = new QSlider(Qt::Horizontal);
    westZoomSlider->setRange(0, 100);
    westZoomSlider->setValue(20); // Default to seeing waves
    westZoomLay->addWidget(westZoomSlider);
    westLayout->addLayout(westZoomLay);

    // DESCRIPTION
    QLabel *westDesc = new QLabel("<b>WEST COAST LAB</b><br>Nonlinear waveshaping based on Buchla 259/208 topologies.");
    westDesc->setStyleSheet("color: #d4af37; padding: 10px; background-color: #222; border-radius: 5px;");
    westLayout->addWidget(westDesc);

    // CONTROLS
    QFormLayout *westForm = new QFormLayout();

    westBuildMode = new QComboBox();
    westBuildMode->addItems({"Legacy (Additive)", "Nightly (Nested)"});

    westModelSelect = new QComboBox();
    westModelSelect->addItems({
        "Model 259 (Parallel Deadband)",
        "Model 208 (Series Rectifier)",
        "Serge VCM (Odd Harmonic Sweep)",
        "Lockhart Folder (Differential Pair)"
    });

    westTimbreSlider = new QSlider(Qt::Horizontal); westTimbreSlider->setRange(1, 100); westTimbreSlider->setValue(30);
    westSymmetrySlider = new QSlider(Qt::Horizontal); westSymmetrySlider->setRange(-100, 100); westSymmetrySlider->setValue(0);
    westOrderSlider = new QSlider(Qt::Horizontal); westOrderSlider->setRange(0, 100); westOrderSlider->setValue(50);
    westVactrolSim = new QCheckBox("Vactrol 'LPG' Decay (Buchla Bongo)");
    westVactrolSim->setChecked(true);

    westForm->addRow("Build Mode:", westBuildMode);
    westForm->addRow("Circuit Topology:", westModelSelect);
    westForm->addRow("Timbre (Fold Gain):", westTimbreSlider);
    westForm->addRow("Symmetry (DC Bias):", westSymmetrySlider);
    westForm->addRow("Order (Harmonics):", westOrderSlider);
    westForm->addRow(westVactrolSim);

    westModFreqSlider = new QSlider(Qt::Horizontal); westModFreqSlider->setRange(1, 100); westModFreqSlider->setValue(10);
    westModIndexSlider = new QSlider(Qt::Horizontal); westModIndexSlider->setRange(0, 100); westModIndexSlider->setValue(0);
    westFoldStages = new QSlider(Qt::Horizontal); westFoldStages->setRange(1, 8); westFoldStages->setValue(3);
    westHalfWaveFold = new QCheckBox("Half-Wave Symmetry (Serge Bias)");

    westForm->addRow("Mod Osc Freq (Ratio):", westModFreqSlider);
    westForm->addRow("FM Index (Timbre Mod):", westModIndexSlider);
    westForm->addRow("Folding Stages (Series):", westFoldStages);
    westForm->addRow(westHalfWaveFold);

    westLayout->addLayout(westForm);

    // BUTTONS
    QPushButton *btnBongo = new QPushButton("LOAD BUCHLA BONGO PRESET");
    btnBongo->setStyleSheet("background-color: #444; color: #d4af37;");
    westLayout->addWidget(btnBongo);

    QPushButton *btnLiquid = new QPushButton("LOAD 259 LIQUID METAL");
    btnLiquid->setStyleSheet("background-color: #444; color: #00ffff;");
    westLayout->addWidget(btnLiquid);

    QPushButton *btnGenWest = new QPushButton("GENERATE WEST COAST FORMULA");
    btnGenWest->setStyleSheet("font-weight: bold; background-color: #d4af37; color: black; height: 40px;");
    westLayout->addWidget(btnGenWest);

    // VISUALISER LOGIC
    auto updateWestScope = [=]() {
        if (!westTimbreSlider || !westScope) return; // Safety

        // Fetch Values
        int model = westModelSelect->currentIndex();
        double gain = westTimbreSlider->value() / 10.0;
        double bias = westSymmetrySlider->value() / 100.0;
        double modRatio = westModFreqSlider->value() / 10.0;
        double modIndex = westModIndexSlider->value() / 10.0;
        double order = westOrderSlider->value() / 100.0;
        int stages = westFoldStages->value();
        bool useVactrol = westVactrolSim->isChecked();
        bool halfWave = westHalfWaveFold->isChecked();
        double zoom = westZoomSlider->value() / 100.0;

        // The Math Lambda
        std::function<double(double)> simFunc = [=](double t) {
            double baseFreq = 60.0; // Low frequency to see the folds clearly
            double pi = 3.14159265;

            // FM Modulator
            // Standard FM: sin(c*t + I*sin(m*t))
            double mod = modIndex * std::sin(t * baseFreq * modRatio * 2 * pi);

            // Core Oscillator (Triangle or Sine)
            // We use Phase Modulation (t + mod) to simulate FM for stability in the scope
            double phase = (t * baseFreq * 2 * pi) + mod;
            double core = 0.0;

            if (model == 1) { // Triangle (208)
                // Arccos of Cos gives a triangle wave roughly
                core = (2.0 / pi) * std::asin(std::sin(phase));
            } else { // Sine (259 / Serge)
                core = std::sin(phase);
            }

            // Pre-Conditioning (Gain + Bias)
            double signal = (core * gain) + bias;

            // Wavefolding Algorithms
            if (model == 2) { // Serge VCM (Iterative)
                for(int i = 0; i < stages; ++i) {
                    double thresh = 0.2 + (i * 0.1);
                    // Fold logic: x - 2 * clamp(...)
                    double cl = signal;
                    if (cl > thresh) cl = thresh;
                    if (cl < -thresh) cl = -thresh;
                    signal = signal - 2.0 * cl;
                }
            }
            else if (model == 0) { // Model 259 (Parallel Deadband)
                auto deadband = [](double x, double limit) {
                    if (x > limit) return limit;
                    if (x < -limit) return -limit;
                    return x;
                };
                // Parallel mix of different clipped versions creates complex folds
                signal = signal
                         - 2.0 * deadband(signal, 0.2)
                         + 2.0 * deadband(signal, 0.4)
                         - 2.0 * deadband(signal, 0.6);
            }
            else { // Model 208 (Rectifier/Crossfade)
                // (1-order)*signal + order*(folded)
                double folded = 2.0 * std::abs(signal) - 1.0; // Full rectification
                signal = ((1.0 - order) * signal) + (order * folded);
            }

            // Half-Wave
            if (halfWave) {
                if (signal < 0) signal = 0;
            }

            // Vactrol Envelope
            if (useVactrol) {
                double env = std::exp(-t * 15.0);
                signal *= env;
            }

            return signal;
        };

        // Update Scope
        westScope->updateScope(simFunc, 1.0, zoom);
    };

    // CONNECTIONS
    connect(westTimbreSlider, &QSlider::valueChanged, updateWestScope);
    connect(westSymmetrySlider, &QSlider::valueChanged, updateWestScope);
    connect(westOrderSlider, &QSlider::valueChanged, updateWestScope);
    connect(westModFreqSlider, &QSlider::valueChanged, updateWestScope);
    connect(westModIndexSlider, &QSlider::valueChanged, updateWestScope);
    connect(westFoldStages, &QSlider::valueChanged, updateWestScope);
    connect(westZoomSlider, &QSlider::valueChanged, updateWestScope);
    connect(westModelSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), updateWestScope);
    connect(westVactrolSim, &QCheckBox::toggled, updateWestScope);
    connect(westHalfWaveFold, &QCheckBox::toggled, updateWestScope);

    // Connect Generator Buttons
    connect(btnGenWest, &QPushButton::clicked, this, &MainWindow::generateWestCoast);

    // Presets need to trigger the update manually after setting values
    connect(btnBongo, &QPushButton::clicked, [=](){
        westModelSelect->setCurrentIndex(0); // 259 Parallel
        westTimbreSlider->setValue(85);      // High Gain
        westSymmetrySlider->setValue(15);    // Slight Offset
        westVactrolSim->setChecked(true);    // Natural Decay
        updateWestScope();
    });

    connect(btnLiquid, &QPushButton::clicked, [=](){
        westModelSelect->setCurrentIndex(0); // 259
        westModFreqSlider->setValue(45);     // High ratio
        westModIndexSlider->setValue(60);    // Deep FM
        westTimbreSlider->setValue(70);      // Heavy Fold
        westVactrolSim->setChecked(false);
        updateWestScope();
    });

    // Initial Draw
    QTimer::singleShot(200, [=](){ updateWestScope(); });

    westLayout->addStretch();
    modeTabs->addTab(westTab, "West Coast Lab");

    // ------------------------------------
    // TAB 26. MODULAR SYNTH (EXTERNAL CODE)
    // ------------------------------------
    ModularSynthTab* modularTab = new ModularSynthTab(this);

    // Handle Text Code Generation
    connect(modularTab, &ModularSynthTab::expressionGenerated, this, [=](QString code){
        statusBox->setText(code);
        QApplication::clipboard()->setText(code);
    });

    // Handle PLAY Request
    connect(modularTab, &ModularSynthTab::startPreview, this, [=](std::function<double(double)> func){
        m_ghostSynth->setAudioSource(func);
        m_ghostSynth->start();
    });

    // Handle STOP Request
    connect(modularTab, &ModularSynthTab::stopPreview, this, [=](){
        m_ghostSynth->setAudioSource([](double){ return 0.0; });
        m_ghostSynth->stop();
    });

    modeTabs->addTab(modularTab, "Modular Grid");


    // -------------------------------------
    // TAB 27: SPECTRAL RESYNTHESISER
    // -------------------------------------
    QWidget *specTab = new QWidget();
        auto *specLayout = new QVBoxLayout(specTab);

        // THE SCOPE ---
        specScope = new UniversalScope();
        specScope->setMinimumHeight(180);
        specLayout->addWidget(specScope);

        // SETTINGS FORM ---
        auto *specForm = new QFormLayout();

        specBuildMode = new QComboBox();
        specBuildMode->addItems({"Nightly (Nested Variables)", "Legacy (Additive Inline)"});

        specWindowRes = new QDoubleSpinBox();
        specWindowRes->setRange(1, 500);
        specWindowRes->setValue(10); // Default to 10 as requested
        specWindowRes->setSuffix(" Analysis Windows");

        specTopHarmonics = new QSpinBox();
        specTopHarmonics->setRange(1, 64); specTopHarmonics->setValue(8);
        specTopHarmonics->setSuffix(" Harmonics");

        specDecaySlider = new QSlider(Qt::Horizontal);
        specDecaySlider->setRange(1, 100);
        specDecaySlider->setValue(25);

        specForm->addRow("Build Mode:", specBuildMode);
        specForm->addRow("Time Resolution:", specWindowRes);
        specForm->addRow("Spectral Detail:", specTopHarmonics);
        specForm->addRow("Decay Speed:", specDecaySlider);
        specLayout->addLayout(specForm);

        // BUTTON & SELECTOR ROW ---
        auto *specBtnLay = new QHBoxLayout();

        specPitchCombo = new QComboBox();
        specPitchCombo->addItems({"C-3", "G-3", "A-3", "C-4", "E-4", "A-4", "C-5"});
        specPitchCombo->setCurrentText("A-4");
        specBtnLay->addWidget(new QLabel("Pitch:"));
        specBtnLay->addWidget(specPitchCombo);

        btnDeChord = new QPushButton("De-Chord: OFF");
        btnDeChord->setCheckable(true);
        btnDeChord->setStyleSheet("background-color: #444; color: white; height: 35px;");
        specBtnLay->addWidget(btnDeChord);

        QPushButton *btnLoadSpec = new QPushButton("Load Stab (.wav)");
        btnLoadSpec->setStyleSheet("height: 35px;");
        specBtnLay->addWidget(btnLoadSpec);

        btnPlaySpec = new QPushButton("▶ Play Preview");
        btnPlaySpec->setCheckable(true);
        btnPlaySpec->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 35px;");
        specBtnLay->addWidget(btnPlaySpec);

        specLayout->addLayout(specBtnLay);

        // EXPRESSION BOX ---
        specExpressionBox = new QTextEdit();
        specExpressionBox->setPlaceholderText("Generated spectral formula will appear here...");
        specExpressionBox->setMaximumHeight(100);
        specExpressionBox->setReadOnly(true);
        specLayout->addWidget(specExpressionBox);

        modeTabs->addTab(specTab, "Spectral Resynth");

        // CONNECTIONS ---
        connect(specWindowRes, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](){ updateSpectralPreview(); });
        connect(specTopHarmonics, QOverload<int>::of(&QSpinBox::valueChanged), [=](){ updateSpectralPreview(); });
        connect(specBuildMode, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](){ updateSpectralPreview(); });
        connect(specPitchCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](){ updateSpectralPreview(); });
        connect(specDecaySlider, &QSlider::valueChanged, [=](){ updateSpectralPreview(); });

        connect(btnDeChord, &QPushButton::toggled, [=](bool checked){
            m_deChordEnabled = checked;
            btnDeChord->setText(checked ? "De-Chord: ON (Mono)" : "De-Chord: OFF");
            btnDeChord->setStyleSheet(checked ? "background-color: #0066ff; color: white;" : "background-color: #444; color: white;");
            updateSpectralPreview();
        });

        connect(btnLoadSpec, &QPushButton::clicked, this, [=](){
            loadWav();
            if(!originalData.empty()) {
                specSampleData = originalData;
                if(specSampleData.size() > (fileFs * 2)) specSampleData.resize(fileFs * 2);
                updateSpectralPreview();
            }
        });

        connect(btnPlaySpec, &QPushButton::toggled, [=](bool checked){
            if(!checked) {
                m_ghostSynth->stop();
                btnPlaySpec->setText("▶ Play Preview");
                btnPlaySpec->setStyleSheet("background-color: #335533; color: white;");
            } else {
                m_ghostSynth->start();
                btnPlaySpec->setText("⏹ Stop");
                btnPlaySpec->setStyleSheet("background-color: #338833; color: white;");
                updateSpectralPreview();
            }
        });

        // -------------------------------------
        // TAB 28: SUBTRACTIVE LAB
        // -------------------------------------
        QWidget *subTab = new QWidget();
        QVBoxLayout *subMainLayout = new QVBoxLayout(subTab);

        // TOP SECTION: THE ANALYSERS
        QHBoxLayout *vizLayout = new QHBoxLayout();
        subScope = new UniversalScope();     // Waveform
        subSpectrum = new UniversalScope();  // Spectral Analyzer
        vizLayout->addWidget(subScope);
        vizLayout->addWidget(subSpectrum);
        subMainLayout->addLayout(vizLayout);

        // THE CONTROL RACK (Horizontal Flow)
        QHBoxLayout *rackLayout = new QHBoxLayout();

        // OSCILLATORS (Rotary Dials) ---
        QGroupBox *oscUnit = new QGroupBox("1. OSCILLATORS");
        QGridLayout *oscGrid = new QGridLayout(oscUnit);

        subOsc1Dial = new QDial(); subOsc1Dial->setRange(0, 3); subOsc1Dial->setNotchesVisible(true);
        subOsc2Dial = new QDial(); subOsc2Dial->setRange(0, 3); subOsc2Dial->setNotchesVisible(true);
        subDetuneSlider = new QSlider(Qt::Horizontal); subDetuneSlider->setRange(0, 50);

        oscGrid->addWidget(new QLabel("Osc 1 Type"), 0, 0); oscGrid->addWidget(subOsc1Dial, 1, 0);
        oscGrid->addWidget(new QLabel("Osc 2 Type"), 0, 1); oscGrid->addWidget(subOsc2Dial, 1, 1);
        oscGrid->addWidget(new QLabel("Detune Osc 2"), 2, 0, 1, 2); oscGrid->addWidget(subDetuneSlider, 3, 0, 1, 2);
        rackLayout->addWidget(oscUnit);

        // UNIT 2: MIXER ---
        QGroupBox *mixUnit = new QGroupBox("2. MIXER");
        QVBoxLayout *mixVLay = new QVBoxLayout(mixUnit);
        subMixSlider = new QSlider(Qt::Vertical); subMixSlider->setRange(0, 100); subMixSlider->setValue(50);
        subShowMathToggle = new QCheckBox("Show Math Overlay");
        subShowMathToggle->setChecked(true);
        mixVLay->addWidget(subMixSlider, 0, Qt::AlignCenter);
        mixVLay->addWidget(new QLabel("Mix O1/O2"), 0, Qt::AlignCenter);
        mixVLay->addWidget(subShowMathToggle);
        rackLayout->addWidget(mixUnit);

        // UNIT 3: FULL ADSR ENVELOPE ---

        QGroupBox *adsrUnit = new QGroupBox("3. ADSR AMPLIFIER");
        QHBoxLayout *adsrHLay = new QHBoxLayout(adsrUnit);
        subAttSlider = new QSlider(Qt::Vertical); subDecSlider = new QSlider(Qt::Vertical);
        subSusSlider = new QSlider(Qt::Vertical); subRelSlider = new QSlider(Qt::Vertical);
        for(auto* s : {subAttSlider, subDecSlider, subSusSlider, subRelSlider}) s->setRange(0, 100);
        subSusSlider->setValue(70); subDecSlider->setValue(30); subRelSlider->setValue(20);

        auto addAdsrKnob = [&](QString name, QSlider* s) {
            QVBoxLayout *v = new QVBoxLayout(); v->addWidget(s, 0, Qt::AlignCenter);
            v->addWidget(new QLabel(name), 0, Qt::AlignCenter); adsrHLay->addLayout(v);
        };
        addAdsrKnob("A", subAttSlider); addAdsrKnob("D", subDecSlider);
        addAdsrKnob("S", subSusSlider); addAdsrKnob("R", subRelSlider);
        rackLayout->addWidget(adsrUnit);

        subMainLayout->addLayout(rackLayout);

        // MASTER OUTPUT BUTTON
        btnSubPlayMaster = new QPushButton("▶ START SYNTH ENGINE");
        btnSubPlayMaster->setCheckable(true);
        btnSubPlayMaster->setStyleSheet("height: 60px; font-weight: bold; background-color: #335533; color: white;");
        subMainLayout->addWidget(btnSubPlayMaster);

        modeTabs->addTab(subTab, "Subtractive Lab");

        // CONNECTIONS (Loop through for efficiency)
        QList<QWidget*> subWidgets = {subOsc1Dial, subOsc2Dial, subDetuneSlider, subMixSlider,
                                          subAttSlider, subDecSlider, subSusSlider, subRelSlider};
        for(auto* w : subWidgets) {
            if(auto* d = qobject_cast<QDial*>(w)) connect(d, &QDial::valueChanged, this, &MainWindow::updateSubtractivePreview);
            if(auto* s = qobject_cast<QSlider*>(w)) connect(s, &QSlider::valueChanged, this, &MainWindow::updateSubtractivePreview);
        }
        connect(subShowMathToggle, &QCheckBox::toggled, this, &MainWindow::updateSubtractivePreview);

        connect(btnSubPlayMaster, &QPushButton::toggled, [=](bool checked){
            if(checked) {
                m_ghostSynth->start(); // Use the existing SynthEngine
                btnSubPlayMaster->setText("⏹ STOP ENGINE");
                btnSubPlayMaster->setStyleSheet("background-color: #883333; color: white;");
            } else {
                m_ghostSynth->stop();
                btnSubPlayMaster->setText("▶ START SYNTH ENGINE");
                btnSubPlayMaster->setStyleSheet("background-color: #335533; color: white;");
            }
            updateSubtractivePreview();
        });


        // ------------------------------------
        // TAB 29: PIXEL SYNTH
        // ------------------------------------
        pixelTab = new QWidget();
        QVBoxLayout *pixelLayout = new QVBoxLayout(pixelTab);

        // IMAGE PREVIEW AREA
        pixelPreviewLabel = new QLabel("No Image Loaded");
        pixelPreviewLabel->setAlignment(Qt::AlignCenter);
        pixelPreviewLabel->setStyleSheet("border: 2px dashed #666; background-color: #222; color: #AAA;");
        pixelPreviewLabel->setMinimumHeight(200);
        pixelPreviewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        pixelLayout->addWidget(pixelPreviewLabel);

        // SETTINGS GROUP
        QGroupBox *pixSettings = new QGroupBox("Resolution & Physics");
        QGridLayout *pixGrid = new QGridLayout(pixSettings);

        pixelBuildMode = new QComboBox();
        pixelBuildMode->addItems({"Nightly (Nested Variables)", "Legacy (Additive Chain)"});

        pixelTimeSteps = new QSpinBox();
        pixelTimeSteps->setRange(4, 256); pixelTimeSteps->setValue(32);
        pixelTimeSteps->setSuffix(" Time Slices");
        pixelTimeSteps->setToolTip("How many slices to cut the image into (X-Axis).");

        pixelFreqBands = new QSpinBox();
        pixelFreqBands->setRange(10, 1000); pixelFreqBands->setValue(100);
        pixelFreqBands->setSuffix(" Freq Bands");
        pixelFreqBands->setToolTip("Vertical resolution (Y-Axis).");

        pixelMaxPartials = new QSlider(Qt::Horizontal);
        pixelMaxPartials->setRange(1, 50); pixelMaxPartials->setValue(10);
        QLabel *lblPartials = new QLabel("Max Voices: 10");
        connect(pixelMaxPartials, &QSlider::valueChanged, [=](int v){ lblPartials->setText(QString("Max Voices: %1").arg(v)); });

        pixelDuration = new QDoubleSpinBox(); pixelDuration->setValue(2.0); pixelDuration->setSuffix(" sec");

        pixelMinFreq = new QDoubleSpinBox(); pixelMinFreq->setRange(20, 1000); pixelMinFreq->setValue(100); pixelMinFreq->setPrefix("Low: ");
        pixelMaxFreq = new QDoubleSpinBox(); pixelMaxFreq->setRange(100, 20000); pixelMaxFreq->setValue(5000); pixelMaxFreq->setPrefix("High: ");

        pixelLogScale = new QCheckBox("Logarithmic Scale (Musical)");
        pixelLogScale->setChecked(true);

        pixGrid->addWidget(new QLabel("Build Mode:"), 0, 0); pixGrid->addWidget(pixelBuildMode, 0, 1);
        pixGrid->addWidget(new QLabel("Duration:"), 0, 2);   pixGrid->addWidget(pixelDuration, 0, 3);

        pixGrid->addWidget(new QLabel("X-Res (Time):"), 1, 0); pixGrid->addWidget(pixelTimeSteps, 1, 1);
        pixGrid->addWidget(new QLabel("Y-Res (Freq):"), 1, 2); pixGrid->addWidget(pixelFreqBands, 1, 3);

        pixGrid->addWidget(lblPartials, 2, 0);      pixGrid->addWidget(pixelMaxPartials, 2, 1);
        pixGrid->addWidget(pixelLogScale, 2, 2);

        QHBoxLayout *freqLay = new QHBoxLayout();
        freqLay->addWidget(pixelMinFreq); freqLay->addWidget(pixelMaxFreq);
        pixGrid->addLayout(freqLay, 3, 0, 1, 4);

        pixelLayout->addWidget(pixSettings);

        // BUTTONS
        QHBoxLayout *pixBtnLay = new QHBoxLayout();

        btnLoadPixel = new QPushButton("Load Image (JPG/PNG)");
        btnLoadPixel->setStyleSheet("height: 40px;");

        btnGenPixel = new QPushButton("GENERATE FORMULA");
        btnGenPixel->setStyleSheet("height: 40px; background-color: #444; color: white; font-weight: bold;");

        btnPlayPixel = new QPushButton("▶ Play");
        btnPlayPixel->setCheckable(true);
        btnPlayPixel->setStyleSheet("height: 40px; background-color: #335533; color: white; font-weight: bold;");

        pixBtnLay->addWidget(btnLoadPixel);
        pixBtnLay->addWidget(btnGenPixel);
        pixBtnLay->addWidget(btnPlayPixel);
        pixelLayout->addLayout(pixBtnLay);

        modeTabs->addTab(pixelTab, "Pixel Synth");

        // CONNECTIONS
        connect(btnLoadPixel, &QPushButton::clicked, [=](){
            QString path = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.jpg *.bmp)");
            if(path.isEmpty()) return;
            pixelLoadedImage.load(path);
            // Show Preview (Scaled to fit)
            pixelPreviewLabel->setPixmap(QPixmap::fromImage(pixelLoadedImage).scaled(pixelPreviewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            pixelPreviewLabel->setText("");
            });

        connect(btnGenPixel, &QPushButton::clicked, this, &MainWindow::generatePixelSynth);

        connect(btnPlayPixel, &QPushButton::toggled, [=](bool checked){
            if(checked) {
                m_ghostSynth->start();
            } else {
                m_ghostSynth->stop();
            }
        });

        // ------------------------------------
        // TAB 30: DJ SCRATCH GENERATOR
        // ------------------------------------
        QWidget *scratchTab = new QWidget();
        QVBoxLayout *scratchLayout = new QVBoxLayout(scratchTab);

        // Visualizer
        scratchScope = new UniversalScope();
        scratchScope->setMinimumHeight(180);
        scratchLayout->addWidget(scratchScope);

        QGroupBox *scratchCtrlGroup = new QGroupBox("Turntable Physics");
        QFormLayout *scratchForm = new QFormLayout(scratchCtrlGroup);

        scratchBuildMode = new QComboBox();
        scratchBuildMode->addItems({"Nightly (Variables)", "Legacy (Additive)"});

        scratchPatternCombo = new QComboBox();
        scratchPatternCombo->addItems({"Baby Scratch", "Transformer", "Chirp", "Flare"});

        scratchVinylPitch = new QSlider(Qt::Horizontal);
        scratchVinylPitch->setRange(40, 440); scratchVinylPitch->setValue(220);

        scratchFaderSpeed = new QSlider(Qt::Horizontal);
        scratchFaderSpeed->setRange(1, 20); scratchFaderSpeed->setValue(6);

        scratchGritSlider = new QSlider(Qt::Horizontal);
        scratchGritSlider->setRange(0, 100); scratchGritSlider->setValue(15);

        scratchForm->addRow("Build Mode:", scratchBuildMode);
        scratchForm->addRow("Scratch Technique:", scratchPatternCombo);
        scratchForm->addRow("Vinyl Pitch (f):", scratchVinylPitch);
        scratchForm->addRow("Hand Speed (Hz):", scratchFaderSpeed);
        scratchForm->addRow("Groove Grit:", scratchGritSlider);
        scratchLayout->addWidget(scratchCtrlGroup);

        // Buttons
        btnPlayScratch = new QPushButton("▶ Test Scratch Loop");
        btnPlayScratch->setCheckable(true);
        btnPlayScratch->setStyleSheet("height: 40px; background-color: #333355; color: white; font-weight: bold;");

        QPushButton *btnGenScratch = new QPushButton("GENERATE SCRATCH FORMULA");
        btnGenScratch->setStyleSheet("height: 40px; font-weight: bold;");

        scratchLayout->addWidget(btnPlayScratch);
        scratchLayout->addWidget(btnGenScratch);
        modeTabs->addTab(scratchTab, "Scratch Lab");

        // --- CONNECTIONS (The "Wires") ---
        connect(btnGenScratch, &QPushButton::clicked, this, &MainWindow::generateScratchLogic);

        // Connect sliders to update the Scope in real-time
        connect(scratchVinylPitch, &QSlider::valueChanged, [=](){ updateScratchPreview(); });
        connect(scratchFaderSpeed, &QSlider::valueChanged, [=](){ updateScratchPreview(); });
        connect(scratchGritSlider, &QSlider::valueChanged, [=](){ updateScratchPreview(); });
        connect(scratchPatternCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](){ updateScratchPreview(); });

        connect(btnPlayScratch, &QPushButton::toggled, [=](bool checked){
            if(checked) {
                m_ghostSynth->start();
                updateScratchPreview();
            } else {
                m_ghostSynth->stop();
            }
        });

        // ---------------------------------------------------------
        // TAB 31: NATURE MEGA LAB
        // ---------------------------------------------------------
        QWidget *natureTab = new QWidget();
        QVBoxLayout *natureLayout = new QVBoxLayout(natureTab);

        // TOP BAR: Mode & Build Type
        QGroupBox *natureModeBox = new QGroupBox("Environment Settings");
        QHBoxLayout *natureModeLayout = new QHBoxLayout(natureModeBox);

        natureTypeCombo = new QComboBox();
        natureTypeCombo->addItems({
                "Forest Birds (FM)",
                "Night Crickets (Pulse)",
                "Guinea Pig Herd (Harmonic)",
                "Rushing Stream (Granular)",
                "Rainstorm (Stochastic)",
                "Thunder & Rumble (Explosive)",
                "Swamp Frogs (Resonant)"
        });

        natureBuildMode = new QComboBox();
        natureBuildMode->addItems({"Nightly (Variables)", "Legacy (Additive)"});

        natureModeLayout->addWidget(new QLabel("Model:"));
        natureModeLayout->addWidget(natureTypeCombo, 1);
        natureModeLayout->addWidget(new QLabel("Build:"));
        natureModeLayout->addWidget(natureBuildMode, 1);
        natureLayout->addWidget(natureModeBox);

        // The Mega Slider Bank
        QGroupBox *paramBox = new QGroupBox("Physics Parameters");
        QGridLayout *paramGrid = new QGridLayout(paramBox);

        for(int i=0; i<8; i++) {
            natureLabels[i] = new QLabel(QString("Param %1").arg(i+1));
            natureLabels[i]->setAlignment(Qt::AlignCenter);
                natureLabels[i]->setStyleSheet("font-size: 10px; color: #555;");

                // Dynamic pointers to our class members
                QSlider** targetSlider = nullptr;
                switch(i) {
                    case 0: targetSlider = &natureParam1; break;
                    case 1: targetSlider = &natureParam2; break;
                    case 2: targetSlider = &natureParam3; break;
                    case 3: targetSlider = &natureParam4; break;
                    case 4: targetSlider = &natureParam5; break;
                    case 5: targetSlider = &natureParam6; break;
                    case 6: targetSlider = &natureParam7; break;
                    case 7: targetSlider = &natureParam8; break;
                }

                *targetSlider = new QSlider(Qt::Vertical);
                (*targetSlider)->setRange(0, 100);
                (*targetSlider)->setValue(50);

                // Add to grid (2 rows of 4)
                int row = (i < 4) ? 0 : 2;
                int col = i % 4;

                paramGrid->addWidget(*targetSlider, row, col, Qt::AlignHCenter);
                paramGrid->addWidget(natureLabels[i], row+1, col, Qt::AlignHCenter);

                // Connect update
                connect(*targetSlider, &QSlider::valueChanged, this, &MainWindow::generateNatureLogic);
            }
            natureLayout->addWidget(paramBox);

            // Visuals & Controls
            natureScope = new UniversalScope();
            natureScope->setMinimumHeight(120);
            natureLayout->addWidget(natureScope);

            // BUTTON LAYOUT
            QHBoxLayout *natureBtnLay = new QHBoxLayout();

            btnPlayNature = new QPushButton("▶ Play Preview");
            btnPlayNature->setCheckable(true);
            btnPlayNature->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");

            btnGenNature = new QPushButton("GENERATE STRING");
            btnGenNature->setStyleSheet("background-color: #444; color: white; font-weight: bold; height: 40px;");

            natureBtnLay->addWidget(btnPlayNature);
            natureBtnLay->addWidget(btnGenNature);
            natureLayout->addLayout(natureBtnLay);

            // CONNECTIONS
            connect(natureTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index){
                updateNatureLabels(index);
                generateNatureLogic();
            });

            // Play Toggle Logic
            connect(btnPlayNature, &QPushButton::toggled, [=](bool checked){
                if(!checked) {
                    m_ghostSynth->setAudioSource([](double){ return 0.0; });
                    m_ghostSynth->stop();
                    btnPlayNature->setText("▶ Play Preview");
                    btnPlayNature->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");
                } else {
                    m_ghostSynth->start();
                    btnPlayNature->setText("⏹ Stop");
                    btnPlayNature->setStyleSheet("background-color: #338833; color: white; font-weight: bold; height: 40px;");
                    generateNatureLogic(); // Trigger sound
                }
            });

            connect(btnGenNature, &QPushButton::clicked, this, &MainWindow::generateNatureLogic);

            modeTabs->addTab(natureTab, "Nature Lab");

            // Initialize labels
            updateNatureLabels(0);

        // ---------------------------------------------------------
        // TAB 32: VECTOR MORPH (XY SYNTHESIS)
        // ---------------------------------------------------------
                vectorTab = new QWidget();
                QVBoxLayout *vectorLayout = new QVBoxLayout(vectorTab);

                // 1. VISUALIZER
                UniversalScope *vectorScope = new UniversalScope();
                vectorScope->setMinimumHeight(150);
                vectorLayout->addWidget(vectorScope);

                // 2. CONTROLS
                QGroupBox *vecCtrlGroup = new QGroupBox("4-Point Vector Plane (A/B/C/D)");
                QGridLayout *vecGrid = new QGridLayout(vecCtrlGroup);

                // Re-using header pointers for X/Y
                morphX = new QSlider(Qt::Horizontal); morphX->setRange(0, 100); morphX->setValue(50);
                morphY = new QSlider(Qt::Horizontal); morphY->setRange(0, 100); morphY->setValue(50);

                // Additional "Cool" Sliders for animation
                QSlider *vecLfoRate = new QSlider(Qt::Horizontal); vecLfoRate->setRange(0, 100); vecLfoRate->setValue(20);
                QSlider *vecLfoDepth = new QSlider(Qt::Horizontal); vecLfoDepth->setRange(0, 100); vecLfoDepth->setValue(0);
                QSlider *vecDetune = new QSlider(Qt::Horizontal); vecDetune->setRange(0, 100); vecDetune->setValue(10);

                vecGrid->addWidget(new QLabel("<b>X-AXIS (Saw <-> Sqr):</b>"), 0, 0);
                vecGrid->addWidget(morphX, 0, 1);
                vecGrid->addWidget(new QLabel("<b>Y-AXIS (Tri <-> FM):</b>"), 1, 0);
                vecGrid->addWidget(morphY, 1, 1);

                vecGrid->addWidget(new QLabel("Orbit Speed (LFO):"), 2, 0);
                vecGrid->addWidget(vecLfoRate, 2, 1);
                vecGrid->addWidget(new QLabel("Orbit Depth:"), 3, 0);
                vecGrid->addWidget(vecLfoDepth, 3, 1);
                vecGrid->addWidget(new QLabel("Stereo Spread:"), 4, 0);
                vecGrid->addWidget(vecDetune, 4, 1);

                vectorLayout->addWidget(vecCtrlGroup);

                // 3. BUTTONS
                QHBoxLayout *vecBtnLay = new QHBoxLayout();
                QPushButton *btnPlayVector = new QPushButton("▶ Play Vector Field");
                btnPlayVector->setCheckable(true);
                btnPlayVector->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 45px;");

                btnGenVector = new QPushButton("GENERATE VECTOR STRING");
                btnGenVector->setStyleSheet("background-color: #444; color: white; font-weight: bold; height: 45px;");

                vecBtnLay->addWidget(btnPlayVector);
                vecBtnLay->addWidget(btnGenVector);
                vectorLayout->addLayout(vecBtnLay);

                // 4. LOGIC
                auto updateVector = [=]() {
                    double xVal = morphX->value() / 100.0;
                    double yVal = morphY->value() / 100.0;
                    double lfoSpd = vecLfoRate->value() / 5.0;
                    double lfoDep = vecLfoDepth->value() / 200.0; // +/- 0.5 max
                    double detune = vecDetune->value() / 500.0;

                    std::function<double(double)> vecAlgo = [=](double t) {
                        // LFO Orbit Logic
                        double orbit = std::sin(t * lfoSpd);
                        double modX = std::clamp(xVal + (orbit * lfoDep), 0.0, 1.0);
                        double modY = std::clamp(yVal + (orbit * lfoDep * 0.5), 0.0, 1.0); // Elliptical orbit

                        double f = 110.0;
                        // 4 Corners
                        // A (0,0): Sawtooth
                        double oscA = 2.0 * std::fmod(t * f, 1.0) - 1.0;
                        // B (1,0): Square
                        double oscB = (std::sin(t * f * 6.28) > 0) ? 1.0 : -1.0;
                        // C (0,1): Triangle
                        double oscC = (2.0/3.14) * std::asin(std::sin(t * f * 6.28));
                        // D (1,1): FM Noise/Bell
                        double oscD = std::sin(t * f * 6.28 + 3.0 * std::sin(t * f * 2.5 * 6.28));

                        // Bilinear Interpolation
                        double top = (oscA * (1.0 - modX)) + (oscB * modX);
                        double bot = (oscC * (1.0 - modX)) + (oscD * modX);
                        double signal = (top * (1.0 - modY)) + (bot * modY);

                        return signal;
                    };

                    vectorScope->updateScope(vecAlgo, 0.05, 1.0);
                    if(btnPlayVector->isChecked()) m_ghostSynth->setAudioSource(vecAlgo);
                };

                // 5. GENERATOR
                connect(btnGenVector, &QPushButton::clicked, [=]() {
                    // Generates an Xpressive formula using variables for X/Y
                    QString code = QString(
                        "var vx := %1; var vy := %2;\n"
                        "var A := saww(integrate(f));\n"
                        "var B := squarew(integrate(f));\n"
                        "var C := trianglew(integrate(f));\n"
                        "var D := sinew(integrate(f) + 3*sinew(integrate(f*2.5)));\n"
                        "clamp(-1, ((A*(1-vx) + B*vx)*(1-vy) + (C*(1-vx) + D*vx)*vy), 1)"
                    ).arg(morphX->value()/100.0).arg(morphY->value()/100.0);
                    statusBox->setText(code);
                    QApplication::clipboard()->setText(code);
                });

                // Connections
                connect(morphX, &QSlider::valueChanged, updateVector);
                connect(morphY, &QSlider::valueChanged, updateVector);
                connect(vecLfoRate, &QSlider::valueChanged, updateVector);
                connect(vecLfoDepth, &QSlider::valueChanged, updateVector);
                connect(vecDetune, &QSlider::valueChanged, updateVector);

                connect(btnPlayVector, &QPushButton::toggled, [=](bool c){
                    if(c) { m_ghostSynth->start(); updateVector(); btnPlayVector->setText("⏹ Stop"); }
                    else { m_ghostSynth->stop(); btnPlayVector->setText("▶ Play Vector Field"); }
                });

                modeTabs->addTab(vectorTab, "Vector Morph");
                QTimer::singleShot(200, updateVector);
    // ---------------------------------------------------------
    // TAB 33: PLUCK LAB
    // ---------------------------------------------------------
    pluckTab = new QWidget();
    QVBoxLayout *pluckLayout = new QVBoxLayout(pluckTab);

    // VISUALISER
    UniversalScope *pluckScope = new UniversalScope();
    pluckScope->setMinimumHeight(150);
    pluckLayout->addWidget(pluckScope);

    //CONTROLS
    QGroupBox *pluckCtrlGroup = new QGroupBox("Physical String Modeling");
    QFormLayout *pluckForm = new QFormLayout(pluckCtrlGroup);

    // Header pointers
    pluckDamping = new QSlider(Qt::Horizontal); pluckDamping->setRange(0, 100); pluckDamping->setValue(50);
    pluckDecay = new QSlider(Qt::Horizontal); pluckDecay->setRange(80, 100); pluckDecay->setValue(99);

    QSlider *pluckColor = new QSlider(Qt::Horizontal); pluckColor->setRange(0, 100); pluckColor->setValue(80);
    QSlider *pluckTension = new QSlider(Qt::Horizontal); pluckTension->setRange(0, 100); pluckTension->setValue(0);
    QSlider *pluckDrive = new QSlider(Qt::Horizontal); pluckDrive->setRange(100, 200); pluckDrive->setValue(100);

    pluckForm->addRow("String Damping (LPF):", pluckDamping);
    pluckForm->addRow("Sustain / Feedback:", pluckDecay);
    pluckForm->addRow("Exciter Brightness:", pluckColor);
    pluckForm->addRow("String Tension (Bend):", pluckTension);
    pluckForm->addRow("Overdrive:", pluckDrive);

    pluckLayout->addWidget(pluckCtrlGroup);

    // BUTTONS
    QHBoxLayout *pluckBtnLay = new QHBoxLayout();
    QPushButton *btnPlayPluck = new QPushButton("▶ Pluck String");
    btnPlayPluck->setCheckable(true);
    btnPlayPluck->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 45px;");

    btnGenPluck = new QPushButton("GENERATE PLUCK STRING");
    btnGenPluck->setStyleSheet("background-color: #444; color: white; font-weight: bold; height: 45px;");

    pluckBtnLay->addWidget(btnPlayPluck);
    pluckBtnLay->addWidget(btnGenPluck);
    pluckLayout->addLayout(pluckBtnLay);

    // LOGIC
                    auto updatePluck = [=]() {
                        double damp = pluckDamping->value() / 100.0; // 0.0 to 1.0
                        double fbk = pluckDecay->value() / 100.0;    // 0.8 to 1.0
                        double color = pluckColor->value() / 100.0;
                        double bend = pluckTension->value();
                        double drive = pluckDrive->value() / 100.0;

                        std::function<double(double)> pluckAlgo = [=](double t) {
                            // Simulate the burst
                            double burstLen = 0.01;
                            double burst = (t < burstLen) ? ((double)rand()/RAND_MAX * 2.0 - 1.0) : 0.0;

                            // Apply Exciter Color (Simple Lowpass on burst)
                            if (t > 0.005 && color < 0.5) burst *= 0.2;

                            // Karplus-Strong Simulation (Approximation for Scope)
                            // Decay envelope simulates the feedback loop loss
                            double freq = 110.0 + (bend * std::exp(-t*10)); // Pitch bend on attack
                            double decayRate = (1.0 - fbk) * 200.0 + (damp * 50.0);
                            double env = std::exp(-t * decayRate);

                            // Sine summation to look like a string on the scope
                            double stringBody = std::sin(t * freq * 6.28)
                                              + 0.5 * std::sin(t * freq * 2 * 6.28) * (1.0-damp)
                                              + 0.2 * std::sin(t * freq * 3 * 6.28) * (1.0-damp);

                            double out = (burst * 0.5) + (stringBody * env * drive);
                            return std::clamp(out, -1.0, 1.0);
                        };

                        // Zoom in to see the attack
                        pluckScope->updateScope(pluckAlgo, 0.2, 1.0);
                        if(btnPlayPluck->isChecked()) m_ghostSynth->setAudioSource(pluckAlgo);
                    };

    // GENERATOR
    connect(btnGenPluck, &QPushButton::clicked, [=]() {
                        // Actual Karplus-Strong Math for Xpressive
                        // O1 = Burst + Feedback
                        double fb = pluckDecay->value() / 100.0;
                        double lp = pluckDamping->value() / 100.0;
                        QString code = QString(
                            "var burst := (t < 0.005 ? randv(t) : 0);\n"
                            "var delay := last(sr/f);\n" // Delay line of 1 wavelength
                            "var filter := delay * %1 + last(sr/f + 1) * %2;\n" // Simple averaging filter
                            "clamp(-1, burst + (filter * %3), 1)"
                        ).arg(1.0 - lp).arg(lp).arg(fb);

                        statusBox->setText(code);
                        QApplication::clipboard()->setText(code);
    });

    connect(pluckDamping, &QSlider::valueChanged, updatePluck);
    connect(pluckDecay, &QSlider::valueChanged, updatePluck);
    connect(pluckColor, &QSlider::valueChanged, updatePluck);
            connect(pluckTension, &QSlider::valueChanged, updatePluck);
                    connect(pluckDrive, &QSlider::valueChanged, updatePluck);

                    connect(btnPlayPluck, &QPushButton::toggled, [=](bool c){
                        if(c) { m_ghostSynth->start(); updatePluck(); btnPlayPluck->setText("⏹ Stop"); }
                        else { m_ghostSynth->stop(); btnPlayPluck->setText("▶ Pluck String"); }
                    });

    modeTabs->addTab(pluckTab, "Pluck Lab");
    QTimer::singleShot(300, updatePluck);
    // ------------------------------------
    //TAB 34
    // ------------------------------------
    initHouseOrganTab();  // Different way to keep code at bottom now

    // ------------------------------------
    //TAB 35
    // ------------------------------------
    // External now
    PCMEditorTab* PCMTab = new PCMEditorTab(m_ghostSynth, this);
    modeTabs->addTab(PCMTab, "PCM Editor");

    // ------------------------------------
    //TAB 36
    // ------------------------------------
    // External now
    oscTab = new OscilloscopeTab(this);
    modeTabs->addTab(oscTab, "Oscilloscope Gen");

    // ------------------------------------
    //TAB 37
    // ------------------------------------
    // External now
    MelodyRendererTab* melodyTab = new MelodyRendererTab(this);
    modeTabs->addTab(melodyTab, "Melody Builder");

    // ------------------------------------
    //TAB 38
    // ------------------------------------
    // External now
    EffectsTab* fxTab = new EffectsTab(this);
    modeTabs->addTab(fxTab, "Effects Builder");

    // ------------------------------------
    //TAB 39
    // ------------------------------------
    // External now
    VocalGeneratorTab *vocalTab = new VocalGeneratorTab(this);
    modeTabs->addTab(vocalTab, "Vocal Generator");


    // ------------------------------------
    // TAB xx: (UNTIL FINISHED INCASE MORE ADDED ). NEED TO KNOW / NOTES TAB
    // ------------------------------------
    QWidget *notesTab = new QWidget();
    auto *notesLayout = new QVBoxLayout(notesTab);

    QTextEdit *notesText = new QTextEdit();
    notesText->setReadOnly(true);

    // HTML Formatting for clarity
    notesText->setHtml(
        "<h2 style='color:#333;'>Project Status & Limitations</h2>"
        "<p><b>Current Version:</b> Experimental Build</p>"
        "<hr>"

        "<h3 style='color:red;'></h3>"
        "<ul>"
        "<li><b></b> </li>"
        "<li><b></b></li>"
        "<li><b>:</b></li>"
        "</ul>"
        );

    notesLayout->addWidget(notesText);
    modeTabs->addTab(notesTab, "Need to Know");

    statusBox = new QTextEdit(); statusBox->setMaximumHeight(100);
    rightLayout->addWidget(statusBox);
    setCentralWidget(centralWidget);
    resize(1200, 850);

    // Connections
    connect(btnLoad, &QPushButton::clicked, this, &MainWindow::loadWav);
    connect(btnSave, &QPushButton::clicked, this, &MainWindow::saveExpr);
    connect(btnCopy, &QPushButton::clicked, this, &MainWindow::copyToClipboard);
    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::addSidSegment);
    connect(btnClear, &QPushButton::clicked, this, &MainWindow::clearAllSid);
    connect(btnSaveSid, &QPushButton::clicked, this, &MainWindow::saveSidExpr);
    connect(btnGenConsole, &QPushButton::clicked, this, &MainWindow::generateConsoleWave);
    connect(btnGenSFX, &QPushButton::clicked, this, &MainWindow::generateSFXMacro);
    connect(btnGenArp, &QPushButton::clicked, this, &MainWindow::generateArpAnimator);
    connect(btnGenWT, &QPushButton::clicked, this, &MainWindow::generateWavetableForge);
    connect(btnBes, &QPushButton::clicked, this, &MainWindow::generateBesselFM);
    connect(besselPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::loadBesselPreset);
    connect(btnHar, &QPushButton::clicked, this, &MainWindow::generateHarmonicLab);
    connect(btnGenVel, &QPushButton::clicked, this, &MainWindow::generateVelocilogic);
    connect(btnNoise, &QPushButton::clicked, this, &MainWindow::generateNoiseForge);
    connect(btnSaveXpf, &QPushButton::clicked, this, &MainWindow::saveXpfInstrument);
    connect(btnRand, &QPushButton::clicked, this, &MainWindow::generateRandomPatch);
}

// =========================================================
// =========================================================
// SYNTHESIS ALGORITHMS & DSP ROUTINES
// =========================================================
// =========================================================


// SHARED


QString MainWindow::getModulatorFormula(int index) { return QString("(0.5 + %1(t * %2) * %3)").arg(mods[index].shape->currentText()).arg(mods[index].rate->value()).arg(mods[index].depth->value()); }




void MainWindow::copyToClipboard() { QApplication::clipboard()->setText(statusBox->toPlainText()); }



// TAB 1: SID ARCHITECT
void MainWindow::saveSidExpr() {
    if (sidSegments.empty()) return;

    QString finalExpr;
    bool isModern = (buildModeSid->currentIndex() == 0);

    if (isModern) {
        // We use a simple loop to build the nested ternary string from the BACK to the FRONT.
        // This ensures the nesting is mathematically sound for the Xpressive engine.
        QString nestedBody = "0"; // The final 'else' if time runs out
        double totalTime = 0;
        for (const auto& s : sidSegments) totalTime += s.duration->value();

        double currentTime = totalTime;
        // Iterate backwards to nest properly: (t < t1 ? seg1 : (t < t2 ? seg2 : 0))
        for (int i = sidSegments.size() - 1; i >= 0; --i) {
            const auto& s = sidSegments[i];
            double segmentDur = s.duration->value();
            currentTime -= segmentDur;

            QString fBase = (s.freqOffset->value() == 0) ? "f" : QString("(f + %1)").arg(s.freqOffset->value());
            QString waveExpr = getSegmentWaveform(s, fBase);
            QString envExpr = QString("exp(-(t - %1) * %2)").arg(currentTime, 0, 'f', 4).arg(s.decay->value());

            double tEnd = currentTime + segmentDur;
            nestedBody = QString("(t < %1 ? (%2 * %3) : %4)")
                             .arg(tEnd, 0, 'f', 4)
                             .arg(waveExpr)
                             .arg(envExpr)
                             .arg(nestedBody);
        }
        finalExpr = nestedBody;
    } else {
        // Legacy Mode: Uses additive logic (Segment1 + Segment2)
        QStringList bodies;
        double tPos = 0.0;
        for (const auto& s : sidSegments) {
            QString fBase = (s.freqOffset->value() == 0) ? "f" : QString("(f + %1)").arg(s.freqOffset->value());
            QString waveExpr = getSegmentWaveform(s, fBase);
            bodies << QString("(t >= %1 & t < %2) * %3 * exp(-(t-%1)*%4)")
                          .arg(tPos, 0, 'f', 4)
                          .arg(tPos + s.duration->value(), 0, 'f', 4)
                          .arg(waveExpr)
                          .arg(s.decay->value());
            tPos += s.duration->value();
        }
        finalExpr = bodies.join(" + ");
    }

    statusBox->setText(QString("clamp(-1, %1, 1)").arg(finalExpr));
    waveVisualizer->updateData(sidSegments);
    btnCopy->setEnabled(true);
}

void MainWindow::clearAllSid() {
    for(auto& s : sidSegments) s.container->deleteLater();
    sidSegments.clear();
    waveVisualizer->updateData(sidSegments);
}

QString MainWindow::getSegmentWaveform(const SidSegment& s, const QString& fBase) {
    QString wave = s.waveType->currentText();
    if (wave.contains("PWM")) return QString("sgn(mod(t, 1/%1) < (%2 / %1)) * 2 - 1").arg(fBase, getModulatorFormula(wave.contains("4") ? 3 : 4));
    if (wave.contains("FM:")) return QString("trianglew(integrate(%1 + (%2 * 500)))").arg(fBase, getModulatorFormula(0));
    if (wave.contains("Arp")) return getArpFormula(wave.contains("1") ? 0 : 1);
    if (wave == "randv") return "randv(t * srate)";
    return QString("%1(integrate(%2))").arg(wave, fBase);
}

void MainWindow::addSidSegment() {
    auto *frame = new QFrame(); auto *h = new QHBoxLayout(frame);
    auto *type = new QComboBox(); type->addItems({"trianglew", "squarew", "saww", "randv", "PWM (Mod 4)", "PWM (Mod 5)", "Arp 1", "Arp 2", "FM: Mod 1", "sinew"});
    auto *dur = new QDoubleSpinBox(); dur->setValue(0.1); auto *off = new QDoubleSpinBox(); off->setRange(-10000, 10000);
    auto *dec = new QDoubleSpinBox(); dec->setValue(0); auto *rem = new QPushButton("X");
    h->addWidget(type); h->addWidget(dur); h->addWidget(off); h->addWidget(dec); h->addWidget(rem);
    sidSegmentsLayout->addWidget(frame); sidSegments.push_back({type, dur, dec, off, rem, frame});
    connect(rem, &QPushButton::clicked, this, &MainWindow::removeSidSegment);
}

void MainWindow::removeSidSegment() {
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    for (auto it = sidSegments.begin(); it != sidSegments.end(); ++it) if (it->deleteBtn == btn) { it->container->deleteLater(); sidSegments.erase(it); break; }
}

QString MainWindow::getArpFormula(int index) {
    QString wave = arps[index].wave->currentText(), speed = arps[index].sync->isChecked() ? QString("(tempo/60) * %1").arg(arps[index].multiplier->currentText().remove('x')) : QString::number(arps[index].speed->value());
    QString r1 = (arps[index].chord->currentText() == "Minor") ? "1.1892" : "1.2599", r2 = (arps[index].chord->currentText() == "Dim") ? "1.4142" : "1.4983";
    return QString("%1(integrate(f * (mod(t * %2, 3) < 1 ? 1 : (mod(t * %2, 3) < 2 ? %3 : %4))))").arg(wave, speed, r1, r2);
}


// TAB 2: PCM SAMPLER
void MainWindow::loadWav() {
    QString path = QFileDialog::getOpenFileName(this, "Select WAV", "", "WAV (*.wav)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Basic Header Checks
    char riff[4], wave[4];
    file.read(riff, 4);
    file.seek(8);
    file.read(wave, 4);

    if (strncmp(riff, "RIFF", 4) != 0 || strncmp(wave, "WAVE", 4) != 0) {
        statusBox->setText("Error: Not a valid WAV file.");
        return;
    }

    // Scan Chunks (robustly)
    uint16_t audioFormat = 0; // 1=PCM, 3=Float
    uint16_t numChannels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataSize = 0;
    qint64 dataOffset = 0;

    while (!file.atEnd()) {
        char chunkID[4];
        if (file.read(chunkID, 4) < 4) break;

        uint32_t chunkSize;
        stream >> chunkSize;
        qint64 nextChunk = file.pos() + chunkSize;

        // Correct for odd-sized chunks (WAV standard requires padding byte)
        if (chunkSize % 2 != 0) nextChunk++;

        if (strncmp(chunkID, "fmt ", 4) == 0) {
            stream >> audioFormat;
            stream >> numChannels;
            stream >> sampleRate;
            file.seek(file.pos() + 6); // Skip ByteRate & BlockAlign
            stream >> bitsPerSample;
        }
        else if (strncmp(chunkID, "data", 4) == 0) {
            dataSize = chunkSize;
            dataOffset = file.pos();
            break; // Stop scanning once data is found
        }

        file.seek(nextChunk);
    }

    if (dataSize == 0) {
        statusBox->setText("Error: No audio data found.");
        return;
    }

    // Prepare Data
    fileFs = sampleRate;
    originalData.clear();
    file.seek(dataOffset);
    QByteArray raw = file.read(dataSize);

    if (audioFormat == 3 && bitsPerSample == 32) {
        // --- 32-BIT FLOAT ---
        const float* samples = reinterpret_cast<const float*>(raw.data());
        int count = raw.size() / 4;
        for(int i = 0; i < count; i += numChannels) {
            double val = samples[i]; // Float is already -1.0 to 1.0
            // Mix Stereo
            if (numChannels > 1 && (i+1 < count)) val = (val + samples[i+1]) * 0.5;
            originalData.push_back(val);
        }
    }
    else if (audioFormat == 1) {
        // --- PCM INTEGERS ---

        if (bitsPerSample == 16) {
            const int16_t* samples = reinterpret_cast<const int16_t*>(raw.data());
            int count = raw.size() / 2;
            for(int i = 0; i < count; i += numChannels) {
                double val = samples[i] / 32768.0;
                if (numChannels > 1 && (i+1 < count))
                    val = (val + (samples[i+1] / 32768.0)) * 0.5;
                originalData.push_back(val);
            }
        }
        else if (bitsPerSample == 8) {
            const uint8_t* samples = reinterpret_cast<const uint8_t*>(raw.data());
            int count = raw.size();
            for(int i = 0; i < count; i += numChannels) {
                // 8-bit is unsigned 0..255 (center 128)
                double val = (samples[i] - 128) / 128.0;
                if (numChannels > 1 && (i+1 < count))
                    val = (val + ((samples[i+1] - 128) / 128.0)) * 0.5;
                originalData.push_back(val);
            }
        }
        else if (bitsPerSample == 24) {
            // 24-bit is tricky: 3 bytes packed together
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(raw.data());
            int count = raw.size() / 3;
            // We read blocks of (3 * numChannels) bytes
            for(int i = 0; i < count; i += numChannels) {
                int byteIdx = i * 3;

                // Construct 32-bit int from 3 bytes (Little Endian)
                int32_t val32 = (bytes[byteIdx] | (bytes[byteIdx+1] << 8) | (bytes[byteIdx+2] << 16));

                // Sign Extension: If the 24th bit is 1, we make the upper byte 0xFF
                if (val32 & 0x800000) val32 |= 0xFF000000;

                double val = val32 / 8388608.0; // 2^23

                if (numChannels > 1 && (i+1 < count)) {
                    int b2 = (i+1) * 3;
                    int32_t val32R = (bytes[b2] | (bytes[b2+1] << 8) | (bytes[b2+2] << 16));
                    if (val32R & 0x800000) val32R |= 0xFF000000;
                    val = (val + (val32R / 8388608.0)) * 0.5;
                }
                originalData.push_back(val);
            }
        }
    }
    else {
        statusBox->setText("Error: Unsupported WAV format (ADPCM or Compressed).");
        return;
    }

    // Update UI Limits and Scope (Same as before)
    maxDurSpin->setValue((double)originalData.size() / fileFs);
    btnSave->setEnabled(true);

    double dur = (double)originalData.size() / fileFs;
    double zoom = pcmZoomSlider->value() / 100.0;

    pcmScope->updateScope([this](double t){
        int idx = (int)(t * fileFs);
        if(idx >= 0 && idx < (int)originalData.size()) return originalData[idx];
        return 0.0;
    }, dur, zoom);

    statusBox->setText(QString("Loaded: %1Hz, %2-bit, %3 Ch").arg(sampleRate).arg(bitsPerSample).arg(numChannels));
}

void MainWindow::saveExpr() {
    if (originalData.empty()) return;
    double targetFs = sampleRateCombo->currentText().toDouble();
    std::vector<double> proc; double step = (double)fileFs / targetFs;
    int maxS = std::min((int)originalData.size(), (int)(maxDurSpin->value() * targetFs));

    double maxVal = 0.0;
    // Scan for Normalisation
    if (normalizeCheck->isChecked()) {
        for(int i=0; i < maxS; ++i) {
            double d = std::abs(originalData[int(i*step)]);
            if (d > maxVal) maxVal = d;
        }
    }
    if (maxVal < 0.0001) maxVal = 1.0; // Prevent div by zero

    // Figure out how many levels to quantize to based on the UI
    int bitMode = bitDepthCombo->currentIndex();
    double levels = 15.0; // Default to 4-bit
    if (bitMode == 1) levels = 255.0; // 8-bit

    for(int i=0; i < maxS; ++i) {
        double d = originalData[int(i*step)];

        //  Normalise (-1.0 to 1.0)
        if (normalizeCheck->isChecked()) d /= maxVal;

        // Apply Bitcrushing if they didn't select "Uncompressed"
        if (bitMode != 2) {
            double temp = (d + 1.0) * 0.5 * levels; // Scale to 0..levels
            int stepVal = std::round(temp);       // Round to integer step
            if(stepVal < 0) stepVal = 0;
            if(stepVal > levels) stepVal = levels;

            // Convert back to -1.0 .. 1.0 range
            d = (stepVal / levels) * 2.0 - 1.0;
        }

        proc.push_back(d);
    }

    statusBox->setText((buildModeCombo->currentIndex() == 0) ? generateModernPCM(proc, targetFs) : generateLegacyPCM(proc, targetFs));
    btnCopy->setEnabled(true);
}

QString MainWindow::generateModernPCM(const std::vector<double>& q, double sr) {
    int N = q.size(); if (N == 0) return "0";
    QString header = QString("var s := floor(t * %1);\n").arg(sr);
    std::function<QString(int, int)> buildTree = [&](int start, int end) -> QString {
        if (start == end) return QString::number(q[start], 'f', 3);
        int mid = start + (end - start) / 2;
        return QString("((s <= %1) ? (%2) : (%3))").arg(mid).arg(buildTree(start, mid)).arg(buildTree(mid + 1, end));
    };
    return header + buildTree(0, N - 1);
}

QString MainWindow::generateLegacyPCM(const std::vector<double>& q, double sr) {
    int N = q.size(); if (N == 0) return "0";
    // Recursive binary tree with Float formatting
    std::function<QString(int, int)> buildLegacyTree = [&](int start, int end) -> QString {
        if (start == end) return QString::number(q[start], 'f', 3);
        int mid = start + (end - start) / 2;
        double midTime = (double)(mid + 1) / sr;
        return QString("(t < %1 ? %2 : %3)")
            .arg(QString::number(midTime, 'f', 6))
            .arg(buildLegacyTree(start, mid))
            .arg(buildLegacyTree(mid + 1, end));
    };
    return buildLegacyTree(0, N - 1);
}

// TAB 3: CONSOLE LAB
void MainWindow::generateConsoleWave() {
    // Get the build mode (Modern vs Legacy)
    int mode = buildModeConsole ? buildModeConsole->currentIndex() : 0;

    QString result;
    auto *rng = QRandomGenerator::global();

    if (mode == 0) {
        // --- MODERN (Float Math) ---
        QStringList waves = {"sin", "cos", "tri", "saw", "sqr"};
        QStringList mods = {
            "",
            "* (1.0 - t*0.5)",        // Decay
            "* (0.5 + 0.5*sin(t*4))", // Tremolo
            "+ 0.1*sin(t*50)",        // FM Grit
            "* ((t*10 % 2) - 1)"      // Sample & Hold
        };
        QStringList quantizers = {
            "",
            "floor(x * 4)/4",    // 2-bit
            "floor(x * 16)/16",  // 4-bit
            "x > 0 ? 1 : -1"     // 1-bit Comparator
        };

        QString w = waves[rng->bounded(waves.size())];
        QString m = mods[rng->bounded(mods.size())];
        QString q = quantizers[rng->bounded(quantizers.size())];

        // Randomize Frequency Multiplier
        double freqMult = (rng->bounded(4) + 1) * 0.5; // 0.5, 1.0, 1.5, 2.0

        // Build the Core: "sin(t * f * 1.5)"
        QString core = QString("%1(t * f * %2)").arg(w).arg(freqMult);

        // Add Modifiers
        QString full = core + m;

        // Add Quantizer
        if (q.isEmpty()) {
            result = full;
        } else {
            result = q.replace("x", "(" + full + ")");
        }

    } else {
        // --- LEGACY (Bytebeat / Bitwise) ---
        QStringList ops = {"&", "|", "^", ">>", "<<"};
        QStringList vals = {"t", "t*2", "t/2", "t>>4", "t&128"};

        QString a = vals[rng->bounded(vals.size())];
        QString b = vals[rng->bounded(vals.size())];
        QString op = ops[rng->bounded(ops.size())];

        result = QString("(%1 %2 %3) & 255").arg(a, op, b);
    }

    // --- OUTPUT ---

    // 1. Debug Print
    qDebug() << "GENERATED:" << result;

    // 2. Copy to Clipboard
    QGuiApplication::clipboard()->setText(result);

    // 3. VISUAL CONFIRMATION (This was missing!)
    statusBox->setText(result);
}

// TAB 4: SFX MACRO
void MainWindow::generateSFXMacro() {
    double f1 = sfxStartFreq->value(), f2 = sfxEndFreq->value(), d = sfxDur->value();
    QString audio = QString("sinew(integrate(%1 * exp(-t * %2)))").arg(f1).arg(std::log(f1/f2)/d);
    statusBox->setText((buildModeSFX->currentIndex() == 0) ? QString("(t < %1 ? %2 : 0)").arg(d).arg(audio) : QString("(t < %1) * %2").arg(d).arg(audio));
}

// TAB 5: ARP ANIMATOR
void MainWindow::generateArpAnimator() {
    // 1. CALCULATE SPEED
    double hz = 0;
    if (arpBpmSync->isChecked()) {
        double bpm = arpBpmVal->value();
        // 1/16th note = BPM / 60 * 4
        // Hubbard Speed (1/64) is 4x faster than that
        int divIdx = arpSpeedDiv->currentIndex();
        if (divIdx == 4) hz = 50.0; // PAL Frame (Classic C64 standard)
        else {
            double multiplier = (divIdx == 0) ? 4.0 : (divIdx == 1) ? 8.0 : (divIdx == 2) ? 12.0 : 16.0;
            hz = (bpm / 60.0) * multiplier;
        }
    } else {
        hz = arpSpeed->value();
    }

    // GET INTERVALS (Parse the combobox text)
    // Helper to extract number from string like "+7 (Perfect 5th)"
    auto getSemi = [](QString s) {
        return s.split(" ")[0].toInt();
    };
    int note1 = 0; // Root is always 0
    int note2 = getSemi(arpInterval1->currentText());
    int note3 = getSemi(arpInterval2->currentText());

    // GENERATE WAVEFORM LOGIC
    QString waveName = arpWave->currentText();
    double pwm = arpPwmSlider->value() / 100.0;

    // We define a lambda to generate the "Audio" part for a specific pitch
    auto genAudio = [&](QString pitchMult) {
        if (waveName.contains("Pulse")) {
            // Pulse Width formula: (sin(f) > width ? 1 : -1)
            // We scale PWM to -1..1 range for the engine
            return QString("(sinew(integrate(f*%1)) > %2 ? 1 : -1)")
                .arg(pitchMult).arg((pwm * 2.0) - 1.0);
        }
        else if (waveName.contains("Metal")) {
            // Ring Mod: Square * Detuned Square
            return QString("(squarew(integrate(f*%1)) * squarew(integrate(f*%1*2.41)))")
                .arg(pitchMult);
        }
        else if (waveName.contains("Noise")) {
            return QString("randv(t*10000)"); // Noise ignores pitch usually
        }
        else {
            QString osc = waveName.contains("Saw") ? "saww" : "trianglew";
            return QString("%1(integrate(f*%2))").arg(osc).arg(pitchMult);
        }
    };

    QString finalExpr;

    // BUILD THE ARP (Nested vs Legacy)

    // Calculate Pitch Multipliers: 2^(semitones/12)
    QString p1 = "1.0";
    QString p2 = QString::number(std::pow(2.0, note2 / 12.0), 'f', 4);
    QString p3 = QString::number(std::pow(2.0, note3 / 12.0), 'f', 4);

    if (buildModeArp->currentIndex() == 0) { // NIGHTLY (Clean Switching)
        // Logic: var step := floor(t * speed) % 3;
        // (step == 0 ? Note1 : (step == 1 ? Note2 : Note3))

        QString selector = QString("mod(floor(t*%1), 3)").arg(hz);

        finalExpr = QString("(%1 < 1 ? %2 : (%1 < 2 ? %3 : %4))")
                        .arg(selector)
                        .arg(genAudio(p1))
                        .arg(genAudio(p2))
                        .arg(genAudio(p3));

    } else { // LEGACY (Additive)
        // Logic: ((Phase 0) * Note1) + ((Phase 1) * Note2) ...
        // We use mod(floor(t*spd), 3) to find the current step index

        QString stepCheck = QString("mod(floor(t*%1), 3)").arg(hz);

        finalExpr = QString("((%1 < 1) * %2) + ((%1 >= 1 & %1 < 2) * %3) + ((%1 >= 2) * %4)")
                        .arg(stepCheck)
                        .arg(genAudio(p1))
                        .arg(genAudio(p2))
                        .arg(genAudio(p3));
    }

    statusBox->setText(QString("clamp(-1, %1, 1)").arg(finalExpr));
}

// TAB 6: WAVETABLE FORGE
void MainWindow::loadWavetablePreset(int index) {

    wtTrackerTable->blockSignals(true);

    wtTrackerTable->setRowCount(0);

    auto add = [&](QString w, int p, int pwm, double d) {
        int r = wtTrackerTable->rowCount();
        wtTrackerTable->insertRow(r);
        wtTrackerTable->setItem(r, 0, new QTableWidgetItem(w));
        wtTrackerTable->setItem(r, 1, new QTableWidgetItem(QString::number(p)));
        wtTrackerTable->setItem(r, 2, new QTableWidgetItem(QString::number(pwm)));
        wtTrackerTable->setItem(r, 3, new QTableWidgetItem(QString::number(d)));
    };

    QString txt = wtPresetCombo->currentText();

    // --- ROB HUBBARD ---
    if (txt.contains("Commando")) {
        add("Tri", 0, 0, 0.03); add("Tri", -1, 0, 0.03); add("Tri", -2, 0, 0.03);
        add("Tri", -3, 0, 0.03); add("Tri", -4, 0, 0.03); add("Tri", -5, 0, 0.03);
    }
    else if (txt.contains("Monty")) {
        add("Pulse", 0, 10, 0.04); add("Pulse", 0, 50, 0.04); add("Pulse", 0, 90, 0.04);
        add("Pulse", 0, 50, 0.04);
    }
    else if (txt.contains("Delta")) {
        add("Noise", 12, 0, 0.02); add("TriNoise", 0, 0, 0.04); add("Tri", -5, 0, 0.05);
    }
    else if (txt.contains("Zoids")) {
        add("Metal", 0, 0, 0.03); add("Metal", 1, 0, 0.03);
        add("Metal", -1, 0, 0.03); add("Metal", 0, 0, 0.03);
    }
    else if (txt.contains("Ace 2")) {
        add("Square", 0, 50, 0.01); add("Tri", -12, 0, 0.02); add("Tri", -24, 0, 0.08);
    }
    else if (txt.contains("Comets")) {
        add("Saw", 0, 0, 0.06); add("Saw", 0, 0, 0.06);
        add("Saw", 0, 0, 0.06); add("Saw", 0, 0, 0.04);
    }
    // --- MARTIN GALWAY ---
    else if (txt.contains("Wizball")) { add("Tri", 0, 0, 0.03); add("Tri", 4, 0, 0.03); add("Tri", 7, 0, 0.03); }
    else if (txt.contains("Parallax")) { add("Saw", 12, 0, 0.02); add("Pulse", 0, 20, 0.05); add("Pulse", 0, 40, 0.10); }
    else if (txt.contains("Comic")) { add("Pulse", 0, 50, 0.03); add("Pulse", 0, 50, 0.03); add("Pulse", 1, 50, 0.02); add("Pulse", 0, 50, 0.02); }
    else if (txt.contains("Arkanoid")) { add("Saw", 0, 0, 0.06); add("Saw", 0, 0, 0.06); add("Saw", 0, 0, 0.04); add("Saw", 0, 0, 0.04); add("Saw", 0, 0, 0.02); }
    else if (txt.contains("Green Beret")) { add("Noise", 10, 0, 0.02); add("Noise", 5, 0, 0.03); add("Noise", 0, 0, 0.05); }
    // --- JEROEN TEL ---
    else if (txt.contains("Cybernoid")) { add("Metal", 24, 0, 0.02); add("Metal", 12, 0, 0.02); add("Noise", 0, 0, 0.05); }
    else if (txt.contains("Supremacy")) { add("Saw", 0, 0, 0.05); add("Saw", 0, 0, 0.05); add("Saw", 1, 0, 0.02); add("Saw", -1, 0, 0.02); }
    else if (txt.contains("Turbo Outrun")) { add("Metal", 0, 0, 0.02); add("Pulse", -12, 40, 0.04); add("Pulse", -12, 60, 0.08); }
    else if (txt.contains("RoboCop 3")) { add("Saw", 0, 0, 0.02); add("Saw", 7, 0, 0.02); add("Saw", 12, 0, 0.02); add("Saw", 19, 0, 0.02); }
    // --- CHRIS HUELSBECK ---
    else if (txt.contains("Turrican I")) { add("Pulse", 0, 50, 0.02); add("Pulse", 12, 50, 0.02); add("Pulse", 0, 25, 0.02); add("Pulse", 19, 25, 0.02); }
    else if (txt.contains("Katakis")) { add("SawSync", 0, 0, 0.04); add("SawSync", 0, 0, 0.04); }
    else if (txt.contains("Turrican II")) { add("Pulse", 0, 10, 0.05); add("Pulse", 0, 20, 0.05); add("Pulse", 0, 30, 0.05); add("Pulse", 0, 40, 0.05); add("Pulse", 0, 50, 0.20); }
    else if (txt.contains("Great Giana")) { add("Tri", 0, 0, 0.03); add("Square", 0, 50, 0.10); }
    // --- TIM FOLLIN ---
    else if (txt.contains("Solstice")) { add("Pulse", 0, 15, 0.02); add("Pulse", 0, 20, 0.02); add("Pulse", 0, 25, 0.02); add("Pulse", 0, 30, 0.02); }
    else if (txt.contains("Ghouls")) { add("Noise", 24, 0, 0.01); add("Noise", 12, 0, 0.02); }
    else if (txt.contains("Silver Surfer")) { add("Pulse", 0, 25, 0.01); add("Pulse", 4, 25, 0.01); add("Pulse", 7, 25, 0.01); add("Pulse", 11, 25, 0.01); add("Pulse", 14, 25, 0.01); add("Pulse", 12, 50, 0.01); }
    else if (txt.contains("LED Storm")) { add("Saw", 12, 0, 0.02); add("Saw", 0, 0, 0.03); add("Saw", 0, 0, 0.03); add("Saw", 12, 0, 0.02); }
    // --- BEN DAGLISH ---
    else if (txt.contains("Last Ninja")) { add("Saw", 0, 0, 0.04); add("Tri", 0, 0, 0.04); add("Tri", -12, 0, 0.10); }
    else if (txt.contains("Deflektor")) { add("Pulse", 0, 50, 0.02); add("Pulse", 1, 50, 0.02); add("Pulse", 2, 50, 0.02); add("Pulse", 3, 50, 0.02); add("Pulse", 4, 10, 0.10); }
    else if (txt.contains("Trap")) { add("Square", 0, 50, 0.02); add("Square", 4, 50, 0.02); add("Square", 7, 50, 0.02); add("Square", 0, 50, 0.02); }
    // --- DAVID WHITTAKER ---
    else if (txt.contains("Glider Rider")) { add("Square", 0, 50, 0.05); add("Square", 0, 50, 0.05); }
    else if (txt.contains("Lazy Jones")) { add("Saw", 24, 0, 0.01); add("Saw", 20, 0, 0.01); add("Saw", 16, 0, 0.01); add("Saw", 12, 0, 0.01); add("Saw", 8, 0, 0.01); add("Saw", 4, 0, 0.01); }
    // --- YM / ATARI ---
    else if (txt.contains("YM Buzzer")) { add("Saw", 0, 0, 0.02); add("Saw", 0, 0, 0.02); add("Pulse", 0, 50, 0.01); }
    else if (txt.contains("YM Metal")) { add("Metal", 0, 0, 0.05); add("Metal", -12, 0, 0.05); }
    else if (txt.contains("YM 3-Voice")) { add("Saw", 0, 0, 0.01); add("Saw", 4, 0, 0.01); add("Saw", 7, 0, 0.01); }
    else if (txt.contains("Digi-Drum")) { add("Pulse", -24, 50, 0.01); add("Pulse", -24, 50, 0.01); add("Pulse", -24, 90, 0.01); add("Pulse", -24, 10, 0.01); }
    // --- FX ---
    else if (txt.contains("Coin")) { add("Pulse", 0, 50, 0.03); add("Pulse", 5, 50, 0.03); add("Pulse", 10, 50, 0.03); add("Pulse", 15, 50, 0.10); }
    else if (txt.contains("Explosion")) { add("Noise", 0, 0, 0.10); add("Noise", -5, 0, 0.10); add("Noise", -10, 0, 0.20); }
    else if (txt.contains("Fake Chord (Major)")) { add("Saw", 0, 0, 0.01); add("Saw", 4, 0, 0.01); add("Saw", 7, 0, 0.01); }
    else if (txt.contains("Power Up")) { add("Tri", 0, 0, 0.02); add("Tri", 2, 0, 0.02); add("Tri", 4, 0, 0.02); add("Tri", 5, 0, 0.02); add("Tri", 7, 0, 0.02); add("Tri", 12, 0, 0.10); }
    else if (txt.contains("Laser")) { add("Saw", 30, 0, 0.01); add("Saw", 20, 0, 0.01); add("Saw", 10, 0, 0.01); add("Saw", 0, 0, 0.01); add("Saw", -10, 0, 0.01); }
    else if (txt.contains("Hi-Hat (Closed)")) { add("Metal", 40, 0, 0.01); add("Noise", 40, 0, 0.01); }
    else if (txt.contains("Hi-Hat (Open)")) { add("Metal", 40, 0, 0.02); add("Noise", 40, 0, 0.04); }
    else if (txt.contains("Fake Chord (Minor)")) { add("Saw", 0, 0, 0.01); add("Saw", 3, 0, 0.01); add("Saw", 7, 0, 0.01); }
    // --- EXPANSION ---
    else if (txt.contains("Heavy SID Kick")) { add("Square", 36, 50, 0.01); add("Tri", 12, 0, 0.01); add("Tri", 0, 0, 0.02); add("Tri", -12, 0, 0.04); add("Tri", -24, 0, 0.08); }
    else if (txt.contains("Snappy Snare")) { add("Noise", 24, 0, 0.01); add("TriNoise", 12, 0, 0.02); add("TriNoise", 0, 0, 0.03); add("Noise", -12, 0, 0.05); }
    else if (txt.contains("Tech Kick")) { add("Metal", 12, 0, 0.01); add("Pulse", 0, 50, 0.02); add("Pulse", -12, 50, 0.05); add("Pulse", -24, 50, 0.10); }
    else if (txt.contains("Glitch Snare")) { add("Metal", 24, 0, 0.02); add("Metal", 12, 0, 0.03); add("Noise", 0, 0, 0.06); }

    // 2. UNBLOCK AND UPDATE
    wtTrackerTable->blockSignals(false);
    if(wtTrackerTable->rowCount() > 0) {
        // This forces the 'cellChanged' signal to fire safely now
        wtTrackerTable->item(0,0)->setText(wtTrackerTable->item(0,0)->text());
    }
}

void MainWindow::generateWavetableForge() {
    int rows = wtTrackerTable->rowCount();
    if (rows == 0) return;

    double totalDuration = 0;
    for(int i=0; i<rows; ++i)
        if(wtTrackerTable->item(i,3)) totalDuration += wtTrackerTable->item(i, 3)->text().toDouble();

    QString timeVar = "t";
    if (wtLoopCheck->isChecked()) timeVar = QString("mod(t, %1)").arg(totalDuration, 0, 'f', 4);

    // MODE SELECTION
    if (buildModeWavetable->currentIndex() == 0) { // NIGHTLY (Nested)
        QString nestedBody = "0";
        double currentTime = totalDuration;

        for (int i = rows - 1; i >= 0; --i) {
            QString type = wtTrackerTable->item(i, 0)->text().toLower();
            int pitch = wtTrackerTable->item(i, 1)->text().toInt();
            double pwmVal = wtTrackerTable->item(i, 2)->text().toDouble() / 100.0;
            double dur = wtTrackerTable->item(i, 3)->text().toDouble();
            currentTime -= dur;

            double pitchMult = std::pow(2.0, pitch / 12.0);

            // --- NEW WAVEFORM LOGIC ---
            QString audio;

            if (type.contains("trinoise")) {
                // Classic C64 Snare Drum trick (Tri + Noise)
                audio = QString("(trianglew(integrate(f*%1)) + 0.5*randv(t*10000))").arg(pitchMult);
            }
            else if (type.contains("metal")) {
                // Ring Mod Simulation (Square * Detuned Square)
                audio = QString("(squarew(integrate(f*%1)) * squarew(integrate(f*%2)))")
                            .arg(pitchMult).arg(pitchMult * 2.41);
            }
            else if (type.contains("sawsync")) {
                // Hard Sync Simulation (Saw * Saw window)
                audio = QString("(saww(integrate(f*%1)) * saww(integrate(f*%2)))")
                            .arg(pitchMult).arg(pitchMult * 0.5);
            }
            else if (type.contains("pulse")) {
                audio = QString("(sinew(integrate(f*%1)) > %2 ? 1 : -1)")
                .arg(pitchMult, 0, 'f', 4).arg((pwmVal * 2.0) - 1.0, 0, 'f', 4);
            }
            else if (type.contains("noise")) {
                audio = "randv(t * 10000)";
            }
            else {
                QString osc = type.contains("tri") ? "trianglew" : "saww";
                audio = QString("%1(integrate(f*%2))").arg(osc).arg(pitchMult, 0, 'f', 4);
            }

            nestedBody = QString("(%1 < %2 ? %3 : %4)")
                             .arg(timeVar).arg(currentTime + dur, 0, 'f', 4)
                             .arg(audio).arg(nestedBody);
        }
        statusBox->setText(QString("clamp(-1, %1, 1)").arg(nestedBody));
    }
    else { // LEGACY (Additive)
        QStringList additiveParts;
        double currentTime = 0.0;
        for (int i = 0; i < rows; ++i) {
            QString type = wtTrackerTable->item(i, 0)->text().toLower();
            int pitch = wtTrackerTable->item(i, 1)->text().toInt();
            double pwmVal = wtTrackerTable->item(i, 2)->text().toDouble() / 100.0;
            double dur = wtTrackerTable->item(i, 3)->text().toDouble();

            double pitchMult = std::pow(2.0, pitch / 12.0);
            QString audio;

            // Same special waveforms for Legacy
            if (type.contains("trinoise")) audio = QString("(trianglew(integrate(f*%1)) + 0.5*randv(t*10000))").arg(pitchMult);
            else if (type.contains("metal")) audio = QString("(squarew(integrate(f*%1)) * squarew(integrate(f*%2)))").arg(pitchMult).arg(pitchMult * 2.41);
            else if (type.contains("sawsync")) audio = QString("(saww(integrate(f*%1)) * saww(integrate(f*%2)))").arg(pitchMult).arg(pitchMult * 0.5);
            else if (type.contains("pulse")) audio = QString("(sinew(integrate(f*%1)) > %2 ? 1 : -1)").arg(pitchMult).arg((pwmVal * 2.0) - 1.0);
            else if (type.contains("noise")) audio = "randv(t * 10000)";
            else {
                QString osc = type.contains("tri") ? "trianglew" : "saww";
                audio = QString("%1(integrate(f*%2))").arg(osc).arg(pitchMult);
            }

            QString block = QString("((%1 >= %2 & %1 < %3) * %4)")
                                .arg(timeVar).arg(currentTime, 0, 'f', 4)
                                .arg(currentTime + dur, 0, 'f', 4).arg(audio);
            additiveParts << block;
            currentTime += dur;
        }
        statusBox->setText(QString("clamp(-1, %1, 1)").arg(additiveParts.join(" + ")));
    }
}

// TAB 7: BESSEL FM
void MainWindow::loadBesselPreset(int idx) {
    if (idx == 0 || idx == 9 || idx == 18 || idx == 28) return;
    auto setFM = [&](QString cw, QString mw, double cm, double mm, double i) {
        besselCarrierWave->setCurrentText(cw); besselModWave->setCurrentText(mw);
        besselCarrierMult->setValue(cm); besselModMult->setValue(mm); besselModIndex->setValue(i);
    };
    switch(idx) {
    case 1:  setFM("sinew", "sinew", 1.0, 14.0, 1.2); break;
    case 2:  setFM("sinew", "sinew", 1.0, 3.5, 2.5); break;
    case 3:  setFM("trianglew", "sinew", 1.0, 8.0, 0.8); break;
    case 4:  setFM("sinew", "sinew", 2.0, 1.0, 0.5); break;
    case 5:  setFM("sinew", "sinew", 1.0, 19.0, 3.0); break;
    case 10: setFM("sinew", "saww", 1.0, 1.0, 3.5); break;
    case 11: setFM("sinew", "sinew", 1.0, 1.0, 1.8); break;
    case 19: setFM("sinew", "sinew", 1.0, 3.5, 2.0); break;
    case 20: setFM("sinew", "sinew", 1.0, 7.11, 4.0); break;
    case 37: setFM("sinew", "saww", 1.0, 8.0, 6.0); break;
    case 40: setFM("sinew", "saww", 8.0, 0.1, 50.0); break;
    default: setFM("sinew", "sinew", 1.0, 2.0, 2.0); break;
    }
}

void MainWindow::generateBesselFM() {
    QString fExpr = QString("f*%1 + (%2(integrate(f*%3))*%4*f*%3)").arg(besselCarrierMult->value()).arg(besselModWave->currentText()).arg(besselModMult->value()).arg(besselModIndex->value());
    statusBox->setText(QString("clamp(-1, %1(integrate(%2)), 1)").arg(besselCarrierWave->currentText(), fExpr));
}

// TAB 8: HARMONIC LAB
void MainWindow::generateHarmonicLab() {
    QStringList t;
    for(int i=0; i<16; ++i) {
        double v = harmonicSliders[i]->value() / 100.0;
        if(v > 0) t << QString("%1 * sinew(integrate(f * %2))").arg(v).arg(i+1);
    }
    statusBox->setText(t.isEmpty() ? "0" : QString("clamp(-1, %1, 1)").arg(t.join(" + ")));
}

// TAB 9: DRUM DESIGNER
void MainWindow::generateDrumXpf() {
    QString waveFunc = drumWaveCombo->currentText().toLower() + "w";
    if(waveFunc == "sinew") waveFunc = "sinew";

    double decayBase = drumDecaySlider->value();
    double expFactor = drumExpSlider->value(); //

    // Pitch Drop Formula
    QString pitch = QString("(f + (%1 * exp(-t * %2)))").arg(drumPitchDropSlider->value()).arg(decayBase / 2.0);

    // Osc + Noise Mix
    double nMix = drumNoiseSlider->value() / 100.0;
    QString source = QString("((%1(integrate(%2)) * %3) + (randv(t*10000) * %4))")
                         .arg(waveFunc).arg(pitch).arg(1.0 - nMix).arg(nMix);

    // Final Exponential Volume Shape
    QString formula = QString("(%1 * exp(-t * %2))").arg(source).arg(decayBase * expFactor);
    formula = formula.replace("\"", "&quot;");

    int typeIndex = drumTypeCombo->currentIndex();
    int filterType = 0; // Default LPF (0)
    if (typeIndex == 1 || typeIndex == 4 || typeIndex == 6) filterType = 2; // BPF
    if (typeIndex == 2 || typeIndex == 5) filterType = 1; // HPF





    QString xpf = getXpfTemplate()
                      .arg(drumTypeCombo->currentText()).arg(drumPitchSlider->value())
                      .arg(formula).arg(drumToneSlider->value())
                      .arg(drumSnapSlider->value() / 100.0).arg(filterType)
                      .arg(0.1).arg(0.5);

    QPushButton* clickedButton = qobject_cast<QPushButton*>(sender());
    if (clickedButton == btnSaveDrumXpf) {
        QString fileName = QFileDialog::getSaveFileName(this, "Save Drum", "", "LMMS Preset (*.xpf)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly)) {
                QTextStream stream(&file);
                stream << xpf;
                file.close();
                statusBox->setText("Drum saved: " + fileName);
            }
        }
    } else {
        QApplication::clipboard()->setText(xpf);
        statusBox->setText("Drum XPF copied to clipboard!");
    }
}

QString MainWindow::getXpfTemplate() {
    QStringList lines;
    lines << "<?xml version=\"1.0\"?>"
          << "<!DOCTYPE lmms-project>"
          << "<lmms-project version=\"20\" creator=\"WaveConv\" type=\"instrumenttracksettings\">"
          << "<head/>"
          << "<instrumenttracksettings name=\"%1\" muted=\"0\" solo=\"0\">"
          << "<instrumenttrack vol=\"100\" pan=\"0\" basenote=\"%2\" pitchrange=\"1\">"
          << "<instrument name=\"xpressive\">"
          << "<xpressive version=\"0.1\" O1=\"%3\" O2=\"0\" bin=\"\">"
          << "<key/></xpressive></instrument>"
          << "<eldata fcut=\"%4\" fres=\"%5\" ftype=\"%6\" fwet=\"1\">"
          << "<elvol rel=\"0.1\" dec=\"0.5\" sustain=\"0\" amt=\"0\"/>" // AMT=0 for manual ADSR
          << "</eldata></instrumenttrack></instrumenttracksettings></lmms-project>";
    return lines.join("\n");
}


// TAB 10: VELOCILOGIC
void MainWindow::generateVelocilogic() {
    int rows = velMapTable->rowCount();
    if (rows == 0) return;

    QString finalFormula;

    // NIGHTLY (Nested Ternary) ---
    if (velMapMode->currentIndex() == 0) {
        QString nestedBody = "0";
        int startIdx = rows - 1;

        // Base case optimization
        if (velMapTable->item(startIdx, 0)->text().toInt() >= 127) {
            nestedBody = velMapTable->item(startIdx, 1)->text();
            startIdx--;
        }

        for (int i = startIdx; i >= 0; --i) {
            // Convert MIDI (0-127) to Volume (0.0-1.0)
            double rawLimit = velMapTable->item(i, 0)->text().toDouble();
            double normLimit = rawLimit / 127.0;

            QString code = velMapTable->item(i, 1)->text();

            // USE 'v' INSTEAD OF 'vel'
            nestedBody = QString("(v < %1 ? %2 : %3)")
                             .arg(normLimit, 0, 'f', 3)
                             .arg(code)
                             .arg(nestedBody);
        }
        finalFormula = nestedBody;
    }

    // LEGACY (Additive) ---
    else {
        QStringList segments;
        double lowerBound = 0.0;

        for (int i = 0; i < rows; ++i) {
            // Convert MIDI (0-127) to Volume (0.0-1.0)
            double rawLimit = velMapTable->item(i, 0)->text().toDouble();
            double upperBound = rawLimit / 127.0;
            QString code = velMapTable->item(i, 1)->text();

            QString rangeCheck;

            // 1. First Zone
            if (i == 0 && lowerBound <= 0.001) {
                rangeCheck = QString("(v < %1)").arg(upperBound, 0, 'f', 3);
            }
            // 2. Last Zone
            else if (i == rows - 1 && rawLimit >= 127) {
                rangeCheck = QString("(v >= %1)").arg(lowerBound, 0, 'f', 3);
            }
            // 3. Middle Zones (Use * for AND)
            else {
                rangeCheck = QString("((v >= %1) * (v < %2))")
                .arg(lowerBound, 0, 'f', 3)
                    .arg(upperBound, 0, 'f', 3);
            }

            segments << QString("(%1 * (%2))").arg(rangeCheck).arg(code);
            lowerBound = upperBound;
        }
        finalFormula = segments.join(" + ");
    }

    QString result = QString("clamp(-1, %1, 1)").arg(finalFormula);
    statusBox->setText(result);
    QApplication::clipboard()->setText(result);
}

// TAB 11: NOISE FORGE
void MainWindow::generateNoiseForge() {
    statusBox->setText(QString("randv(floor(t * %1))").arg(noiseRes->value()));
}

// TAB 12: XPF PACKAGER
void MainWindow::generateXPFPackager() {
    QString c = xpfInput->toPlainText().replace("\"", "&quot;").replace("<", "&lt;");
    statusBox->setText(QString("<?xml version=\"1.0\"?>\n<xpressive version=\"0.1\" O1=\"%1\" />").arg(c));
}

void MainWindow::saveXpfInstrument() {
    // ... (The logic I gave you in the previous step goes here) ...
    // If you need the full logic block again, let me know!

    QString code = xpfInput->toPlainText();
    if (code.isEmpty()) {
        statusBox->setText("Error: No code to package! Paste something in the XPF tab first.");
        return;
    }

    // Escape XML characters (Basic safety)
    code = code.replace("&", "&amp;")
               .replace("\"", "&quot;")
               .replace("'", "&apos;")
               .replace("<", "&lt;")
               .replace(">", "&gt;")
               .replace("\n", ""); // Remove newlines for the attribute

    // THE MASTER TEMPLATE
    // Based on 'foryoumyfriend.xpf' but cleaned up:
    // - O1="1" (Enabled)
    // - W1="0" (Disabled)
    // - O2="0" (Disabled)
    // - pan="0" (Centered)
    // - src1="..." (Your Code)

    QString xmlContent =
        "<?xml version=\"1.0\"?>\n"
        "<!DOCTYPE lmms-project>\n"
        "<lmms-project creator=\"WaveConv\" version=\"20\">\n"
        "  <head/>\n"
        "  <instrumenttracksettings name=\"WaveConv_Patch\" type=\"0\" muted=\"0\" solo=\"0\">\n"
        "    <instrumenttrack usemasterpitch=\"1\" vol=\"100\" pitch=\"0\" pan=\"0\" basenote=\"57\">\n" // Centered Pan
        "      <instrument name=\"xpressive\">\n"
        "        <xpressive \n"
        "           version=\"0.1\" \n"
        "           gain=\"1\" \n"
        "           O1=\"1\" \n"           // O1 Enabled
        "           O2=\"0\" \n"           // O2 Disabled
        "           W1=\"0\" \n"           // W1 Disabled (Removed as requested)
        "           W2=\"0\" \n"
        "           src1=\"" + code + "\" \n"  // <--- YOUR CODE GOES HERE
                 "           src2=\"\" \n"
                 "           p1=\"0\" p2=\"0\" \n"  // Panning for Oscs (Centered)
                 "           crse1=\"0\" fine1=\"0\" \n"
                 "           crse2=\"0\" fine2=\"0\" \n"
                 "           ph1=\"0\" ph2=\"0\" \n"
                 "           bin=\"\" \n"           // Cleared binary data
                 "        >\n"
                 "          <key/>\n"
                 "        </xpressive>\n"
                 "      </instrument>\n"
                 "      <eldata fcut=\"14000\" fres=\"0.5\" ftype=\"0\" fwet=\"0\">\n"
                 "        <elvol amt=\"1\" att=\"0\" dec=\"0.5\" hold=\"0.5\" rel=\"0.1\" sustain=\"0.5\"/>\n"
                 "        <elcut amt=\"0\"/>\n"
                 "        <elres amt=\"0\"/>\n"
                 "      </eldata>\n"
                 "    </instrumenttrack>\n"
                 "  </instrumenttracksettings>\n"
                 "</lmms-project>\n";

    // Save File Dialog
    QString fileName = QFileDialog::getSaveFileName(this, "Save Instrument", "", "LMMS Instrument (*.xpf)");

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << xmlContent;
        file.close();
        statusBox->setText("Saved successfully to: " + fileName);
    } else {
        statusBox->setText("Error: Could not save file.");
    }
}


// TAB 13: FILTER FORGE
void MainWindow::initFilterForgeTab() {
    filterForgeTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(filterForgeTab);

    // --- 1. WARNING ---
    lblNightlyWarning = new QLabel("⚠️ STRICTLY FOR NIGHTLY BUILD (ExprTk) ⚠️\n"
                                   "Uses 'last(1)' memory. Requires 'saww', 'sqr' etc.");
    lblNightlyWarning->setStyleSheet("QLabel { background-color: #440000; color: #ffcccc; font-weight: bold; padding: 10px; border-radius: 5px; }");
    lblNightlyWarning->setAlignment(Qt::AlignCenter);
    layout->addWidget(lblNightlyWarning);

    // --- 2. CONTROLS ---
    QFormLayout *form = new QFormLayout();

    // Waveform Selector
    filterWaveformCombo = new QComboBox();
    filterWaveformCombo->addItems({"Sawtooth (saww)", "Square (sqr)", "Sine (sinew)", "Triangle (tri)", "Noise (noise)"});
    form->addRow("Waveform:", filterWaveformCombo);

    // Filter Type
    filterTypeCombo = new QComboBox();
    filterTypeCombo->addItems({"Lowpass (Standard)", "Highpass (Subtraction)", "Bandpass (Diff-of-Lowpass)"});
    form->addRow("Filter Type:", filterTypeCombo);

    // Cutoff Slider & Label
    QWidget *cutoffContainer = new QWidget();
    QHBoxLayout *cutoffLay = new QHBoxLayout(cutoffContainer);
    cutoffLay->setContentsMargins(0,0,0,0);

    filterCutoffSlider = new QSlider(Qt::Horizontal);
    filterCutoffSlider->setRange(50, 8000);
    filterCutoffSlider->setValue(2000);

    filterCutoffValueLabel = new QLabel("2000 Hz");
    filterCutoffValueLabel->setFixedWidth(60); // Keep it steady

    cutoffLay->addWidget(filterCutoffSlider);
    cutoffLay->addWidget(filterCutoffValueLabel);

    form->addRow("Cutoff Freq:", cutoffContainer);

    // Width / Q Slider
    filterResSlider = new QSlider(Qt::Horizontal);
    filterResSlider->setRange(10, 2000);
    filterResSlider->setValue(500);
    form->addRow("Width / Q:", filterResSlider);

    layout->addLayout(form);

    // --- 3. CODE OUTPUT ---
    layout->addWidget(new QLabel("Generated Xpressive Code:"));
    filterCodeOutput = new QTextEdit();
    filterCodeOutput->setReadOnly(true);
    filterCodeOutput->setStyleSheet("font-family: Consolas, Monospace; color: #aaddff; background: #222;");
    layout->addWidget(filterCodeOutput);

    // --- CONNECTIONS ---

    // Update the Hz Label and Generate Code when slider moves
    connect(filterCutoffSlider, &QSlider::valueChanged, this, [=](int val){
        filterCutoffValueLabel->setText(QString::number(val) + " Hz");
        generateFilterCode();
    });

    // Update Code for other changes
    connect(filterWaveformCombo, &QComboBox::currentIndexChanged, this, &MainWindow::generateFilterCode);
    connect(filterTypeCombo, &QComboBox::currentIndexChanged, this, &MainWindow::generateFilterCode);
    connect(filterResSlider, &QSlider::valueChanged, this, &MainWindow::generateFilterCode);

    // Initial Generate
    generateFilterCode();
}

void MainWindow::generateFilterCode() {
    QString type = filterTypeCombo->currentText();
    QString waveSel = filterWaveformCombo->currentText();

    double cutoff = filterCutoffSlider->value();
    double width = filterResSlider->value();
    double pi = 3.14159;

    // 1. Determine Input Oscillator
    // Using your verified waveform names
    QString input;
    if (waveSel.contains("Saw")) input = "saww(integrate(f))";
    else if (waveSel.contains("Square")) input = "squarew(integrate(f))";
    else if (waveSel.contains("Sine")) input = "sinew(integrate(f))";
    else if (waveSel.contains("Triangle")) input = "trianglew(integrate(f))";
    else if (waveSel.contains("Noise")) input = "noise()";

    QString code;

    if (type.contains("Lowpass")) {
        // --- LOWPASS (Standard) ---
        // Just O1 is needed.
        QString alpha = QString::number(2 * pi * cutoff);

        code = QString("// --- LOWPASS SETUP ---\n"
                       "// 1. Paste into Slot O1\n"
                       "// 2. Set O2 Volume to 0\n\n"
                       "// SLOT O1 (%1 Hz)\n"
                       "(last(1) + (%2 / srate) * (%3 - last(1)))")
                       .arg(QString::number(cutoff), alpha, input);
    }
    else if (type.contains("Highpass")) {
        // --- HIGHPASS (Phase Cancellation) ---
        // O1 (Dry) + O2 (Inverted Lowpass) = Highpass
        QString alpha = QString::number(2 * pi * cutoff);

        code = QString("// --- HIGHPASS SETUP ---\n"
                       "// 1. Paste Block 1 into O1\n"
                       "// 2. Paste Block 2 into O2\n"
                       "// 3. Set O1 and O2 Volumes to EQUAL\n\n"
                       "// SLOT O1 (Dry Signal)\n"
                       "%3\n\n"
                       "// SLOT O2 (Inverted Lowpass @ %1 Hz)\n"
                       "-1 * (last(1) + (%2 / srate) * (%3 - last(1)))")
                       .arg(QString::number(cutoff), alpha, input);
    }
    else if (type.contains("Bandpass")) {
        // --- BANDPASS (Double Lowpass Difference) ---
        // O1 (Wide LP) - O2 (Narrow LP) = Bandpass
        double upperFreq = cutoff + (width * 0.5);
        double lowerFreq = cutoff - (width * 0.5);
        if (lowerFreq < 50) lowerFreq = 50;

        QString alphaFast = QString::number(2 * pi * upperFreq);
        QString alphaSlow = QString::number(2 * pi * lowerFreq);

        code = QString("// --- BANDPASS SETUP ---\n"
                       "// 1. Paste Block 1 into O1\n"
                       "// 2. Paste Block 2 into O2\n"
                       "// 3. Set O1 and O2 Volumes to EQUAL\n\n"
                       "// SLOT O1 (Upper Cutoff: %4 Hz)\n"
                       "(last(1) + (%1 / srate) * (%3 - last(1)))\n\n"
                       "// SLOT O2 (Lower Cutoff Inverted: %5 Hz)\n"
                       "-1 * (last(1) + (%2 / srate) * (%3 - last(1)))")
                       .arg(alphaFast, alphaSlow, input,
                            QString::number(upperFreq), QString::number(lowerFreq));
    }

    filterCodeOutput->setText(code);
}

// TAB 14: LEAD STACKER
void MainWindow::generateLeadStack() {
    int voices = leadUnisonCount->value();
    double detune = leadDetuneSlider->value() / 200.0; // 0.0 to 0.5 range
    double sub = leadSubSlider->value() / 100.0;
    double noise = leadNoiseSlider->value() / 100.0;
    double drift = leadVibeSlider->value() / 100.0;

    QString wName = leadWaveType->currentText().toLower();
    QString func = "saww";
    if(wName.contains("square")) func = "squarew";
    else if(wName.contains("triangle")) func = "trianglew";
    else if(wName.contains("sine")) func = "sinew";

    QStringList stack;

    // Generate Unison Voices
    for (int i = 0; i < voices; ++i) {
        // Calculate offset centered around 1.0
        // e.g. 5 voices: -2, -1, 0, 1, 2 scaled by detune
        double ratio = (voices > 1) ? ((double)i / (voices - 1)) - 0.5 : 0.0;
        double mult = 1.0 + (ratio * detune);

        QString freqExpr = QString("f * %1").arg(mult, 0, 'f', 4);

        // Add Drift if active (Random slow sine LFO)
        if(drift > 0) {
            freqExpr += QString(" * (1 + %1 * sinew(t*3))").arg(drift * 0.02);
        }

        // Add to stack
        stack << QString("%1(integrate(%2))").arg(func).arg(freqExpr);
    }

    QString finalExpr = "(" + stack.join(" + ") + ")";

    // Normalize Volume (Divide by count)
    finalExpr = QString("(%1 / %2)").arg(finalExpr).arg(voices);

    // Add Sub Oscillator
    if (sub > 0) {
        finalExpr += QString(" + (%1 * squarew(integrate(f*0.5)))").arg(sub);
    }

    // Add Noise
    if (noise > 0) {
        finalExpr += QString(" + (%1 * randv(t*10000))").arg(noise);
    }

    // Final Clamp
    finalExpr = QString("clamp(-1, %1, 1)").arg(finalExpr);

    statusBox->setText(finalExpr);
    QApplication::clipboard()->setText(finalExpr);
}


// TAB 15: RANDOMISER
void MainWindow::generateRandomPatch() {
    int theme = std::rand() % 3;
    double ch = chaosSlider->value() / 100.0;
    QString textStr;
    double pi = 3.14159;

    // We define the math for audio/scope here
    if (theme == 0) { // FM Synthesis
        double c = (std::rand() % 4) + 1;       // Carrier Ratio
        double m = (std::rand() % 8) + 1;       // Mod Ratio
        double i = (std::rand() % 15) * ch + 1; // FM Index (Intensity)

        // String for Clipboard
        textStr = QString("sinew(integrate(f*%1 + sinew(integrate(f*%2))*%3*f*%2))")
                  .arg(c).arg(m).arg(i);

        // Function for Audio Engine
        currentRandFunc = [=](double t) {
            double f = 220.0; // Preview at A3
            double mod = i * std::sin(t * f * m * 2 * pi);
            return std::sin((t * f * c * 2 * pi) + mod);
        };
    }
    else if (theme == 1) { // Bitcrushed Sawtooth
        double steps = (int)(16 * ch + 2);

        textStr = QString("floor(saww(integrate(f)) * %1) / %1").arg(steps);

        currentRandFunc = [=](double t) {
            double f = 220.0;
            double saw = 2.0 * (std::fmod(t * f, 1.0)) - 1.0;
            return std::floor(saw * steps) / steps;
        };
    }
    else { // Additive Sine
        textStr = "sinew(integrate(f)) + 0.5*sinew(integrate(f*2))";

        currentRandFunc = [=](double t) {
            double f = 220.0;
            return std::sin(t * f * 2 * pi) + 0.5 * std::sin(t * f * 2.0 * 2 * pi);
        };
    }

    // Update UI Text
    statusBox->setText(QString("clamp(-1, %1, 1)").arg(textStr));

    // Update Scope
    if (randScope && currentRandFunc) {
        randScope->updateScope(currentRandFunc, 0.05, 1.0); // Fast zoom (0.05s) to see waveform
    }

    // Update Audio (If currently playing)
    if (btnPlayRand->isChecked() && currentRandFunc) {
        m_ghostSynth->setAudioSource(currentRandFunc);
    }
}

// TAB 16: PHONETIC LAB (SAM)
void MainWindow::initSamLibrary() {
    // Phonemes mapped from SAM source tables.
    // https://github.com/s-macke/SAM
    // Structure: {Name, F1, F2, F3, Voiced, A1, A2, A3, LENGTH}

    // --- VOICED VOWELS (Long & Loud) ---
    samLibrary["IY"] = {"IY", 10, 84, 110, true, 15, 10, 5, 18};
    samLibrary["IH"] = {"IH", 14, 73, 93,  true, 15, 10, 5, 15};
    samLibrary["EH"] = {"EH", 19, 67, 91,  true, 15, 10, 5, 16};
    samLibrary["AE"] = {"AE", 24, 63, 88,  true, 15, 10, 5, 18};
    samLibrary["AA"] = {"AA", 27, 40, 89,  true, 15, 10, 5, 18};
    samLibrary["AH"] = {"AH", 23, 44, 87,  true, 15, 10, 5, 16};
    samLibrary["AO"] = {"AO", 21, 31, 88,  true, 15, 10, 5, 18};
    samLibrary["UH"] = {"UH", 16, 37, 82,  true, 15, 10, 5, 15};
    samLibrary["AX"] = {"AX", 20, 45, 89,  true, 15, 10, 5, 12};
    samLibrary["IX"] = {"IX", 14, 73, 93,  true, 15, 10, 5, 12};
    samLibrary["ER"] = {"ER", 18, 49, 62,  true, 15, 10, 5, 18};
    samLibrary["UX"] = {"UX", 14, 36, 82,  true, 15, 10, 5, 15};
    samLibrary["OH"] = {"OH", 18, 30, 88,  true, 15, 10, 5, 18};

    // --- DIPHTHONGS (Very Long) ---
    samLibrary["EY"] = {"EY", 19, 72, 90,  true, 15, 10, 5, 20};
    samLibrary["AY"] = {"AY", 27, 39, 88,  true, 15, 10, 5, 22};
    samLibrary["OY"] = {"OY", 21, 31, 88,  true, 15, 10, 5, 22};
    samLibrary["AW"] = {"AW", 27, 43, 88,  true, 15, 10, 5, 22};
    samLibrary["OW"] = {"OW", 18, 30, 88,  true, 15, 10, 5, 20};
    samLibrary["UW"] = {"UW", 13, 34, 82,  true, 15, 10, 5, 18};

    // --- LIQUIDS & NASALS (Medium) ---
    samLibrary["M*"] = {"M*", 6,  46, 81,  true, 12, 8, 4, 15};
    samLibrary["N*"] = {"N*", 6,  54, 121, true, 12, 8, 4, 15};
    samLibrary["NX"] = {"NX", 6,  86, 101, true, 12, 8, 4, 15};
    samLibrary["R*"] = {"R*", 18, 50, 60,  true, 12, 8, 4, 14};
    samLibrary["L*"] = {"L*", 14, 30, 110, true, 12, 8, 4, 14};
    samLibrary["W*"] = {"W*", 11, 24, 90,  true, 12, 8, 4, 12};
    samLibrary["Y*"] = {"Y*", 9,  83, 110, true, 12, 8, 4, 12};

    // --- VOICED CONSONANTS (Short) ---
    samLibrary["Z*"] = {"Z*", 9,  51, 93,  true, 10, 6, 3, 10};
    samLibrary["ZH"] = {"ZH", 10, 66, 103, true, 10, 6, 3, 10};
    samLibrary["V*"] = {"V*", 8,  40, 76,  true, 10, 6, 3, 8};
    samLibrary["DH"] = {"DH", 10, 47, 93,  true, 10, 6, 3, 8};
    samLibrary["J*"] = {"J*", 6,  66, 121, true, 10, 6, 3, 8};
    samLibrary["B*"] = {"B*", 6,  26, 81,  true, 10, 6, 3, 6};
    samLibrary["D*"] = {"D*", 6,  66, 121, true, 10, 6, 3, 6};
    samLibrary["G*"] = {"G*", 6,  110, 112, true, 10, 6, 3, 6};
    samLibrary["GX"] = {"GX", 6,  84, 94,  true, 10, 6, 3, 6};

    // --- UNVOICED / NOISE ---

    // Fricatives (Sustained Noise)
    samLibrary["S*"] = {"S*", 6,  73, 99,  false, 8, 0, 0, 12};
    samLibrary["SH"] = {"SH", 6,  79, 106, false, 8, 0, 0, 12};
    samLibrary["F*"] = {"F*", 6,  26, 81,  false, 8, 0, 0, 10};
    samLibrary["TH"] = {"TH", 6,  66, 121, false, 8, 0, 0, 10};
    samLibrary["/H"] = {"/H", 14, 73, 93,  false, 8, 0, 0, 10};
    samLibrary["CH"] = {"CH", 6,  79, 101, false, 8, 0, 0, 10};

    // Plosives (Bursts) - UPDATED FOR AUDIBILITY
    samLibrary["P*"] = {"P*", 6,  26, 81,  false, 10, 0, 0, 5};
    samLibrary["T*"] = {"T*", 6,  66, 121, false, 10, 0, 0, 5};
    samLibrary["K*"] = {"K*", 6,  85, 101, false, 10, 0, 0, 6};
    samLibrary["KX"] = {"KX", 6,  84, 94,  false, 10, 0, 0, 6};

    // Special Characters
    samLibrary[" *"] = {" *", 0, 0, 0, false, 0, 0, 0, 5};
    samLibrary[".*"] = {".*", 19, 67, 91, false, 0, 0, 0, 10};

    // Internal Bridge Phonemes
    for(int b=43; b<=77; ++b) {
        QString key = QString("**%1").arg(b);
        if(!samLibrary.contains(key)) samLibrary[key] = {"**", 6, 60, 100, true, 10, 5, 2, 8};
    }
}

void MainWindow::generatePhoneticFormula() {
    QStringList input = phoneticInput->toPlainText().split(" ", Qt::SkipEmptyParts);

    // SETTINGS ---
    double frameTime = 0.012;
    double hzScale = 19.5;

    // Engine Logic: 0 = Legacy (Additive), 1 = Nightly (Nested Ternary)
    bool isNightly = (parsingStyleCombo->currentIndex() == 1);
    bool isLofi = (parserModeCombo->currentIndex() == 1);

    // Temp structure to hold parsed phonemes before assembly
    struct ParsedSegment {
        QString content;
        double duration;
    };
    QList<ParsedSegment> sequence;

    // PARSE INPUT TO DATA ---
    for(int i = 0; i < std::min((int)input.size(), 128); ++i) {
        QString rawToken = input[i].toUpper();
        QRegularExpression re("([A-Z\\*\\/\\.\\?\\,\\-]+)(\\d*)");
        QRegularExpressionMatch match = re.match(rawToken);

        QString key = match.captured(1);
        QString stressStr = match.captured(2);

        if(!samLibrary.contains(key)) continue;
        SAMPhoneme p = samLibrary[key];

        // Stress Logic
        int stress = stressStr.isEmpty() ? 4 : stressStr.toInt();
        double pitchMult = 0.85 + (stress * 0.05);
        double duration = (p.length * frameTime) * (0.8 + (stress * 0.05));

        QString content;

        if (p.voiced) {
            // VOICED SYNTHESIS
            double freq1 = p.f1 * hzScale;
            double freq2 = p.f2 * hzScale;
            double freq3 = p.f3 * hzScale;

            QString s1 = QString("%1*sinew(integrate(%2))").arg(p.a1 * 0.05).arg(freq1);
            QString s2 = QString("%1*sinew(integrate(%2))").arg(p.a2 * 0.05).arg(freq2);
            QString s3 = QString("%1*sinew(integrate(%2))").arg(p.a3 * 0.05).arg(freq3);
            QString glottal = QString("(0.8 * (1 - mod(t*f*%1, 1)))").arg(pitchMult);

            content = QString("((%1 + %2 + %3) * %4)").arg(s1, s2, s3, glottal);
        } else {
            // NOISE SYNTHESIS
            double rawF1 = (p.f1 > 0 ? p.f1 : 100);
            double noiseColor = (rawF1 > 90 ? 90 : rawF1) * 80.0;
            double noiseAmp = (p.length < 8) ? 0.9 : 0.4;

            content = QString("(%1 * randv(t*%2))").arg(noiseAmp).arg(noiseColor);
        }

        sequence.append({content, duration});
    }

    if (sequence.isEmpty()) return;

    QString finalFormula;

    // ASSEMBLE FORMULA ---
    if (isNightly) {
        // NIGHTLY BUILD: Nested Ternary Logic (Recursive Backwards)
        // Matches "Modern" logic in SID Architect

        QString nestedBody = "0"; // End of chain (silence)
        double totalTime = 0;
        for(const auto &s : sequence) totalTime += s.duration;

        double currentTime = totalTime;

        // Iterate backwards from last sound to first
        for (int i = sequence.size() - 1; i >= 0; --i) {
            ParsedSegment seg = sequence[i];
            currentTime -= seg.duration;

            // Envelopes relative to local segment start (currentTime)
            // Fast attack/release to prevent clicks
            double fadeSpeed = 120.0;
            QString attack = QString("min(1, (t-%1)*%2)").arg(currentTime).arg(fadeSpeed);
            QString decay = QString("min(1, (%1-t)*%2)").arg(currentTime + seg.duration).arg(fadeSpeed);

            double tEnd = currentTime + seg.duration;

            // Structure: (t < end_time ? (sound * env) : (next_nested_block))
            nestedBody = QString("(t < %1 ? (%2 * %3 * %4) : %5)")
                             .arg(tEnd, 0, 'f', 4)
                             .arg(seg.content)
                             .arg(attack)
                             .arg(decay)
                             .arg(nestedBody);
        }
        finalFormula = nestedBody;

    } else {
        // LEGACY PARSING: Additive Logic (Forwards)
        // Calculates all segments simultaneously and sums them

        QStringList stringSegments;
        double time = 0.0;

        for (const auto &seg : sequence) {
            double fadeSpeed = 120.0; // Same envelope speed
            QString attack = QString("min(1, (t-%1)*%2)").arg(time).arg(fadeSpeed);
            QString decay = QString("min(1, (%1-t)*%2)").arg(time + seg.duration).arg(fadeSpeed);

            // Structure: ((Range Check) * Sound * Env)
            QString block = QString("((t >= %1 & t < %2) * %3 * %4 * %5)")
                                .arg(time)
                                .arg(time + seg.duration)
                                .arg(seg.content)
                                .arg(attack)
                                .arg(decay);
            stringSegments << block;
            time += seg.duration;
        }
        finalFormula = stringSegments.join(" + ");
    }

    // --- 3. POST-PROCESSING (Audio Quality) ---
    if (isLofi) {
        // Bitcrush / Retro mode: Quantize to 16 steps
        finalFormula = QString("clamp(-1, floor((%1) * 16)/16, 1)").arg(finalFormula);
    } else {
        // High Quality: Simple safety clamp
        finalFormula = QString("clamp(-1, %1, 1)").arg(finalFormula);
    }

    statusBox->setText(finalFormula);
    QApplication::clipboard()->setText(finalFormula);
}

int findScopeAwareChar(const QString &str, char target) {
    int balance = 0;
    for (int i = 0; i < str.length(); ++i) {
        if (str[i] == '(') balance++;
        else if (str[i] == ')') balance--;
        else if (str[i] == target && balance == 0) return i;
    }
    return -1;
}

// TAB 17: LOGIC CONVERTER


QString MainWindow::convertLegacyToNightly(QString input) {
    input = input.trimmed();
    if (input.isEmpty()) return "";


    QString prefix = "";
    QString suffix = "";
    if (input.startsWith("clamp(")) {
        int c = input.indexOf(',');
        int p = input.lastIndexOf(')');
        if (c != -1 && p != -1) {
            prefix = input.left(c + 1) + " ";
            suffix = input.mid(p);
            input = input.mid(c + 1, p - c - 1).trimmed();
        }
    }


    QString compactInput = input;
    compactInput.remove(' ');
    compactInput.remove('\n');


    int tCount = compactInput.count("t<");
    bool isPcm = (tCount > 50) || (tCount > 0 && compactInput.contains("?") && !compactInput.contains("&t<"));

    if (isPcm) {

        double sampleRate = 8000.0;
        QRegularExpression floatReg("0\\.00[0-9]+");
        auto matchIt = floatReg.globalMatch(compactInput);
        double minVal = 1.0;
        int checkCount = 0;

        while (matchIt.hasNext() && checkCount < 100) {
            double val = matchIt.next().captured(0).toDouble();
            if (val > 0.000001 && val < minVal) minVal = val;
            checkCount++;
        }
        if (minVal < 1.0) sampleRate = std::round(1.0 / minVal);

        QString result;
        result.reserve(compactInput.size() + 2048); // Pre-allocate memory!

        int lastPos = 0;
        int processCounter = 0;

        for (int i = 0; i < compactInput.length() - 2; ++i) {
            if (compactInput[i] == 't' && compactInput[i+1] == '<') {
                result.append(QStringView{compactInput}.mid(lastPos, i - lastPos));
                i += 2;
                int numStart = i;
                while (i < compactInput.length() && (compactInput[i].isDigit() || compactInput[i] == '.')) {
                    i++;
                }
                double tVal = QStringView{compactInput}.mid(numStart, i - numStart).toDouble();
                int sVal = std::floor(tVal * sampleRate) - 1;
                if (sVal < 0) sVal = 0;

                result.append("s<=");
                result.append(QString::number(sVal));
                lastPos = i;
                i--; // Adjust index


                if (++processCounter % 15000 == 0) QApplication::processEvents();
            }
        }
        result.append(QStringView{compactInput}.mid(lastPos));

        return QString("var s:=floor(t*%1);\n").arg(int(sampleRate)) + prefix + result + suffix;
    }
    else {

        return prefix + input + suffix;
    }
}

QString MainWindow::convertNightlyToLegacy(QString input) {
    input = input.trimmed();
    if (input.isEmpty()) return "";

    QString prefix = "";
    QString suffix = "";
    if (input.startsWith("clamp(")) {
        int c = input.indexOf(',');
        int p = input.lastIndexOf(')');
        if (c != -1 && p != -1) {
            prefix = input.left(c + 1) + " ";
            suffix = input.mid(p);
            input = input.mid(c + 1, p - c - 1).trimmed();
        }
    }

    QString compactInput = input;
    compactInput.remove(' ');
    compactInput.remove('\n');

    // Mode A: PCM Tree
    int varIdx = compactInput.indexOf("vars:=floor(t*");
    if (varIdx != -1) {
        int endVar = compactInput.indexOf(");", varIdx);
        if (endVar != -1) {
            int rateStart = varIdx + 14;
            double sr = QStringView{compactInput}.mid(rateStart, endVar - rateStart).toDouble();
            if (sr <= 0) sr = 8000.0;

            QString coreInput = compactInput.mid(endVar + 2);

            QString result;
            result.reserve(coreInput.size() + 2048);
            int lastPos = 0;
            int processCounter = 0;

            for (int i = 0; i < coreInput.length() - 3; ++i) {
                if (coreInput[i] == 's' && coreInput[i+1] == '<' && coreInput[i+2] == '=') {
                    result.append(QStringView{coreInput}.mid(lastPos, i - lastPos));
                    i += 3;
                    int numStart = i;
                    while (i < coreInput.length() && coreInput[i].isDigit()) {
                        i++;
                    }
                    double sVal = QStringView{coreInput}.mid(numStart, i - numStart).toDouble();
                    double tVal = (sVal + 1.0) / sr;

                    result.append("t<");
                    result.append(QString::number(tVal, 'f', 6));
                    lastPos = i;
                    i--;

                    if (++processCounter % 15000 == 0) QApplication::processEvents();
                }
            }
            result.append(QStringView{coreInput}.mid(lastPos));
            return prefix + result + suffix;
        }
    }

    // Mode B: Nightly Logic
    if (compactInput.contains("?") && !compactInput.contains("&t<")) {
        QStringList additiveParts;
        double lastStartTime = 0.0;
        QString currentLayer = compactInput;
        int processCounter = 0;

        while (true) {
            int questIdx = currentLayer.indexOf('?');
            if (questIdx == -1) break;

            QString postQuest = currentLayer.mid(questIdx + 1);
            int localColon = findScopeAwareChar(postQuest, ':');
            if (localColon == -1) break;
            int colonIdx = questIdx + 1 + localColon;

            int timeEnd = questIdx - 1;
            int timeStart = currentLayer.lastIndexOf("t<", timeEnd);
            QString timeStr = (timeStart != -1) ? currentLayer.mid(timeStart + 2, timeEnd - timeStart - 1) : "0";

            QString content = currentLayer.mid(questIdx + 1, colonIdx - questIdx - 1);
            QString remainder = currentLayer.mid(colonIdx + 1);

            while (remainder.startsWith("(") && remainder.endsWith(")") &&
                   findScopeAwareChar(remainder.mid(1, remainder.length()-2), ':') != -1) {
                remainder = remainder.mid(1, remainder.length() - 2);
            }

            additiveParts << QString("((t >= %1 & t < %2) * %3)")
                                 .arg(QString::number(lastStartTime, 'f', 6))
                                 .arg(timeStr).arg(content);

            lastStartTime = timeStr.toDouble();
            currentLayer = remainder;

            if (++processCounter % 1000 == 0) QApplication::processEvents();
        }

        if (!additiveParts.isEmpty()) {
            return prefix + additiveParts.join(" + ") + suffix;
        }
    }

    // Mode C: Fallback
    return prefix + input + suffix;
}


// TAB 18: KEY MAPPER
void MainWindow::generateKeyMapper() {
    int rows = keyMapTable->rowCount();
    if (rows == 0) {
        statusBox->setText("Error: Key Map Table is empty.");
        return;
    }

    QString finalFormula;

    // --- MODE A: NIGHTLY (Nested Ternary) ---
    if (keyMapMode->currentIndex() == 0) {
        QString nestedBody = "0";
        int startIdx = rows - 1;

        // Base case: If last row goes to 127, it captures everything else
        if (keyMapTable->item(startIdx, 0)->text().toInt() >= 127) {
            nestedBody = keyMapTable->item(startIdx, 1)->text();
            startIdx--;
        }

        for (int i = startIdx; i >= 0; --i) {
            QString limit = keyMapTable->item(i, 0)->text();
            QString code = keyMapTable->item(i, 1)->text();
            nestedBody = QString("(key < %1 ? %2 : %3)").arg(limit).arg(code).arg(nestedBody);
        }
        finalFormula = nestedBody;
    }

    // --- MODE B: LEGACY (Additive) ---
    else {
        QStringList segments;
        int lowerBound = 0;

        for (int i = 0; i < rows; ++i) {
            int upperBound = keyMapTable->item(i, 0)->text().toInt();
            QString code = keyMapTable->item(i, 1)->text();

            QString rangeCheck;

            // Logic to match your working example exactly
            if (i == 0 && lowerBound == 0) {
                // First Zone: Just check Upper Limit (e.g. key < 60)
                rangeCheck = QString("(key < %1)").arg(upperBound);
            }
            else if (i == rows - 1 && upperBound >= 127) {
                // Last Zone: Just check Lower Limit (e.g. key >= 60)
                rangeCheck = QString("(key >= %1)").arg(lowerBound);
            }
            else {
                // Middle Zone: Check Both (e.g. key >= 40 * key < 60)
                rangeCheck = QString("((key >= %1) * (key < %2))").arg(lowerBound).arg(upperBound);
            }

            segments << QString("((%1) * (%2))").arg(rangeCheck).arg(code);
            lowerBound = upperBound;
        }
        finalFormula = segments.join(" + ");
    }

    QString result = QString("clamp(-1, %1, 1)").arg(finalFormula);
    statusBox->setText(result);
    QApplication::clipboard()->setText(result);
}

// TAB 19: STEP GATE
void MainWindow::generateStepGate() {
    // 1. Calculate Rate
    // Base: 16th notes at Tempo. Formula: (tempo/60) * 4
    QString speedExpr = "(tempo/60.0)*4.0";
    double mult = 1.0;
    int sIdx = gateSpeedCombo->currentIndex();
    if(sIdx == 0) mult = 0.5;
    if(sIdx == 2) mult = 2.0;
    if(sIdx == 3) mult = 4.0;
    if(gateTripletCheck->isChecked()) mult *= 1.5; // Triplet feel (faster)

    speedExpr += QString("*%1").arg(mult);

    // 2. Get Carrier Waveform
    QString wave = "squarew(integrate(f))"; // Default
    int shapeIdx = gateShapeCombo->currentIndex();
    if(shapeIdx == 1) wave = "saww(integrate(f))";
    else if(shapeIdx == 2) wave = "sinew(integrate(f))";
    else if(shapeIdx == 3) wave = "randv(t*10000)";
    else if(shapeIdx == 4) wave = QString("(%1)").arg(gateCustomShape->toPlainText());

    // 3. Build Gate Pattern
    // We map step (0-15) to On(1) or Off(0)
    QString gateLogic;
    bool nightly = (gateBuildMode->currentIndex() == 0);

    if (nightly) {
        // Nightly: Use variables for cleaner code
        // "var step := mod(floor(t * speed), 16);"
        QString stepMap = "0";
        // Build nested ternary backwards: (step == 15 ? val : (step == 14 ? val ...))
        for(int i=15; i>=0; --i) {
            QString val = gateSteps[i]->isChecked() ? "1" : "0";
            stepMap = QString("(step == %1 ? %2 : %3)").arg(i).arg(val).arg(stepMap);
        }
        gateLogic = QString("var step := mod(floor(t * %1), 16);\nvar g := %2;\n(g * %3)").arg(speedExpr).arg(stepMap).arg(wave);
    } else {
        // Legacy: Inline everything (Additive)
        // "((mod(floor(t*rate),16)==0)*1) + ..."
        QStringList parts;
        for(int i=0; i<16; ++i) {
            if(gateSteps[i]->isChecked()) {
                // We use >= i & < i+1 logic to catch the step
                parts << QString("((mod(floor(t*%1),16) >= %2 & mod(floor(t*%1),16) < %3))")
                             .arg(speedExpr).arg(i).arg(i+1);
            }
        }
        if(parts.isEmpty()) gateLogic = "0";
        else gateLogic = QString("(%1) * %2").arg(parts.join(" + ")).arg(wave);
    }

    // 4. Mix Amount (Dry / Wet)
    double mix = gateMixSlider->value() / 100.0;
    if(mix < 1.0) {
        // Formula: (Original_Wave * (1-mix)) + (Gated_Wave * mix)
        gateLogic = QString("((%1 * %2) + (%3 * %4))").arg(wave).arg(1.0-mix).arg(gateLogic).arg(mix);
    }

    statusBox->setText(QString("clamp(-1, %1, 1)").arg(gateLogic));
    QApplication::clipboard()->setText(statusBox->toPlainText());
}

// TAB 20: NUMBERS 1981
void MainWindow::generateNumbers1981() {
    int steps = (numStepsCombo->currentIndex() == 0) ? 16 : 32;
    double dur = numDuration->value();
    bool isRandom = (numModeCombo->currentIndex() == 0);

    // Formula: (tempo / 15) is roughly 4 steps per beat if tempo is 60.
    QString speedMath = "(tempo / 15.0)";

    // Determine the Pitch Source
    QString pitchSource;

    if (isRandom) {
        // Random Logic: randv(-1 to 1) * 12 gives +/- 12 semitones
        pitchSource = QString("randv(floor(mod(t * %1, %2))) * 12")
                          .arg(speedMath).arg(steps);
    } else {
        // Pattern Editor Logic
        QString nested = "0"; // Default

        // Iterate BACKWARDS to build the nest
        for(int i = steps - 1; i >= 0; --i) {
            QString val = numPatternTable->item(0, i)->text();
            if(val.isEmpty()) val = "0";
            nested = QString("(s == %1 ? %2 : %3)").arg(i).arg(val).arg(nested);
        }

        pitchSource = QString("var s := floor(mod(t * %1, %2));\n%3")
                          .arg(speedMath).arg(steps).arg(nested);
    }

    // Gate Logic
    QString gate = QString("(mod(t * %1, 1) < %2)").arg(speedMath).arg(dur);

    // O1 (Left)
    QString o1 = QString("squarew(integrate(f * semitone(%1))) * %2")
                     .arg(pitchSource).arg(gate);

    // O2 (Right) - Detuned + Vibrato
    QString pitchSourceO2 = pitchSource;

    if (isRandom) {
        pitchSourceO2 = QString("randv(floor(mod(t * %1, %2))) * 12 + 0.5 * sinew(t * 12)")
        .arg(speedMath).arg(steps);
    } else {
        pitchSourceO2 += " + 0.5 * sinew(t * 12)";
    }

    QString o2 = QString("squarew(integrate(f * 1.02 * semitone(%1))) * %2")
                     .arg(pitchSourceO2).arg(gate);

    // Output
    numOut1->setText(o1);
    numOut2->setText(o2);
}

// TAB 21: DELAY ARCHITECT
void MainWindow::generateDelayArchitect() {
    // Determine Source Audio
    QString source;
    int idx = delayWaveCombo->currentIndex();
    if (idx == 0) source = "(trianglew(integrate(f)) * exp(-t * 20))"; // Pluck
    else if (idx == 1) source = "(saww(integrate(f)) * exp(-t * 5))";
    else if (idx == 2) source = "(squarew(integrate(f)) * exp(-t * 10))";
    else source = QString("(%1)").arg(delayCustomInput->toPlainText());

    // Calculate Delay Variables
    double time = delayTimeSpin->value();       // e.g. 0.2s
    double rate = delayRateSpin->value();       // e.g. 8000Hz
    double fb = delayFeedbackSpin->value();     // e.g. 0.6
    int taps = delayTapsSpin->value();          // e.g. 4

    int samples = (int)(time * rate);           // Convert Time to Sample Count (e.g. 1600)

    // Build Multitap Chain
    // Formula: Source + Tap1 + Tap2...
    // Tap N: last(samples * N) * (feedback ^ N) * (t > time * N)

    QStringList chain;
    chain << source; // The dry signal

    for(int i = 1; i <= taps; ++i) {
        int offset = samples * i;
        double gain = std::pow(fb, i);
        double startTime = time * i;

        // We check (t > startTime) to prevent garbage data from looking back before t=0
        QString tap = QString("(%1 * last(%2) * (t > %3))")
                          .arg(gain, 0, 'f', 3)
                          .arg(offset)
                          .arg(startTime, 0, 'f', 3);
        chain << tap;
    }

    // Output
    statusBox->setText(QString("clamp(-1, %1, 1)").arg(chain.join(" + ")));
    QApplication::clipboard()->setText(statusBox->toPlainText());
}

// TAB 22: MACRO MORPH
void MainWindow::generateMacroMorph() {
    int style = macroStyleCombo->currentIndex();
    bool isLegacy = (macroBuildMode->currentIndex() == 1);

    // 1. FETCH SLIDER VALUES (Normalized 0.0 to 1.0)
    double mColor = macroColorSlider->value() / 100.0;
    double mTime  = macroTimeSlider->value() / 100.0;
    double mGrit  = macroBitcrushSlider->value() / 100.0; // Bitcrush / Distortion
    double mTex   = macroTextureSlider->value() / 100.0;  // Noise / Grain
    double mWidth = macroWidthSlider->value() / 100.0;    // Detune / Chorus
    double mWonk  = macroWonkySlider->value() / 100.0;    // Sidechain / Swing

    QString osc, env;

    switch(style) {
    case 0: // SUPER SAWS (Anthemic)
        // 3 Detuned Saws averaged
        osc = QString("((saww(integrate(f)) + saww(integrate(f * %1)) + saww(integrate(f * %2))) / 3)")
                  .arg(1.0 + (mWidth * 0.02))
                  .arg(1.0 - (mWidth * 0.02));
        // Color = Lowpass Filter Simulation (Crossfade Sine vs Saw)
        osc = QString("(%1 * %2 + sinew(integrate(f)) * %3)")
                  .arg(osc).arg(mColor).arg(1.0 - mColor);
        // Time = Envelope Decay
        env = QString("min(1, t * 20) * exp(-t * %1)").arg(5.0 - (mTime * 4.0));
        break;

    case 1: // FORMANT VOCAL LEAD (Chops)
    {
        // Base: Triangle wave for body
        QString base = "trianglew(integrate(f/2))";

        double vibSpeed = 6.0;
        double vibDepth = mTime * 0.05;

        if (isLegacy) {
            // Inline LFO logic
            QString lfo = QString("(1.0 + sinew(t*%1)*%2)").arg(vibSpeed).arg(vibDepth);
            osc = QString("(%1 * (0.5 + 0.4 * sinew(integrate(f * %2 * %3))))")
                      .arg(base).arg(lfo).arg(2.0 + (mColor * 3.0));
        } else {
            // Nightly: Use variables
            osc = QString("(%1 * (0.5 + 0.4 * sinew(integrate(f * %2))))")
                      .arg(base).arg(2.0 + (mColor * 3.0));
            // Prepend LFO variable
            if(mTime > 0) {
                osc = QString("var vib:=sinew(t*%1)*%2; %3")
                .arg(vibSpeed).arg(vibDepth)
                    .arg(osc.replace("(f", "(f*(1+vib)"));
            }
        }
        env = "1"; // Sustained
        break;
    }

    case 2: // WOBBLY CASSETTE KEYS (Lo-Fi)
    {
        // Width = Tape Drift amount
        double drift = 1.0 + (mWidth * 0.005);
        // Color = Brightness (Triangle vs Sine)
        osc = QString("(trianglew(integrate(f * %1)) + %2 * sinew(integrate(f * 4)))")
                  .arg(drift).arg(mColor * 0.5);
        // Time = Decay Speed
        env = QString("exp(-t * %1)").arg(10.0 - (mTime * 8.0));
        break;
    }

    case 3: // GRANULAR PAD (Jitter)
        // Texture determines grain frequency
        osc = QString("(saww(integrate(f)) * (0.8 + 0.2 * randv(t * %1)))")
                  .arg(50 + mTex * 500);
        env = QString("min(1, t * %1)").arg(0.5 + mTime * 2.0);
        break;

    case 4: // HOLLOW BASS (Deep House)
        // Square wave
        osc = QString("(squarew(integrate(f)) * (1 - %1 * exp(-t*20)))")
                  .arg(mColor); // Color controls filter pluck amount
        env = "1";
        break;

    case 5: // PORTAMENTO LEAD (Gliding)
        // Width = Detune amount between two saws
        osc = QString("saww(integrate(f)) + 0.5 * saww(integrate(f * %1))").arg(1.0 + mWidth * 0.02);
        env = "1";
        break;

    case 6: // PLUCKY ARP (Short)
        // Uses a conditional to create a pluck shape
        osc = "squarew(integrate(f)) * (sinew(integrate(f*2)) > 0 ? 1 : 0)";
        env = QString("exp(-t * %1)").arg(20.0 - mTime * 10.0);
        break;

    case 7: // VINYL ATMOSPHERE (Texture Only)
        osc = "0"; // Silence base
        env = "1";
        break;

    case 8: // CYBERPUNK BASS [NEW]
    {
        // Sub Osc + Saw
        QString raw = "(saww(integrate(f)) + 0.5*sinew(integrate(f/2)))";
        // Distortion (Grit) controls clamp tightness
        double drive = 1.0 + (mGrit * 5.0);
        // "Fold" effect using clamp
        osc = QString("clamp(-0.8, %1 * %2, 0.8)").arg(raw).arg(drive);
        env = "1"; // Sustained
        break;
    }

    case 9: // HARDSTYLE KICK [NEW]
    {
        // Pitch Envelope: Drops from High to Low
        // Time slider controls drop speed
        double dropSpeed = 10.0 + (mTime * 20.0);
        QString pitchEnv = QString("(f + (400.0 * exp(-t * %1)))").arg(dropSpeed);

        // Core Sine
        osc = QString("sinew(integrate(%1))").arg(pitchEnv);

        // Hard Clip / Distortion (Grit)
        double clip = 1.0 + (mGrit * 10.0);
        osc = QString("clamp(-0.9, %1 * %2, 0.9)").arg(osc).arg(clip);

        env = QString("exp(-t * 3.0)"); // Fixed short decay for kick
        break;
    }

    case 10: // VAPORWAVE E-PIANO [NEW]
    {
        // FM Bell Tone
        // Color controls Modulation Index (Brightness)
        double modIndex = 1.0 + (mColor * 8.0);
        QString modulator = QString("(%1 * sinew(integrate(f * 4.0)))").arg(modIndex);

        // Width controls Chorus LFO
        double lfoSpeed = 2.0;
        double lfoDepth = mWidth * 0.01;
        QString chorus = QString("(1.0 + %1 * sinew(t * %2))").arg(lfoDepth).arg(lfoSpeed);

        osc = QString("sinew(integrate(f * %1 + %2))").arg(chorus).arg(modulator);

        // Time controls Release
        env = QString("exp(-t * %1)").arg(4.0 - (mTime * 3.0));
        break;
    }
    }

    //PROCESSING CHAIN ---

    // Apply Envelope (Skip for Atmosphere)
    if (style != 7) {
        osc = QString("(%1 * %2)").arg(osc).arg(env);
    }

    // TEXTURE LAYER (Noise/Grain)
    if (mTex > 0 || style == 7) {
        // High frequency noise
        QString noise = QString("(randv(t * 8000) * %1)").arg(mTex * 0.25);
        if (style == 7) osc = noise; // Atmosphere is pure noise
        else osc = QString("(%1 + %2)").arg(osc).arg(noise);
    }

    // WONK (Sidechain / Ducking)
    if (mWonk > 0) {
        // AM Modulation simulating sidechain compression
        // 8.0 is roughly 120BPM quarter notes
        QString sidechain = QString("(1.0 - %1 * abs(sinew(t * 8.0)))").arg(mWonk * 0.8);
        osc = QString("(%1 * %2)").arg(osc).arg(sidechain);
    }

    // BITCRUSH (Quantization)
    // Applied last for maximum artifacting
    if (mGrit > 0 && style != 8 && style != 9) {
        // (Skip for Cyberpunk/Hardstyle as they use Grit for distortion instead)
        double steps = 16.0 - (mGrit * 14.0);
        // "floor(x * steps) / steps"
        osc = QString("floor(%1 * %2) / %2").arg(osc).arg(steps);
    }

    // FINAL OUTPUT
    // Clamp result to prevent clipping in LMMS
    QString finalResult = QString("clamp(-1, %1, 1)").arg(osc);

    statusBox->setText(finalResult);
    QApplication::clipboard()->setText(finalResult);
}

// TAB 23: STRING MACHINE
void MainWindow::generateStringMachine() {
    int model = stringModelCombo->currentIndex();
    int chord = stringChordCombo->currentIndex();

    // Normalised Values
    double valEns    = stringEnsembleSlider->value() / 100.0;
    double valAtt    = stringAttackSlider->value() / 100.0;
    double valEvolve = stringEvolveSlider->value() / 100.0;
    double valMotion = stringMotionSlider->value() / 100.0;
    double valAge    = stringAgeSlider->value() / 100.0;
    double valRel    = stringSpaceSlider->value() / 100.0;

    // DEFINE THE OSCILLATOR "CELL" ---
    auto getOsc = [&](double detuneMult, double mix, double phaseOffset) {
        QString shape;

        // $tinkworx / Aquatic Logic:
        // Uses PWM (Pulse Width Modulation) that drifts over time (t)
        if (model == 3) {
            // Pulse wave where width modulates from 50% (Square) to Thin
            double pwmSpeed = 2.0 + (valMotion * 5.0);
            shape = QString("(sinew(integrate(f*%1)) > (0.8 * sinew(t*%2 + %3)) ? 1 : -1)")
                        .arg(detuneMult)
                        .arg(pwmSpeed)
                        .arg(phaseOffset); // Phase offset ensures lines don't overlap on scope
        }
        // String / Solina Logic:
        else {
            // EVOLUTION LOGIC:
            // Mixes Triangle (Dark) -> Saw (Bright) over time based on 'Evolve' slider
            // This replicates your "t" logic but makes it dynamic.

            // Base Tone: Sawtooth
            QString saw = QString("saww(integrate(f*%1))").arg(detuneMult);
            // Dark Tone: Triangle
            QString tri = QString("trianglew(integrate(f*%1))").arg(detuneMult);

            // Evolve factor: 0 = Static Saw, 1 = Tri fading into Saw
            if (valEvolve > 0) {
                // Logic: (Dark * (1-env)) + (Bright * env)
                double speed = 1.0 + (valEvolve * 4.0);
                QString filterEnv = QString("(1 - exp(-t*%1))").arg(speed);
                shape = QString("((%1 * (1-%3)) + (%2 * %3))").arg(tri).arg(saw).arg(filterEnv);
            } else {
                shape = saw; // Pure Amazing String style
            }
        }

        // VISUAL FIX: PHASE MOTION
        // We inject a tiny frequency modulation based on time to make the scope "dance"
        // instead of locking into a static jagged line.
        if (valMotion > 0) {
            shape = shape.replace("(f*", QString("(f * (1 + %1 * sinew(t*3 + %2)) *")
                                      .arg(valMotion * 0.002) // Subtle drift
                                      .arg(phaseOffset));     // Unique per osc
        }

        // VINTAGE AGE (Wobble)
        if (valAge > 0) {
            shape = shape.replace("(f", QString("(f * (1 + %1 * sinew(t*6))").arg(valAge * 0.005));
        }

        return QString("(%1 * %2)").arg(shape).arg(mix);
    };

    // BUILD THE ENSEMBLE ---
    double spread = 1.0 + (valEns * 0.015);

    // We pass distinct Phase Offsets (0, 2, 4) to ensure the visual lines separate
    QString oscC = getOsc(1.0, 0.5, 0.0);
    QString oscL = getOsc(spread, 0.25, 2.0);
    QString oscR = getOsc(2.0 - spread, 0.25, 4.0);

    QString cell = QString("(%1 + %2 + %3)").arg(oscC, oscL, oscR);

    // CHORD MEMORY LOGIC ---
    QString stack;
    if (chord == 0) stack = cell; // Manual
    else if (chord == 1) stack = QString("(%1 + 0.5*%2)").arg(cell).arg(cell.replace("(f", "(f*2"));
    else if (chord == 2) stack = QString("(%1 + 0.5*%2)").arg(cell).arg(cell.replace("(f", "(f*1.498"));
    else if (chord == 3) { // Amazing String (m9)
        QString c1=cell; QString c2=cell; c2.replace("(f","(f*1.189");
        QString c3=cell; c3.replace("(f","(f*1.498");
        QString c4=cell; c4.replace("(f","(f*1.781");
        QString c5=cell; c5.replace("(f","(f*2.245");
        stack = QString("(0.25*%1+0.2*%2+0.2*%3+0.15*%4+0.15*%5)").arg(c1,c2,c3,c4,c5);
    }
    else if (chord == 4) { // $tinkworx Minor 11 (Deep Dub Chord)
        // Root, m3, 5, m7, 11 (No 9th)
        QString c1=cell;
        QString c2=cell; c2.replace("(f","(f*1.189"); // m3
        QString c3=cell; c3.replace("(f","(f*1.498"); // 5
        QString c4=cell; c4.replace("(f","(f*1.781"); // m7
        QString c5=cell; c5.replace("(f","(f*2.669"); // 11 (Perfect 4th up an octave)
        stack = QString("(0.3*%1+0.2*%2+0.2*%3+0.15*%4+0.15*%5)").arg(c1,c2,c3,c4,c5);
    }
    else if (chord == 5) { // Sus4
        QString c1=cell; QString c2=cell; c2.replace("(f","(f*1.334");
        QString c3=cell; c3.replace("(f","(f*1.498");
        stack = QString("(0.4*%1+0.3*%2+0.3*%3)").arg(c1,c2,c3);
    }

    // EVOLUTION ENVELOPE ---
    double attTime = 0.01 + (valAtt * 2.0); // 0 to 2 seconds
    double relTime = 0.1 + (valRel * 3.0);  // Release tail

    // Attack Logic
    QString envLogic = QString("min(1, t / %1)").arg(attTime);

    QString finalResult = QString("(%1 * %2)").arg(stack).arg(envLogic);

    statusBox->setText(QString("clamp(-1, %1, 1)").arg(finalResult));
    QApplication::clipboard()->setText(statusBox->toPlainText());
}

// TAB 24: HARDWARE LAB
void MainWindow::loadHardwarePreset(int idx) {
    if (idx <= 0) return;
    struct HardwarePatch { QString wave; int a, d, s, r, f, q, ps, pd, vs, vd, n; bool peak; };
    QMap<int, HardwarePatch> lib;

    // --- 1. THE STUDIO CLASSICS (1-8) ---
    lib[1] = {"squarew", 0, 30, 0, 10, 1100, 20, 12, 15, 0, 0, 65, false};   // Hissing Minimal
    lib[2] = {"saww", 5, 55, 40, 35, 3800, 30, 0, 0, 8, 10, 5, false};      // Analog Drift
    lib[3] = {"squarew", 0, 25, 0, 10, 2800, 95, 35, 70, 5, 2, 0, true};     // Resonance Burner
    lib[4] = {"trianglew", 0, 15, 0, 5, 4500, 60, 0, 0, 0, 0, 20, true};     // Metallic Tick
    lib[5] = {"trianglew", 2, 45, 15, 20, 450, 45, 45, 40, 0, 0, 0, false};  // PWM Low-End
    lib[6] = {"saww", 0, 70, 60, 30, 14000, 15, 0, 0, 10, 15, 10, true};     // Power Saw
    lib[7] = {"sinew", 10, 80, 50, 60, 1200, 10, 5, 10, 15, 5, 2, false};    // Phase Mod Keys
    lib[8] = {"saww", 60, 90, 80, 90, 900, 5, 10, 20, 12, 12, 15, false};    // Deep Pad

    // --- 2. MODULAR MINIMAL (9-16) ---
    for(int i=9; i<=16; ++i)
        lib[i] = {"squarew", 0, 10+i, 10, 10, 2000+(i*100), 10+(i*2), 5, 5, 0, 0, 5, false};

    // --- 3. INDUSTRIAL WAREHOUSE (17-24) ---
    for(int i=17; i<=24; ++i)
        lib[i] = {"squarew", 0, 20+(i-16)*5, 0, 15, 1000+(i*50), 85, 40, 60, 0, 0, 25, true};

    // --- 4. SIGNAL DRIFT (25-32) ---
    for(int i=25; i<=32; ++i)
        lib[i] = {"saww", 40+(i-24)*5, 80, 70, 85, 1500, 10, 5, 10, 12, 25, 30, false};

    // --- 5. FREQUENCY GLITCH (33-40) ---
    for(int i=33; i<=40; ++i)
        lib[i] = {"trianglew", 0, 5+(i-32)*2, 0, 2, 9000-(i*100), 50, 90, 95, 0, 0, 70, true};

    if (lib.contains(idx)) {
        HardwarePatch p = lib[idx];
        hwBaseWave->setCurrentText(p.wave);
        hwAttack->setValue(p.a); hwDecay->setValue(p.d);
        hwSustain->setValue(p.s); hwRelease->setValue(p.r);
        hwCutoff->setValue(p.f); hwResonance->setValue(p.q);
        hwPwmSpeed->setValue(p.ps); hwPwmDepth->setValue(p.pd);
        hwVibSpeed->setValue(p.vs); hwVibDepth->setValue(p.vd);
        hwNoiseMix->setValue(p.n);
        hwPeakBoost->setChecked(p.peak);


        adsrVisualizer->updateEnvelope(p.a/100.0, p.d/100.0, p.s/100.0, p.r/100.0);
    }
}

void MainWindow::generateHardwareXpf() {
    QString wave = hwBaseWave->currentText();
    QString pitchMod = QString("(1 + sinew(t * %1) * %2)").arg(hwVibSpeed->value()/10.0).arg(hwVibDepth->value()/500.0);
    QString osc = (wave == "squarew") ?
                      QString("(sinew(integrate(f * %1)) > (sinew(t * %2) * %3) ? 1 : -1)").arg(pitchMod).arg(hwPwmSpeed->value()/10.0).arg(hwPwmDepth->value()/100.0) :
                      QString("%1(integrate(f * %2))").arg(wave).arg(pitchMod);

    double nMix = hwNoiseMix->value() / 100.0;
    QString finalSource = QString("((%1 * %2) + (randv(t*10000) * %3))").arg(osc).arg(1.0 - nMix).arg(nMix);
    if (hwPeakBoost->isChecked()) finalSource = QString("clamp(-1, %1 * 1.8, 1)").arg(finalSource);

    QString xml =
        "<?xml version=\"1.0\"?>\n<!DOCTYPE lmms-project>\n"
        "<lmms-project version=\"20\" creator=\"WaveConv\" type=\"instrumenttracksettings\">\n"
        "  <head/>\n"
        "  <instrumenttracksettings name=\"Hardware_Patch\" muted=\"0\" solo=\"0\">\n"
        "    <instrumenttrack vol=\"100\" pan=\"0\" basenote=\"" + QString::number(hwBaseNote->value()) + "\" pitchrange=\"1\">\n"
                                                 "      <instrument name=\"xpressive\">\n"
                                                 "        <xpressive version=\"0.1\" O1=\"" + finalSource.replace("\"", "&quot;") + "\" O2=\"0\" bin=\"\">\n"
                                                "          <key/>\n"
                                                "        </xpressive>\n"
                                                "      </instrument>\n"
                                                "      <eldata fcut=\"" + QString::number(hwCutoff->value()) + "\" fres=\"" + QString::number(hwResonance->value()/100.0) + "\" ftype=\"0\" fwet=\"1\">\n"
                                                                                                              "        <elvol att=\"" + QString::number(hwAttack->value()/100.0) + "\" dec=\"" + QString::number(hwDecay->value()/100.0) + "\" sustain=\"" + QString::number(hwSustain->value()/100.0) + "\" rel=\"" + QString::number(hwRelease->value()/100.0) + "\" amt=\"1\"/>\n"
                                                                                                                                                                                                                                             "      </eldata>\n"
                                                                                                                                                                                                                                             "    </instrumenttrack>\n"
                                                                                                                                                                                                                                             "  </instrumenttracksettings>\n"
                                                                                                                                                                                                                                             "</lmms-project>\n";

    QString fileName = QFileDialog::getSaveFileName(this, "Save Hardware Patch", "", "LMMS Patch (*.xpf)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) { QTextStream(&file) << xml; file.close(); }
    }
}

void MainWindow::generateRandomHardware() {

    hwBaseWave->setCurrentIndex(std::rand() % 4);


    hwAttack->setValue(std::rand() % 40);
    hwDecay->setValue(20 + (std::rand() % 60));
    hwSustain->setValue(std::rand() % 80);
    hwRelease->setValue(10 + (std::rand() % 50));


    hwCutoff->setValue(500 + (std::rand() % 8000));
    hwResonance->setValue(std::rand() % 90);


    hwPwmSpeed->setValue(std::rand() % 100);
    hwPwmDepth->setValue(std::rand() % 70);
    hwVibSpeed->setValue(std::rand() % 50);
    hwVibDepth->setValue(std::rand() % 30);


    hwNoiseMix->setValue(std::rand() % 40);

    statusBox->setText("Hardware Parameters Randomised!");
}

// TAB 25: WEST COAST LAB
void MainWindow::generateWestCoast() {
    int model = westModelSelect->currentIndex();
    double gain = westTimbreSlider->value() / 10.0;
    double bias = westSymmetrySlider->value() / 100.0;
    double modRatio = westModFreqSlider->value() / 10.0;
    double modIndex = westModIndexSlider->value() / 10.0;
    int stages = westFoldStages->value();


    QString modulator = QString("(%1 * sinew(integrate(f * %2)))").arg(modIndex).arg(modRatio);


    QString core = (model == 1) ? "trianglew(integrate(f + " + modulator + "))"
                                : "sinew(integrate(f + " + modulator + "))";


    QString input = QString("(%1 * %2 + %3)").arg(core).arg(gain).arg(bias);


    QString folder;
    if (model == 2) { // Serge Middle VCM (Series Diode String)

        folder = input;
        for(int i = 0; i < stages; ++i) {
            double thresh = 0.2 + (i * 0.1);
            folder = QString("(%1 - 2*clamp(-%2, %1, %2))").arg(folder).arg(thresh);
        }
    } else if (model == 0) { // Model 259 (Parallel Deadband)

        folder = QString("(%1 - 2*clamp(-0.2, %1, 0.2) + 2*clamp(-0.4, %1, 0.4) - 2*clamp(-0.6, %1, 0.6))").arg(input);
    } else { // Default / 208 logic
        folder = QString("((1-%1)*%2 + %1*(2*abs(%2)-1))").arg(westOrderSlider->value()/100.0).arg(input);
    }


    if (westHalfWaveFold->isChecked()) {
        folder = QString("max(0, %1)").arg(folder);
    }


    if (westVactrolSim->isChecked()) {

        folder = QString("(%1 * exp(-t * 15))").arg(folder);
    }

    statusBox->setText(QString("clamp(-1, %1, 1)").arg(folder));
    QApplication::clipboard()->setText(statusBox->toPlainText());
}

// TAB 26. SYNTH ENGINE
    // IN SEPERATE .CPP

// TAB 27: SPECTRAL RESYNTHESISER
void MainWindow::updateSpectralPreview() {
    if (specSampleData.empty()) return;

    int numWindows = (int)specWindowRes->value();
    int topN = specTopHarmonics->value();
    double totalDur = (double)specSampleData.size() / fileFs;
    bool deChord = m_deChordEnabled;
    bool isNightly = (specBuildMode->currentIndex() == 0);
    double decayVal = specDecaySlider->value() / 5.0; // Decay constant


    double fundamental = 440.0;
    QString pText = specPitchCombo->currentText();
    if(pText.contains("C-3")) fundamental = 130.81;
    else if(pText.contains("A-3")) fundamental = 220.00;
    else if(pText.contains("C-4")) fundamental = 261.63;
    else if(pText.contains("C-5")) fundamental = 523.25;


    std::function<double(double)> resynthAlgo = [=](double t) {
        double loopTime = std::fmod(t, totalDur + 0.1);
        if (loopTime >= totalDur) return 0.0;

        // Find current window
        int currentWin = std::floor(loopTime * (numWindows / totalDur));
        if (currentWin >= numWindows) currentWin = numWindows - 1;

        double signal = 0.0;
        for (int i = 1; i <= topN; ++i) {
            // Note: A real FFT would change ratios per window, but here we
            // use a fixed set to demonstrate the windowed expression logic.
            double freq = deChord ? (fundamental * i) : (fundamental * (1.0 + (i-1)*0.25));
            double amp = (1.0 / i) * std::exp(-loopTime * decayVal);
            signal += amp * std::sin(loopTime * freq * 6.2831);
        }
        return signal * 0.3;
    };


    QString finalFormula;
    if (isNightly) {
        finalFormula = QString("var w := floor(t * (%1 / %2));\n").arg(numWindows).arg(totalDur);
        finalFormula += QString("var env := exp(-t * %1);\n").arg(decayVal, 0, 'f', 2);

        QStringList windowBlocks;
        for(int i = 0; i < numWindows; ++i) {
            QStringList harms;
            for(int h = 1; h <= topN; ++h) {
                double ratio = deChord ? (double)h : (1.0 + (h-1)*0.25);
                harms << QString("((1/%1)*sinew(integrate(f*%2)))").arg(h).arg(ratio);
            }
            windowBlocks << QString("(w == %1 ? (%2) : ").arg(i).arg(harms.join("+"));
        }

        finalFormula += "clamp(-1, env * (" + windowBlocks.join("") + "0" + QString(")").repeated(numWindows) + "), 1)";
    } else {
        QStringList parts;
        double winSize = totalDur / numWindows;
        for(int i = 0; i < numWindows; ++i) {
            QStringList harms;
            for(int h = 1; h <= topN; ++h) {
                double ratio = deChord ? (double)h : (1.0 + (h-1)*0.25);
                harms << QString("((1/%1)*exp(-t*%3)*sinew(integrate(f*%2)))").arg(h).arg(ratio).arg(decayVal);
            }
            parts << QString("((t >= %1 & t < %2) * (%3))").arg(i*winSize, 0, 'f', 4).arg((i+1)*winSize, 0, 'f', 4).arg(harms.join("+"));
        }
        finalFormula = "clamp(-1, " + parts.join(" + ") + ", 1)";
    }


    specExpressionBox->setText(finalFormula);
    specScope->updateScope(resynthAlgo, totalDur, 1.0);

    if (btnPlaySpec->isChecked()) {
        m_ghostSynth->setAudioSource(resynthAlgo);
    }
}

// TAB 28: SUBTRACTIVE LAB!!!!!!!
void MainWindow::updateSubtractivePreview() {
    QString waves[] = {"saww", "squarew", "trianglew", "sinew"};
    QString w1 = waves[subOsc1Dial->value()];
    QString w2 = waves[subOsc2Dial->value()];


    double a = (subAttSlider->value() / 100.0) * 0.5 + 0.01;
    double d = (subDecSlider->value() / 100.0) * 1.0 + 0.01;
    double s = subSusSlider->value() / 100.0;
    double r = (subRelSlider->value() / 100.0) * 2.0 + 0.01; // Exponential factor

    double mix2 = subMixSlider->value() / 100.0;
    double mix1 = 1.0 - mix2;
    double detune = 1.0 + (subDetuneSlider->value() / 1200.0);

    // Xpressive ADSR using conditional ternary logic
    QString envExpr = QString(
        "((t < %1) ? (t/%1) : "                       // Attack stage
        "(t < %2) ? (1 - (t-%1)/%3 * (1-%4)) : "      // Decay stage
        "(t < 2.0) ? %4 : "                           // Sustain stage
        "(%4 * exp(-(t-2.0)/%5)))"                    // Exponential Release stage
    ).arg(a).arg(a+d).arg(d).arg(s).arg(r);

    QString finalExpr = QString("clamp(-1, (%1*%2(integrate(f)) + %3*%4(integrate(f*%5))) * %6, 1)")
                        .arg(mix1).arg(w1).arg(mix2).arg(w2).arg(detune).arg(envExpr);

    statusBox->setText(finalExpr); // Output to main UI

    auto subAlgo = [=](double t) {
        double f = 220.0;
        auto getS = [](QString type, double ph) {
            if(type == "saww") return 2.0 * std::fmod(ph, 1.0) - 1.0;
            if(type == "squarew") return (std::fmod(ph, 1.0) > 0.5) ? 1.0 : -1.0;
            if(type == "trianglew") return (2.0/3.14159) * std::asin(std::sin(ph * 6.2831));
            return std::sin(ph * 6.2831);
        };

        double curEnv = 0;
        if (t < a) curEnv = t / a;
        else if (t < a + d) curEnv = 1.0 - ((t - a) / d) * (1.0 - s);
        else if (t < 2.0) curEnv = s;
        else curEnv = s * std::exp(-(t - 2.0) / r);

        return ((mix1 * getS(w1, t * f)) + (mix2 * getS(w2, t * f * detune))) * curEnv;
    };

    subScope->updateScope(subAlgo, 3.0, 1.0); // Visualize full envelope
    if(btnSubPlayMaster->isChecked()) m_ghostSynth->setAudioSource(subAlgo);
}

// TAB 29: PIXEL SYNTH !!!!!!
void MainWindow::generatePixelSynth() {
    if (pixelLoadedImage.isNull()) {
        statusBox->setText("Error: No image loaded!");
        return;
    }


    int steps = pixelTimeSteps->value();       // X-Axis Resolution
    int bands = pixelFreqBands->value();       // Y-Axis Resolution
    int maxPartials = pixelMaxPartials->value();
    double dur = pixelDuration->value();
    double minF = pixelMinFreq->value();
    double maxF = pixelMaxFreq->value();
    bool logScale = pixelLogScale->isChecked();
    bool nightly = (pixelBuildMode->currentIndex() == 0);


    QImage scanImg = pixelLoadedImage.scaled(steps, bands, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);


    QProgressDialog progress("Scanning Pixels & Calculating Harmonics...", "Cancel", 0, steps, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0); // Show immediately

    QStringList timeSlices;
    double timePerStep = dur / steps;


    for (int x = 0; x < steps; ++x) {
        if (progress.wasCanceled()) break;
        progress.setValue(x);


        struct PixelData { double freq; double amp; };
        std::vector<PixelData> columnPixels;

        for (int y = 0; y < bands; ++y) {

            QRgb pixel = scanImg.pixel(x, bands - 1 - y);
            double brightness = qGray(pixel) / 255.0;

            if (brightness > 0.05) { // Noise floor gate

                double normalizedY = (double)y / (bands - 1);
                double freq;

                if (logScale) {

                    freq = minF * std::pow(maxF / minF, normalizedY);
                } else {

                    freq = minF + (maxF - minF) * normalizedY;
                }

                columnPixels.push_back({freq, brightness});
            }
        }


        std::sort(columnPixels.begin(), columnPixels.end(), [](const PixelData &a, const PixelData &b){
            return a.amp > b.amp;
        });


        QStringList oscillatorSum;
        int count = 0;
        for (const auto& p : columnPixels) {
            if (count >= maxPartials) break;

            oscillatorSum << QString("%1*sinew(integrate(%2))").arg(p.amp, 0, 'f', 3).arg(p.freq, 0, 'f', 1);
            count++;
        }

        QString sliceFormula = oscillatorSum.isEmpty() ? "0" : oscillatorSum.join("+");

         timeSlices << sliceFormula;
    }
    progress.setValue(steps);


    QString finalCode;

    if (nightly) {

        QString nested = "0";
        for (int i = steps - 1; i >= 0; --i) {
            nested = QString("(s == %1 ? (%2) : %3)").arg(i).arg(timeSlices[i]).arg(nested);
        }
        double rate = steps / dur;
        finalCode = QString("var s := floor(t * %1);\n%2").arg(rate).arg(nested);

    } else {

        QStringList adds;
        for (int i = 0; i < steps; ++i) {
            double tStart = i * timePerStep;
            double tEnd = (i + 1) * timePerStep;
            adds << QString("((t >= %1 & t < %2) * (%3))")
                    .arg(tStart, 0, 'f', 4).arg(tEnd, 0, 'f', 4).arg(timeSlices[i]);
        }
        finalCode = adds.join(" + ");
    }


    finalCode = QString("clamp(-1, %1, 1)").arg(finalCode);
    statusBox->setText(finalCode);
    QApplication::clipboard()->setText(finalCode);
}


// TAB 30: SCRATCH
void MainWindow::updateScratchPreview() {
    double baseF = scratchVinylPitch->value();
    double handHz = scratchFaderSpeed->value();
    double grit = scratchGritSlider->value() / 100.0;
    int pattern = scratchPatternCombo->currentIndex();

    auto scratchAlgo = [=](double t) {

        double motion = 1.0 + std::sin(t * handHz * 6.2831);


        double fader = 1.0;
        if (pattern == 1) fader = (std::sin(t * handHz * 12.56) > 0) ? 1.0 : 0.0; // Transformer
        if (pattern == 2) fader = (std::sin(t * handHz * 6.28) > 0.6) ? 1.0 : 0.0; // Chirp


        double signal = std::sin(t * baseF * motion * 6.2831);


        if (grit > 0) {
            double noise = ((double)rand()/RAND_MAX * 2.0 - 1.0) * grit * 0.2;
            signal += noise * std::abs(motion);
        }

        return signal * fader;
    };

    scratchScope->updateScope(scratchAlgo, 0.5, 1.0);
    if (btnPlayScratch->isChecked()) m_ghostSynth->setAudioSource(scratchAlgo);
}

void MainWindow::generateScratchLogic() {
    double handHz = scratchFaderSpeed->value();
    int pattern = scratchPatternCombo->currentIndex();
    bool isNightly = (scratchBuildMode->currentIndex() == 0);

    QString formula;
    if (isNightly) {

        QString faderLogic = (pattern == 1) ? QString("squarew(t * %1) > 0 ? 1 : 0").arg(handHz*2) : "1";
        formula = QString("var hand := sinew(t * %1);\n"
                          "var fader := %2;\n"
                          "clamp(-1, saww(integrate(f * (1 + hand))) * fader, 1)")
                  .arg(handHz).arg(faderLogic);
    } else {

        QString fader = (pattern == 1) ? QString("(squarew(t * %1) > 0)").arg(handHz*2) : "1";
        formula = QString("clamp(-1, saww(integrate(f * (1 + sinew(t * %1)))) * %2, 1)")
                  .arg(handHz).arg(fader);
    }

    statusBox->setText(formula);
    QApplication::clipboard()->setText(formula);
}

// TAB 31

void MainWindow::updateNatureLabels(int index) {
    QStringList labels;
    switch(index) {
        case 0: labels = QStringList{"Bird Pitch", "Chirp Rate", "FM Depth", "Flock Size", "Tone", "Distance", "Trill Speed", "Echo"}; break;
        case 1: labels = QStringList{"Cricket Pitch", "Chirp Speed", "Pulse Width", "Jitter", "Leg Scrape", "Resonance", "Swarm", "Dampen"}; break;
        case 2: labels = QStringList{"Wheek Pitch", "Urgency", "Vibrato", "Herd Size", "Breath", "Vocal Formant", "Rise Time", "Fall Time"}; break;
        case 3: labels = QStringList{"Flow Rate", "Turbulence", "Bubble Amt", "Bubble Pitch", "Rock Size", "Depth", "Foam", "Stereo"}; break;
        case 4: labels = QStringList{"Rain Intensity", "Droplet Size", "Wind Speed", "Gust Var", "Roof Tone", "Splash", "Stereo", "Distance"}; break;
        case 5: labels = QStringList{"Rumble Freq", "Crack Rate", "Distance", "Blast Pwr", "Echo Tail", "Wind Howl", "Rain Mix", "Pre-Delay"}; break;
        case 6: labels = QStringList{"Croak Pitch", "Ribbit Rate", "Throat Size", "Wetness", "Swamp Gas", "Echo", "Insect Mix", "Night"}; break;
        default: labels = QStringList{"P1", "P2", "P3", "P4", "P5", "P6", "P7", "P8"}; break;
    }

    for(int i=0; i<8; i++) {
        if(i < labels.size()) natureLabels[i]->setText(labels[i]);
    }
}

void MainWindow::generateNatureLogic() {
    int type = natureTypeCombo->currentIndex();
    bool isNightly = (natureBuildMode->currentIndex() == 0);


    double p1 = natureParam1->value() / 100.0;
    double p2 = natureParam2->value() / 100.0;
    double p3 = natureParam3->value() / 100.0;
    double p4 = natureParam4->value() / 100.0;
    double p5 = natureParam5->value() / 100.0;
    double p6 = natureParam6->value() / 100.0;
    double p7 = natureParam7->value() / 100.0;
    double p8 = natureParam8->value() / 100.0;


    std::function<double(double)> natureAlgo;
    auto noise = [](double s) { return std::sin(s * 12.9898) * 43758.5453 - std::floor(std::sin(s * 12.9898) * 43758.5453); };

    if (type == 0) { // FOREST BIRDS
        natureAlgo = [=](double t) {
            double out = 0.0;
            for(int i=0; i<3; i++) {
                double birdOffset = i * 1.5;
                double rhythm = t * (0.5 + p2 * 2.0) + birdOffset + (noise(std::floor(t)) * p4);
                if (std::fmod(rhythm, 2.0) < 0.2 + (p4*0.1)) {
                    double localT = std::fmod(rhythm, 2.0) * (5.0 + p7*5.0);
                    double pitch = (2000 + p1*3000) + std::sin(localT * (20 + p3*50)) * (500 * p3);
                    double amp = std::exp(-localT * (2.0 + p8));
                    out += std::sin(localT * pitch) * amp * p5;
                }
            }
            return std::clamp(out, -0.8, 0.8);
        };
    }
    else if (type == 1) { // CRICKETS
        natureAlgo = [=](double t) {
            double rate = 5.0 + p2 * 15.0;
            double chirp = std::fmod(t * rate, 1.0);
            double carrier = std::sin(t * (3000 + p1 * 4000) * 6.28);
            double env = (chirp < 0.5) ? std::sin(chirp * 3.14) * (std::sin(t*150)>0?1:0) : 0.0;
            return carrier * env * (0.5 + p5);
        };
    }
    else if (type == 3) { // STREAM
        natureAlgo = [=](double t) {
            double flow = 0.0;
            for(int i=1; i<5; i++) flow += std::sin(t * i * (50 + p1*200) + noise(t*i)) / i;
            double bubbles = (noise(t*100) > (0.95 - p3*0.2)) ? std::sin(t*800*6.28)*0.5 : 0.0;
            return (flow * 0.4 * p5) + (bubbles * p3);
        };
    }

    else {

         natureAlgo = [=](double t) { return (noise(t*5000) * p5 * 0.5); };
    }

    QString code;

    if (type == 0) { // FOREST BIRDS

        double speed = 0.5 + p2 * 2.0;
        double pitch = 1000 + p1 * 2000;
        double fm = 20 + p3 * 50;


        auto birdStr = [&](double offset) {
            QString rhythm = QString("mod(t*%1 + %2, 2.0)").arg(speed).arg(offset);
            QString env = QString("exp(-%1 * 5.0)").arg(rhythm); // Decay
            QString osc = QString("sinew(integrate(%1 + 500*sinew(t*%2)))").arg(pitch).arg(fm);
            return QString("((%1 < 0.3) * %2 * %3)").arg(rhythm).arg(osc).arg(env);
        };

        code = QString("(%1 + %2 + %3) * %4")
               .arg(birdStr(0.0)).arg(birdStr(1.3)).arg(birdStr(2.7)).arg(p5);
    }
    else if (type == 1) {
        double rate = 5.0 + p2 * 15.0;
        double pitch = 3000 + p1 * 4000;


        QString carrier = QString("sinew(integrate(%1))").arg(pitch);
        QString lfo = QString("(sinew(t*%1) > 0)").arg(rate); // On/Off rhythm
        QString pulse = QString("sinew(t*150)"); // The "trill" texture inside the chirp

        code = QString("%1 * %2 * %3 * %4").arg(carrier).arg(lfo).arg(pulse).arg(p5);
    }
    else if (type == 2) {
        double speed = 0.5 + p2;
        double pitch = 400 + p1 * 400;


        QString sweep = QString("mod(t*%1, 2.0)").arg(speed);
        QString osc = QString("saww(integrate(%1 + (%2 * 400)))").arg(pitch).arg(sweep);

        code = QString("((%1 < 0.8) * %2 * sinew(%1 * 3.14)) * %3").arg(sweep).arg(osc).arg(p5);
    }
    else if (type == 3) { // RUSHING STREAM
        QString flow = QString("randv(t*%1)").arg(100 + p1 * 500);
        // Bubble: High pitch sine triggered randomly
        QString bubbleTrig = QString("(randv(t*100) > %1)").arg(0.95 - p3*0.2);
        QString bubble = QString("sinew(integrate(800)) * %1").arg(bubbleTrig);

        code = QString("(%1 * 0.4 + %2) * %3").arg(flow).arg(bubble).arg(p5);
    }
    else if (type == 4) { // RAINSTORM

        QString wind = QString("randv(t*%1)").arg(50 + p4*100); // Low rate noise
        QString rain = QString("(randv(t*10000) > %1 ? randv(t*20000) : 0)").arg(0.95 - p1*0.1);

        code = QString("(%1 * 0.2 + %2) * %3").arg(wind).arg(rain).arg(p5);
    }
    else if (type == 5) { // THUNDER

        double rate = 0.1 + p2 * 0.2;
        QString cycle = QString("mod(t*%1, 1.0)").arg(rate);
        QString boom = QString("((%1 < 0.1) * randv(t) * (1.0 - (%1/0.1)))").arg(cycle);
        QString rumble = QString("sinew(integrate(50)) * %1").arg(boom); // Low freq add

        code = QString("(%1 + %2) * %3").arg(boom).arg(rumble).arg(p4);
    }
    else if (type == 6) { // SWAMP FROGS
        // Low pitch saw with envelope
        double rate = 1.0 + p2 * 2.0;
        QString croak = QString("saww(integrate(100 + %1*50))").arg(p1);
        QString rhythm = QString("sinew(t*%1)").arg(rate);

        code = QString("(%1 * (%2 > 0.5 ? 1 : 0)) * %3").arg(croak).arg(rhythm).arg(p5);
    }


    QString finalResult = QString("clamp(-1, %1, 1)").arg(code);


    if(statusBox) statusBox->setText(finalResult);
    QApplication::clipboard()->setText(finalResult);


    if(natureScope) natureScope->updateScope(natureAlgo, 1.0, 1.0);


    if (btnPlayNature->isChecked()) {
        m_ghostSynth->setAudioSource(natureAlgo);
    }
}
// TAB 32: VECTOR MORPH
void MainWindow::initVectorTab() {
    vectorTab = new QWidget();
    QFormLayout *vectorLayout = new QFormLayout(vectorTab);

    morphX = new QSlider(Qt::Horizontal);
    morphY = new QSlider(Qt::Horizontal);
    btnGenVector = new QPushButton("Generate Vector Code");

    vectorLayout->addRow("X Morph (A1):", morphX);
    vectorLayout->addRow("Y Morph (A2):", morphY);
    vectorLayout->addWidget(btnGenVector);

    connect(btnGenVector, &QPushButton::clicked, [=]() {
        QString code = "( (1-A1)*(1-A2)*squarew(integrate(f)) "
                       "+ (A1)*(1-A2)*sinew(integrate(f)) "
                       "+ (1-A1)*(A2)*randv(t) "
                       "+ (A1)*(A2)*sinew(integrate(f*4) * sinew(integrate(f))) "
                       ")";
        emit expressionGenerated(code);
    });

}
// TAB: 33 PLUCK
void MainWindow::initPluckTab() {
    pluckTab = new QWidget();
    QVBoxLayout *pluckLayout = new QVBoxLayout(pluckTab);

    pluckDamping = new QSlider(Qt::Horizontal);
    pluckDecay = new QSlider(Qt::Horizontal);
    btnGenPluck = new QPushButton("Generate Pluck Code");

    pluckLayout->addWidget(new QLabel("Damping (A1):"));
    pluckLayout->addWidget(pluckDamping);
    pluckLayout->addWidget(new QLabel("Sustain/Decay (A2):"));
    pluckLayout->addWidget(pluckDecay);
    pluckLayout->addWidget(btnGenPluck);

    connect(btnGenPluck, &QPushButton::clicked, [=]() {
        QString code = "O1 = (t < 0.01 ? randv(t) : 0) + "
                       "(last(sr/f) * 0.5 + last(sr/f + 1) * 0.5) * (0.99 - (A1*0.05))";
        emit expressionGenerated(code);
    });
}

// TAB 34: HOUSE MUSIC ORGAN ---
void MainWindow::initHouseOrganTab() {
    houseOrganTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(houseOrganTab);

    houseOrganScope = new UniversalScope();
    houseOrganScope->setMinimumHeight(150);
    layout->addWidget(houseOrganScope);


    QGroupBox *ctrlGroup = new QGroupBox("90s Organ Drawbars & Percussion");
    QFormLayout *fLayout = new QFormLayout(ctrlGroup);

    organDrawbar1 = new QSlider(Qt::Horizontal); organDrawbar1->setRange(0, 100); organDrawbar1->setValue(100);
    organDrawbar2 = new QSlider(Qt::Horizontal); organDrawbar2->setRange(0, 100); organDrawbar2->setValue(60);
    organDrawbar3 = new QSlider(Qt::Horizontal); organDrawbar3->setRange(0, 100); organDrawbar3->setValue(40);
    organClickSlider = new QSlider(Qt::Horizontal); organClickSlider->setRange(0, 100); organClickSlider->setValue(60);
    organDecaySlider = new QSlider(Qt::Horizontal); organDecaySlider->setRange(1, 20); organDecaySlider->setValue(7);

    fLayout->addRow("Fundamental (1.0x):", organDrawbar1);
    fLayout->addRow("Octave (2.0x):", organDrawbar2);
    fLayout->addRow("Twelfth (3.0x):", organDrawbar3);
    fLayout->addRow("Percussion Click (12x):", organClickSlider);
    fLayout->addRow("Bounciness (Decay):", organDecaySlider);

    layout->addWidget(ctrlGroup);

    btnGenOrgan = new QPushButton("GENERATE HOUSE ORGAN FORMULA");
    btnGenOrgan->setStyleSheet("font-weight: bold; background-color: #444; color: white; height: 45px;");
    layout->addWidget(btnGenOrgan);

    btnPlayOrgan = new QPushButton("▶ Play A-2 Preview (110Hz)");
    btnPlayOrgan->setCheckable(true);
    btnPlayOrgan->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 45px;");
    layout->addWidget(btnPlayOrgan);

    modeTabs->addTab(houseOrganTab, "House Organ");


    auto sliders = {organDrawbar1, organDrawbar2, organDrawbar3, organClickSlider, organDecaySlider};
    for(auto *s : sliders) connect(s, &QSlider::valueChanged, this, &MainWindow::updateHouseOrgan);

    connect(btnGenOrgan, &QPushButton::clicked, this, &MainWindow::generateHouseOrgan); // Generator Connection

    connect(btnPlayOrgan, &QPushButton::toggled, [=](bool checked){
        if(checked) {
            btnPlayOrgan->setText("⏹ Stop");
            btnPlayOrgan->setStyleSheet("background-color: #338833; color: white;");
            m_ghostSynth->start();
            updateHouseOrgan();
        } else {
            btnPlayOrgan->setText("▶ Play A-2 Preview (110Hz)");
            btnPlayOrgan->setStyleSheet("background-color: #335533; color: white;");
            m_ghostSynth->stop();
        }
    });

    QTimer::singleShot(500, this, &MainWindow::updateHouseOrgan);
}

void MainWindow::updateHouseOrgan() {
    double amp1 = organDrawbar1->value() / 100.0;
    double amp2 = organDrawbar2->value() / 100.0;
    double amp3 = organDrawbar3->value() / 100.0;
    double clickAmp = organClickSlider->value() / 100.0;
    double decay = organDecaySlider->value();

    std::function<double(double)> organAlgo = [=](double t) {
        double f = 110.0; // A2 Bass Note for preview
        double pi = 3.14159265;

        double body = (amp1 * std::sin(2 * pi * f * 1.0 * t) +
                       amp2 * std::sin(2 * pi * f * 2.0 * t) +
                       amp3 * std::sin(2 * pi * f * 3.0 * t));


        double env = std::exp(-t * decay);

        double click = clickAmp * std::sin(2 * pi * f * 12.0 * t) * std::exp(-t * 80.0);

        return (body * env) + click;
    };

    houseOrganScope->updateScope(organAlgo, 0.4, 1.0);
    if(btnPlayOrgan->isChecked()) {
        m_ghostSynth->setAudioSource(organAlgo);
    }
}

void MainWindow::generateHouseOrgan() {
    double amp1 = organDrawbar1->value() / 100.0;
    double amp2 = organDrawbar2->value() / 100.0;
    double amp3 = organDrawbar3->value() / 100.0;
    double clickAmp = organClickSlider->value() / 100.0;
    double decay = organDecaySlider->value();

    QString body = QString("(%1*sinew(integrate(f)) + %2*sinew(integrate(f*2)) + %3*sinew(integrate(f*3)))")
                   .arg(amp1, 0, 'f', 2).arg(amp2, 0, 'f', 2).arg(amp3, 0, 'f', 2);


    QString bodyWithEnv = QString("(%1 * exp(-t * %2))").arg(body).arg(decay, 0, 'f', 1);

    QString click = QString("(%1 * sinew(integrate(f*12)) * exp(-t*80))").arg(clickAmp, 0, 'f', 2);

    QString finalExpr = QString("clamp(-1, %1 + %2, 1)").arg(bodyWithEnv).arg(click);

    statusBox->setText(finalExpr);
    QApplication::clipboard()->setText(finalExpr);
}


// TAB 35: PCM EDITOR
// External now

 // NOT IN USE

 void MainWindow::initTranspilerTab() {
     transpilerTab = new QWidget();
     QVBoxLayout *layout = new QVBoxLayout(transpilerTab);


     lblTranspilerWarning = new QLabel("⚠️ WORK IN PROGRESS / EXPERIMENTAL ⚠️\n"
                                       "This engine maps ZynAddSubFX XML parameters to Xpressive math.\n"
                                       "Status: Implementing ADSR & Oscillator mapping.");
     lblTranspilerWarning->setStyleSheet("QLabel { background-color: #332200; color: #ffcc00; "
                                         "font-weight: bold; padding: 15px; border: 2px solid #ffcc00; border-radius: 5px; }");
     lblTranspilerWarning->setAlignment(Qt::AlignCenter);
     layout->addWidget(lblTranspilerWarning);


     btnLoadZyn = new QPushButton("📂 LOAD ZYNADDSUBFX PRESET (.xiz / .xpf)");
     btnLoadZyn->setStyleSheet("height: 50px; font-weight: bold; background-color: #444466; color: white;");
     layout->addWidget(btnLoadZyn);

     QHBoxLayout *textLayout = new QHBoxLayout();


     QVBoxLayout *logCol = new QVBoxLayout();
     logCol->addWidget(new QLabel("Parsing Log:"));
     transpilerLog = new QTextEdit();
     transpilerLog->setReadOnly(true);
     transpilerLog->setStyleSheet("background: #111; color: #88ff88; font-family: 'Consolas';");
     logCol->addWidget(transpilerLog);


     QVBoxLayout *outCol = new QVBoxLayout();
     outCol->addWidget(new QLabel("Generated Xpressive.xpf Code:"));
     transpilerOutput = new QTextEdit();
     transpilerOutput->setReadOnly(true);
     transpilerOutput->setStyleSheet("background: #111; color: #88ccff; font-family: 'Consolas';");
     outCol->addWidget(transpilerOutput);

     textLayout->addLayout(logCol, 1);
     textLayout->addLayout(outCol, 2);
     layout->addLayout(textLayout);


     connect(btnLoadZyn, &QPushButton::clicked, [=]() {
         QString path = QFileDialog::getOpenFileName(this, "Select Zyn Preset", "", "Zyn Files (*.xiz *.xpf *.xmz)");
         if (!path.isEmpty()) processZynFile(path);
     });

     modeTabs->addTab(transpilerTab, "X-Transpiler");
 }

 void MainWindow::processZynFile(const QString &filePath) {
     QFile file(filePath);
     if (!file.open(QIODevice::ReadOnly)) return;

     QDomDocument doc;
     if (!doc.setContent(&file)) {
         transpilerLog->append("❌ Error: Could not parse XML.");
         return;
     }
     file.close();

     transpilerLog->append("✅ Successfully loaded: " + filePath);


     QDomElement root = doc.documentElement();


     int oscType = root.firstChildElement("INSTRUMENT").firstChildElement("ADD_SYNTH")
                       .firstChildElement("OSC").attribute("type").toInt();

     QString xpressOsc = translateZynOscillator(oscType);
     transpilerLog->append("Detected Zyn Osc Type: " + QString::number(oscType));


     int zynCutoff = root.firstChildElement("INSTRUMENT").firstChildElement("ADD_SYNTH")
                         .firstChildElement("FILTER").attribute("cutoff").toInt();


     double mappedHz = 20.0 + (zynCutoff / 127.0) * 7980.0;
     double alpha = 2 * 3.14159 * mappedHz;


     QString finalExpr = QString("// Transpiled from ZynAddSubFX\n"
                                 "// Original Cutoff: %1\n"
                                 "(last(1) + (%2 / srate) * (%3(integrate(f)) - last(1)))")
                                 .arg(QString::number(zynCutoff),
                                      QString::number(alpha),
                                      xpressOsc);

     transpilerOutput->setText(finalExpr);
     transpilerLog->append("🚀 Transpilation Complete!");
 }

 QString MainWindow::translateZynOscillator(int type) {
     switch(type) {
         case 0: return "sinew";
         case 1: return "trianglew";
         case 2: return "squarew";
         case 3: return "saww";
         default: return "saww";
     }
 }
