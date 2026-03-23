#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QClipboard>
#include <QApplication>
#include <QPainter>
#include <QLabel>
#include <QDial>
#include <QSlider>
#include <QScrollArea>
#include <QMap>
#include <vector>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPainterPath>
#include <QTimer>
#include <functional>
#include <complex>
#include <cmath>
#include "synthengine.h"
#include <QRandomGenerator>
#include <QClipboard>
#include "oscilloscopetab.h"


// ==============================================================================
// DATA STRUCTURES & STRUCTS
// ==============================================================================

// --- PHONETIC LAB STRUCTS ---
struct SAMPhoneme {
    QString name;
    int f1, f2, f3;
    bool voiced;
    int a1 = 15;
    int a2 = 10;
    int a3 = 5;
    int length = 12;
};

// --- SID ARCHITECT STRUCTS ---
struct SidSegment {
    QComboBox* waveType;
    QDoubleSpinBox* duration;
    QDoubleSpinBox* decay;
    QDoubleSpinBox* freqOffset;
    QPushButton* deleteBtn;
    QWidget* container;
};

struct Modulator {
    QComboBox* shape;
    QDoubleSpinBox* rate;
    QDoubleSpinBox* depth;
    QCheckBox* sync;
    QComboBox* multiplier;
};

struct ArpSettings {
    QComboBox* wave;
    QComboBox* chord;
    QDoubleSpinBox* speed;
    QCheckBox* sync;
    QComboBox* multiplier;
};

// --- WAVETABLE STRUCTS ---
struct WavetableStep {
    QString shape;
    int semitones;
    int pwm;
    double duration;
};

// ==============================================================================
// CUSTOM SUB WIDGETS
// ==============================================================================

// --- ENVELOPE VISUALISER ---
class EnvelopeDisplay : public QWidget {
    Q_OBJECT
public:
    explicit EnvelopeDisplay(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(120);
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);
    }
    void updateEnvelope(double a, double d, double s, double r) {
        m_a = a; m_d = d; m_s = s; m_r = r;
        update();
    }
protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        int w = width(), h = height();


        painter.setPen(QColor(45, 45, 45));
        painter.drawLine(w/4, 0, w/4, h);
        painter.drawLine(w/2, 0, w/2, h);
        painter.drawLine(3*w/4, 0, 3*w/4, h);

        QPainterPath path;
        path.moveTo(0, h);

        double x_a = m_a * (w/4.0);
        double x_d = x_a + (m_d * (w/4.0));
        double x_s = x_d + (w/4.0);
        double x_r = x_s + (m_r * (w/4.0));
        double y_s = h - (m_s * (h - 20));

        path.lineTo(x_a, 10);    // Attack
        path.lineTo(x_d, y_s);   // Decay
        path.lineTo(x_s, y_s);   // Sustain
        path.lineTo(x_r, h);     // Release

        painter.setPen(QPen(QColor(0, 255, 120), 3));
        painter.drawPath(path);
    }
private:

    double m_a=0, m_d=0.5, m_s=0.5, m_r=0.1;
};

// --- UNIVERSAL OSCILLOSCOPE ---
class UniversalScope : public QWidget {
    Q_OBJECT
public:
    explicit UniversalScope(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(120);
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);
        m_generator = [](double){ return 0.0; };
    }
    void updateScope(std::function<double(double)> waveFunc, double duration, double zoom) {
        m_generator = waveFunc;
        m_duration = duration;
        m_zoom = zoom;
        update();
    }

    void setHighlight(double start, double end) {
        m_hlStart = start;
        m_hlEnd = end;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor(20, 20, 20));

        int w = width();
        int h = height();
        int midY = h / 2;

        painter.setPen(QPen(QColor(60, 60, 60), 1, Qt::DashLine));
        painter.drawLine(0, midY, w, midY);

        if (w < 1) return;

        double windowSize = 0.02 + (m_duration - 0.02) * m_zoom;
        if(windowSize <= 0) windowSize = 0.01;

        // NEW: Draw the Highlight Zone (Behind the waveform)
        if (m_hlStart >= 0.0 && m_hlEnd > m_hlStart) {
            double x1 = (m_hlStart / windowSize) * w;
            double x2 = (m_hlEnd / windowSize) * w;
            // Draw a semi-transparent green box
            painter.fillRect(QRectF(x1, 0, x2 - x1, h), QColor(0, 255, 120, 40));
            // Draw bright boundary lines
            painter.setPen(QPen(QColor(0, 255, 120, 200), 2, Qt::DotLine));
            painter.drawLine(x1, 0, x1, h);
            painter.drawLine(x2, 0, x2, h);
        }

        QPainterPath path;
        bool started = false;
        int resolution = w;
        painter.setPen(QPen(QColor(0, 255, 255), 2));

        for (int x = 0; x < resolution; ++x) {
            double t = (double)x / (double)resolution * windowSize;
            double sample = m_generator(t);
            if (sample > 1.0) sample = 1.0;
            if (sample < -1.0) sample = -1.0;
            double y = midY - (sample * (midY - 10));
            if (!started) { path.moveTo(x, y); started = true; }
            else { path.lineTo(x, y); }
        }
        painter.drawPath(path);
        painter.setPen(QColor(200, 200, 200));
        QString info = QString("Window: %1s").arg(windowSize, 0, 'f', 3);
        painter.drawText(5, 15, info);
    }
private:
    std::function<double(double)> m_generator;
    double m_duration = 1.0;
    double m_zoom = 0.0;

    double m_hlStart = -1.0;
    double m_hlEnd = -1.0;
};

// --- UNIVERSAL SPECTRUM ANALYZER (FFT) ---
class UniversalSpectrum : public QWidget {
    Q_OBJECT
public:
    explicit UniversalSpectrum(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(120);
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);
        m_generator = [](double){ return 0.0; };
    }
    void updateSpectrum(std::function<double(double)> waveFunc, double sampleRate) {
        m_generator = waveFunc;
        m_sampleRate = sampleRate;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor(10, 10, 15));
        int w = width(); int h = height();
        if (w < 1) return;
        const int N = 512;
        std::vector<std::complex<double>> buffer(N);
        for (int i = 0; i < N; ++i) {
            double t = (double)i / m_sampleRate;
            double sample = m_generator(t);
            double window = 0.5 * (1.0 - std::cos(2.0 * 3.14159 * i / (N - 1)));
            buffer[i] = std::complex<double>(sample * window, 0);
        }
        fft(buffer);
        int numBars = N / 2;
        double barWidth = (double)w / numBars;
        for (int i = 0; i < numBars; ++i) {
            double mag = std::abs(buffer[i]);
            double db = 20.0 * std::log10(mag + 1.0);
            double barHeight = (db * 4.0) * (h / 100.0) * 10.0;
            if (barHeight > h) barHeight = h;
            int r = std::min(255, (int)(i * 255.0 / numBars));
            int g = 255 - r;
            painter.setBrush(QColor(r, g, 100));
            painter.setPen(Qt::NoPen);
            painter.drawRect(QRectF(i * barWidth, h - barHeight, barWidth, barHeight));
        }
        painter.setPen(QColor(200, 200, 200));
        painter.drawText(5, 15, "Spectrum (FFT)");
    }
private:
    std::function<double(double)> m_generator;
    double m_sampleRate = 44100.0;
    void fft(std::vector<std::complex<double>>& x) {
        int n = x.size();
        if (n <= 1) return;
        std::vector<std::complex<double>> even(n / 2), odd(n / 2);
        for (int i = 0; i < n / 2; ++i) { even[i] = x[i * 2]; odd[i] = x[i * 2 + 1]; }
        fft(even); fft(odd);
        for (int k = 0; k < n / 2; ++k) {
            std::complex<double> t = std::polar(1.0, -2 * 3.14159 * k / n) * odd[k];
            x[k] = even[k] + t; x[k + n / 2] = even[k] - t;
        }
    }
};

// --- SID WAVEFORM DISPLAY ---
class WaveformDisplay : public QWidget {
    Q_OBJECT
public:
    explicit WaveformDisplay(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(150);
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);
    }
    void updateData(const std::vector<SidSegment>& segments);
protected:
    void paintEvent(QPaintEvent *) override;
private:
    std::vector<SidSegment> m_segments;
};


// ==============================================================================
// MAIN WINDOW CLASS
// ==============================================================================

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

signals:
    void expressionGenerated(QString code);


    // --- SLOTS (Actions) ---
private slots:
    // General IO
    void loadWav();
    void saveExpr();
    void copyToClipboard();

    // SID Architect
    void addSidSegment();
    void removeSidSegment();
    void clearAllSid();
    void saveSidExpr();

    // Generators (By Tab)
    void generateConsoleWave();
    void generateSFXMacro();
    void generateArpAnimator();
    void generateWavetableForge();
    void generateBesselFM();
    void generateHarmonicLab();
    void generateVelocilogic();
    void generateNoiseForge();
    void generateXPFPackager();
    void generateLeadStack();
    void generateRandomPatch();
    void generateDrumXpf();
    void generatePhoneticFormula();
    void generateStepGate();
    void generateNumbers1981();
    void generateDelayArchitect();
    void generateMacroMorph();
    void generateStringMachine();
    void generateHardwareXpf();
    void generateRandomHardware();
    void generateKeyMapper();
    void generateWestCoast();

    void generateNatureLogic();
    void updateNatureLabels(int index);

    // Preset Loading
    void loadBesselPreset(int index);
    void loadWavetablePreset(int index);
    void loadHardwarePreset(int idx);

    void updateSubtractivePreview();


private:
    // --- CORE & HELPERS ---
    void setupUI();
    void initSamLibrary();
    QString generateLegacyPCM(const std::vector<double>& q, double sr);
    QString generateModernPCM(const std::vector<double>& q, double sr);
    QString getModulatorFormula(int index);
    QString getArpFormula(int index);
    QString getSegmentWaveform(const SidSegment& s, const QString& fBase);
    QString getXpfTemplate();
    QString convertLegacyToNightly(QString input);
    QString convertNightlyToLegacy(QString input);
    void saveXpfInstrument();

    // --- GLOBAL UI ELEMENTS ---
    QTabWidget *modeTabs;
    QTextEdit *statusBox;
    QPushButton *btnSave;
    QPushButton *btnCopy;

    // ------------------------------------
    // TAB 0: OVERVIEW
    // ------------------------------------


    // ------------------------------------
    // TAB 1: SID ARCHITECT
    // ------------------------------------
    QComboBox *buildModeSid;
    QVBoxLayout *sidSegmentsLayout;
    std::vector<SidSegment> sidSegments;
    WaveformDisplay *waveVisualizer;
    Modulator mods[5];
    ArpSettings arps[2];

    // ------------------------------------
    // TAB 2: PCM SAMPLER
    // ------------------------------------
    QComboBox *buildModeCombo;
    QComboBox *sampleRateCombo;
    QDoubleSpinBox *maxDurSpin;
    QCheckBox *normalizeCheck;
    UniversalScope *pcmScope;
    QSlider *pcmZoomSlider;
    QLabel *pcmDisclaimer;
    std::vector<double> originalData;
    QComboBox *bitDepthCombo;
    uint32_t fileFs = 44100;

    // ------------------------------------
    // TAB 3 & 4: CONSOLE LAB / SFX MACRO
    // ------------------------------------
    QComboBox *buildModeConsole;
    QComboBox *consoleWaveType;
    QDoubleSpinBox *consoleSteps;
    UniversalScope *consoleScope;
    UniversalScope *sfxScope;
    QComboBox *buildModeSFX;
    QDoubleSpinBox *sfxStartFreq;
    QDoubleSpinBox *sfxEndFreq;
    QDoubleSpinBox *sfxDur;
    QComboBox *sfxWave;

    // ------------------------------------
    // TAB 5: ARP ANIMATOR
    // ------------------------------------
    UniversalScope *arpScope;
    QPushButton *btnPlayArp;
    QComboBox *buildModeArp;
    QComboBox *arpWave;
    QComboBox *arpInterval1;
    QComboBox *arpInterval2;
    QSlider *arpPwmSlider;
    QCheckBox *arpBpmSync;
    QDoubleSpinBox *arpBpmVal;
    QComboBox *arpSpeedDiv;
    QDoubleSpinBox *arpSpeed;

    // ------------------------------------
    // TAB 6: WAVETABLE FORGE
    // ------------------------------------
    QComboBox *buildModeWavetable;
    QTableWidget *wtTrackerTable;
    QComboBox *wtPresetCombo;
    QCheckBox *wtLoopCheck;
    QComboBox *wtBase;
    QDoubleSpinBox *wtHarmonics;
    UniversalScope *wtScope;

    // ------------------------------------
    // TAB 7: BESSEL FM
    // ------------------------------------
    QComboBox *buildModeBessel;
    QComboBox *besselPresetCombo;
    QComboBox *besselCarrierWave;
    QComboBox *besselModWave;
    QDoubleSpinBox *besselCarrierMult;
    QDoubleSpinBox *besselModMult;
    QDoubleSpinBox *besselModIndex;
    UniversalSpectrum *besselSpectrum;

    // ------------------------------------
    // TAB 8: HARMONIC LAB
    // ------------------------------------
    QComboBox *buildModeHarmonic;
    QSlider *harmonicSliders[16];
    UniversalSpectrum *harmonicSpectrum;

    // ------------------------------------
    // TAB 9: DRUM DESIGNER
    // ------------------------------------
    QComboBox *drumTypeCombo;
    QComboBox *drumWaveCombo;
    QSlider *drumPitchSlider;
    QSlider *drumDecaySlider;
    QSlider *drumToneSlider;
    QSlider *drumSnapSlider;
    QSlider *drumNoiseSlider;
    QSlider *drumPitchDropSlider;
    QSlider *drumPWMSlider;
    QSlider *drumExpSlider;
    UniversalScope *drumScope;
    QPushButton *btnPlayDrum;
    QPushButton *btnGenerateDrum;
    QPushButton *btnSaveDrumXpf;
    QLabel *drumDisclaimer;

    // ------------------------------------
    // TAB 10: VELOCILOGIC
    // ------------------------------------
    QComboBox *buildModeVeloci;
    QComboBox *velMapMode;
    QTableWidget *velMapTable;
    QComboBox *velociType;
    QLabel *velDisclaimer;

    // ------------------------------------
    // TAB 11: NOISE FORGE
    // ------------------------------------
    QComboBox *buildModeNoise;
    QDoubleSpinBox *noiseRes;

    // ------------------------------------
    // TAB 12: XPF PACKAGER
    // ------------------------------------
    QTextEdit *xpfInput;
    QLabel *xpfDisclaimer;
    QPushButton *btnSaveXpf;

    // ------------------------------------
    // TAB 13: FILTER FORGE
    // ------------------------------------
    QWidget *filterForgeTab;
    QComboBox *filterTypeCombo;
    QComboBox *filterWaveformCombo;
    QSlider *filterCutoffSlider;
    QLabel *filterCutoffValueLabel;
    QSlider *filterResSlider;
    QTextEdit *filterCodeOutput;
    QLabel *lblNightlyWarning;

    void initFilterForgeTab();
    void generateFilterCode();
    // ------------------------------------
    // TAB 14: LEAD STACKER
    // ------------------------------------
    UniversalScope *leadScope;
    QComboBox *leadWaveType;
    QSpinBox *leadUnisonCount;
    QSlider *leadDetuneSlider;
    QSlider *leadSubSlider;
    QSlider *leadNoiseSlider;
    QSlider *leadVibeSlider;
    QSlider *leadWidthSlider;
    QPushButton *btnPlayLead;

    // ------------------------------------
    // TAB 15: RANDOMISER
    // ------------------------------------
    QSlider *chaosSlider;
    UniversalScope *randScope;
    QPushButton *btnPlayRand;
    std::function<double(double)> currentRandFunc;

    // ------------------------------------
    // TAB 16: PHONETIC LAB (SAM)
    // ------------------------------------
    QTextEdit *phoneticInput;
    QComboBox *parserModeCombo;
    QComboBox *parsingStyleCombo;
    QPushButton *btnGenPhonetic;
    QLabel *phonemeRefLabel;
    QMap<QString, SAMPhoneme> samLibrary;

    // ------------------------------------
    // TAB 17: LOGIC CONVERTER
    // ------------------------------------
    QTextEdit *convInput;
    QTextEdit *convOutput;
    QPushButton *btnToNightly;
    QPushButton *btnToLegacy;
    QLabel *convDisclaimer;

    // ------------------------------------
    // TAB 18: KEY MAPPER
    // ------------------------------------
    QTableWidget *keyMapTable;
    QComboBox *keyMapMode;
    QLabel *keyMapDisclaimer;

    // ------------------------------------
    // TAB 19: STEP GATE
    // ------------------------------------
    QComboBox *gateBuildMode;
    QComboBox *gateSpeedCombo;
    QCheckBox *gateTripletCheck;
    QComboBox *gateShapeCombo;
    QTextEdit *gateCustomShape;
    QPushButton *gateSteps[16];
    QSlider *gateMixSlider;

    // ------------------------------------
    // TAB 20: NUMBERS 1981
    // ------------------------------------
    QComboBox *numModeCombo;
    QComboBox *numStepsCombo;
    QDoubleSpinBox *numDuration;
    QTableWidget *numPatternTable;
    QTextEdit *numOut1;
    QTextEdit *numOut2;

    // ------------------------------------
    // TAB 21: DELAY ARCHITECT
    // ------------------------------------
    QComboBox *delayWaveCombo;
    QTextEdit *delayCustomInput;
    QDoubleSpinBox *delayTimeSpin;
    QDoubleSpinBox *delayFeedbackSpin;
    QSpinBox *delayTapsSpin;
    QDoubleSpinBox *delayRateSpin;

    // ------------------------------------
    // TAB 22: MACRO MORPH
    // ------------------------------------
    QComboBox *macroStyleCombo;
    QComboBox *macroBuildMode;
    QSlider *macroWonkySlider;
    QSlider *macroColorSlider;
    QSlider *macroTimeSlider;
    QSlider *macroBitcrushSlider;
    QSlider *macroTextureSlider;
    QSlider *macroWidthSlider;
    QSpinBox *macroDetuneSpin;
    UniversalScope *macroScope;
    QPushButton *btnPlayMacro;

    // ------------------------------------
    // TAB 23: STRING MACHINE
    // ------------------------------------
    QComboBox *stringModelCombo;
    QComboBox *stringChordCombo;
    QSlider *stringEnsembleSlider;
    QSlider *stringAttackSlider;
    QSlider *stringEvolveSlider;
    QSlider *stringMotionSlider;
    QSlider *stringSpaceSlider;
    QSlider *stringAgeSlider;
    UniversalScope *stringScope;
    QSlider *stringZoomSlider;

    // ------------------------------------
    // TAB 24: HARDWARE LAB
    // ------------------------------------
    QComboBox *hwBaseWave;
    QComboBox *hwPresetCombo;
    QSlider *hwAttack;
    QSlider *hwDecay;
    QSlider *hwSustain;
    QSlider *hwRelease;
    QSlider *hwCutoff;
    QSlider *hwResonance;
    QCheckBox *hwPeakBoost;
    QSlider *hwPwmSpeed;
    QSlider *hwPwmDepth;
    QSlider *hwVibSpeed;
    QSlider *hwVibDepth;
    QSlider *hwNoiseMix;
    QSpinBox *hwBaseNote;
    EnvelopeDisplay *adsrVisualizer;

    // ------------------------------------
    // TAB 25: WEST COAST LAB
    // ------------------------------------
    QComboBox *westBuildMode;
    QComboBox *westModelSelect;
    QSlider *westTimbreSlider;
    QSlider *westSymmetrySlider;
    QSlider *westOrderSlider;
    QCheckBox *westVactrolSim;
    QSlider *westModFreqSlider;
    QSlider *westModIndexSlider;
    QCheckBox *westHalfWaveFold;
    QSlider *westFoldStages;
    UniversalScope *westScope;
    QSlider *westZoomSlider;

    // ------------------------------------
    // TAB 26. SYNTH ENGINE
    // ------------------------------------
    SynthEngine *m_ghostSynth;

    // -------------------------------------
    // TAB 27: SPECTRAL RESYNTHESISER
    // -------------------------------------
    QComboBox *specBuildMode;
    QComboBox *specPitchCombo;
    QTextEdit *specExpressionBox;
    bool m_deChordEnabled = false;
    QPushButton *btnDeChord;
    UniversalScope *specScope;
    QDoubleSpinBox *specWindowRes;
    QSpinBox *specTopHarmonics;
    QSlider *specDecaySlider;
    QPushButton *btnPlaySpec;
    std::vector<double> specSampleData;
    double specSampleRate = 44100.0;

    void updateSpectralPreview();

    // -------------------------------------
    // TAB 28: SUBTRACTIVE LAB
    // -------------------------------------
    QComboBox *subOsc1Wave, *subOsc2Wave;
    QSlider *subDetuneSlider, *subMixSlider;
    QSlider *subAttSlider, *subDecSlider, *subSusSlider, *subRelSlider;
    QCheckBox *subShowMathToggle;
    UniversalScope *subScope;
    QPushButton *btnPlaySub;
    QLabel *subMathOverlay;
    QDial *subOsc1Dial, *subOsc2Dial;
    UniversalScope *subSpectrum;
    QPushButton *btnSubPlayMaster;

    // ------------------------------------
    // TAB 29: PIXEL SYNTH
    // ------------------------------------
    QWidget *pixelTab;
    QLabel *pixelPreviewLabel;
    QImage pixelLoadedImage;

    QComboBox *pixelBuildMode;
    QSpinBox *pixelTimeSteps;
    QSpinBox *pixelFreqBands;
    QSlider *pixelMaxPartials;
    QDoubleSpinBox *pixelDuration;
    QDoubleSpinBox *pixelMinFreq;
    QDoubleSpinBox *pixelMaxFreq;
    QCheckBox *pixelLogScale;

    QPushButton *btnLoadPixel;
    QPushButton *btnGenPixel;
    QPushButton *btnPlayPixel;

    void generatePixelSynth();

    // -------------------------------------
    // TAB 30: SCRATCH GENERATOR
    // -------------------------------------
    QComboBox *scratchBuildMode;
    QComboBox *scratchPatternCombo;
    QSlider *scratchVinylPitch;
    QSlider *scratchFaderSpeed;
    QSlider *scratchGritSlider;
    UniversalScope *scratchScope;
    QPushButton *btnPlayScratch;
    void generateScratchLogic();
    void updateScratchPreview();

    // -------------------------------------
    // TAB 31: NATURE LAB
    // -------------------------------------
    QComboBox *natureTypeCombo;
    QComboBox *natureBuildMode;
    QSlider *natureParam1;
    QSlider *natureParam2;
    QSlider *natureParam3;
    QSlider *natureParam4;
    QSlider *natureParam5;
    QSlider *natureParam6;
    QSlider *natureParam7;
    QSlider *natureParam8;

    QLabel *natureLabels[8];

    UniversalScope *natureScope;
    QPushButton *btnPlayNature;
    QPushButton *btnGenNature;

    // -------------------------------------
    // TAB 32: VECTOR MORPH
    // -------------------------------------
    QWidget *vectorTab;
    QSlider *morphX;
    QSlider *morphY;
    QPushButton *btnGenVector;
    void initVectorTab();

    // -------------------------------------
    // TAB 33: PLUCK LAB
    // -------------------------------------
    QWidget *pluckTab;
    QSlider *pluckDamping;
    QSlider *pluckDecay;
    QPushButton *btnGenPluck;
    void initPluckTab();

    // -------------------------------------
    // TAB 34: HOUSE MUSIC ORGAN
    // -------------------------------------
    QWidget *houseOrganTab;
    UniversalScope *houseOrganScope;
    QSlider *organDrawbar1, *organDrawbar2, *organDrawbar3;
    QSlider *organClickSlider, *organDecaySlider;
    QPushButton *btnPlayOrgan;
    QPushButton *btnGenOrgan;

    void initHouseOrganTab();
    void updateHouseOrgan();
    void generateHouseOrgan();


    // -------------------------------------
    // TAB 35
    // -------------------------------------

    // -------------------------------------
    // TAB 36
    // -------------------------------------
    OscilloscopeTab* oscTab;

    // -------------------------------------
    // TAB 37
    // -------------------------------------

    // -------------------------------------
    // TAB 38
    // -------------------------------------

    // -------------------------------------
    // TAB 39
    // -------------------------------------



    // -------------------------------------
    // TAB UNUSED: X-TRANSPILER (Zyn to Xpressive)
    // -------------------------------------
    QWidget *transpilerTab;
    QPushButton *btnLoadZyn;
    QTextEdit *transpilerLog;
    QTextEdit *transpilerOutput;
    QLabel *lblTranspilerWarning;

    void initTranspilerTab();
    void processZynFile(const QString &filePath);
    QString translateZynOscillator(int type);




};

#endif // MAINWINDOW_H
