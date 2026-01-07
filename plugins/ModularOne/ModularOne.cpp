/* * ModularOne.cpp * Drone Mode + Crash Fix */
#include "ModularOne.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent> 
#include <QPen>
#include <QRadialGradient>
#include <QDebug>
#include <QDomDocument>
#include <QDomElement>
#include <algorithm> 
#include <cmath>
#include "AudioEngine.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "NotePlayHandle.h"
#include "plugin_export.h"
namespace lmms
{
// ID DEFINITIONS
// VCO 1
const int ID_OSC1_OUT_SINE = 0;
const int ID_OSC1_OUT_SAW  = 1;
const int ID_OSC1_OUT_SQR  = 2; 
const int ID_OSC1_IN_PITCH = 100;
const int ID_OSC1_IN_FM    = 101;
const int ID_OSC1_IN_PWM   = 102; 
// VCO 2 
const int ID_OSC2_OUT_SAW  = 10;
const int ID_OSC2_OUT_SQR  = 11;
// LFO
const int ID_LFO_OUT_TRI  = 3;
const int ID_LFO_OUT_SQR  = 4;
const int ID_LFO_IN_RATE  = 103;
// VCF
const int ID_VCF_IN_AUDIO1 = 105;
const int ID_VCF_IN_AUDIO2 = 106; 
const int ID_VCF_IN_MOD    = 107;
const int ID_VCF_OUT       = 5;
// ADSR
const int ID_ENV_OUT      = 6;
const int ID_ENV_GATE     = 108;
// VCA / MAIN
const int ID_VCA_IN_AUDIO = 109;
const int ID_VCA_IN_CV    = 110;
extern "C"
{
    Plugin::Descriptor PLUGIN_EXPORT ModularOne_plugin_descriptor =
    {
        "ModularOne",
        "ModularOne",
        QT_TRANSLATE_NOOP("PluginBrowser", "Micro Modular"),
        "Ewan",
        0x0100,
        Plugin::Type::Instrument,
        new PluginPixmapLoader("logo"), 
        nullptr,
        nullptr,
    };
}
// =============================================================
// DSP
// =============================================================
struct VoiceOscillator {
    float baseFreq;
    float phase;
    float manualPulseWidth; 
    float out_sine, out_saw, out_sqr;
    struct { const float* signal; } in_pitch, in_fm, in_pwm;
    void reset() { 
        phase = 0.0f; out_sine = 0.0f; out_saw = 0.0f; out_sqr=0.0f; 
        in_pitch.signal = nullptr; in_fm.signal = nullptr; in_pwm.signal = nullptr;
    }
    void process(float sampleRate) {
        float fm = in_fm.signal ? *in_fm.signal : 0.0f;
        float pitchMod = in_pitch.signal ? *in_pitch.signal : 0.0f;
        float freq = baseFreq * std::pow(2.0f, pitchMod + fm);
        if (freq > 20000.0f) freq = 20000.0f;
   
        phase += freq / sampleRate;
        if(phase >= 1.0f) phase -= 1.0f;
     
        out_sine = std::sin(phase * 6.28318f);
        out_saw = 1.0f - (2.0f * phase);
      
        float width = manualPulseWidth;
        if (in_pwm.signal) width += (*in_pwm.signal * 0.4f); 
        width = std::clamp(width, 0.05f, 0.95f);
        out_sqr = (phase < width) ? 1.0f : -1.0f;
    }
};
struct VoiceLFO {
    float baseRate;
    float phase;
    float out_tri, out_square;
    struct { const float* signal; } in_rate;
    void reset() { phase = 0.0f; out_tri = 0.0f; out_square = 0.0f; in_rate.signal = nullptr; }
    void process(float sampleRate) {
        float rateMod = in_rate.signal ? *in_rate.signal : 0.0f;
        float rate = baseRate + (rateMod * 10.0f); 
        if (rate < 0.1f) rate = 0.1f;
        phase += rate / sampleRate;
        if(phase >= 1.0f) phase -= 1.0f;
        out_square = (phase < 0.5f) ? 1.0f : -1.0f;
        out_tri = std::abs(2.0f * phase - 1.0f) * 2.0f - 1.0f;
    }
};
struct VoiceFilter {
    float cutoffBase;
    float resBase;
    float out_low, out_band, out_high; 
    struct { const float* signal; } in_audio1, in_audio2, in_mod;
    void reset() { out_low=0; out_band=0; out_high=0; in_audio1.signal=nullptr; in_audio2.signal=nullptr; in_mod.signal=nullptr; }
    void process(float sampleRate) {
        float a1 = in_audio1.signal ? *in_audio1.signal : 0.0f;
        float a2 = in_audio2.signal ? *in_audio2.signal : 0.0f;
        float audio = (a1 + a2) * 0.7f; 
        float mod = in_mod.signal ? *in_mod.signal : 0.0f;
        float cut = cutoffBase * std::pow(2.0f, mod * 5.0f);
        cut = std::clamp(cut, 20.0f, 18000.0f);
        float res = std::clamp(resBase, 0.0f, 0.95f);
        float f = 2.0f * std::sin(3.14159f * cut / sampleRate);
        float q = 1.0f - res;
        out_low  = out_low + f * out_band;
        out_high = audio - out_low - q * out_band;
        out_band = f * out_high + out_band;
    }
};
struct VoiceEnv {
    float A, D, S, R;
    float out_env, currentLevel;
    enum State { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE } state;
    bool gate_memory; 
    struct { const float* signal; } in_gate;
    void reset() { state = IDLE; currentLevel = 0.0f; out_env = 0.0f; gate_memory = false; in_gate.signal=nullptr; }
    void trigger() { state = ATTACK; }
    void release() { state = RELEASE; }
    void process(float sampleRate) {
        bool gateHigh = false;
        if(in_gate.signal) {
             gateHigh = (*in_gate.signal > 0.5f);
             if(gateHigh && !gate_memory) trigger();
             if(!gateHigh && gate_memory) release();
             gate_memory = gateHigh;
        }
        float rate = 0.0f; float target = 0.0f;
        switch(state) {
            case IDLE: rate=0; target=0; break;
            case ATTACK: target=1.01f; rate=1.0f/(A*sampleRate+1.0f); break;
            case DECAY: target=S; rate=1.0f/(D*sampleRate+1.0f); break;
            case SUSTAIN: target=S; currentLevel=S; rate=0; break;
            case RELEASE: target=-0.01f; rate=1.0f/(R*sampleRate+1.0f); break;
        }
        if(state == ATTACK) {
            currentLevel += (target - currentLevel) * rate * 4.0f; 
            if(currentLevel >= 1.0f) { currentLevel = 1.0f; state = DECAY; }
        } else if (state != IDLE && state != SUSTAIN) {
            currentLevel += (target - currentLevel) * rate * 4.0f;
        }
        if(state == RELEASE && currentLevel <= 0.0f) { currentLevel = 0.0f; state = IDLE; }
        out_env = currentLevel;
    }
};
struct VoiceVCA {
    float driveAmount;
    float finalSignal;
    struct { const float* signal; } in_audio, in_cv;
    void process() {
        float audio = in_audio.signal ? *in_audio.signal : 0.0f;
     
        // FIX: Default CV to 1.0f (Open) if unconnected. 
        // This allows you to hear the Oscillator without patching the envelope.
        float cv = in_cv.signal ? *in_cv.signal : 1.0f;
     
        float sig = audio * cv;
        if (driveAmount > 0.0f) {
            float gain = 1.0f + (driveAmount * 4.0f);
            sig = std::tanh(sig * gain) * 0.8f; 
        }

        finalSignal = sig;
    }
};
class ModularVoice
{
public:
    ModularVoice(std::shared_ptr<const ModularPatch> patch, float sampleRate, float noteFreq, ModularOnePlugin* plugin);
    void nextSample(float* outL, float* outR);
    void releaseNote() { m_env.release(); }
private:
    void wireUp(std::shared_ptr<const ModularPatch> patch);
    VoiceOscillator m_osc1; 
    VoiceOscillator m_osc2; 
    VoiceLFO m_lfo;
    VoiceFilter m_vcf;
    VoiceEnv m_env;
    VoiceVCA m_vca;
    float ZERO = 0.0f; float ONE = 1.0f;
};
ModularVoice::ModularVoice(std::shared_ptr<const ModularPatch> patch, float sampleRate, float noteFreq, ModularOnePlugin* plugin)
{
    m_osc1.reset();
    m_osc1.baseFreq = noteFreq;
    m_osc1.manualPulseWidth = plugin->m_knobPWM.value();

    m_osc2.reset(); 
    float detuneSemitones = plugin->m_knobVCO2Detune.value();
    m_osc2.baseFreq = noteFreq * std::pow(2.0f, detuneSemitones / 12.0f);
    m_osc2.manualPulseWidth = 0.5f; 
  
    m_lfo.reset();
    m_lfo.baseRate = plugin->m_knobLfoRate.value();
 
    m_vcf.reset();
    m_vcf.cutoffBase = plugin->m_knobCutoff.value();
    m_vcf.resBase = plugin->m_knobResonance.value();
  
    m_env.reset();
    m_env.A = plugin->m_knobEnvA.value();
    m_env.D = plugin->m_knobEnvD.value();

    m_env.S = plugin->m_knobEnvS.value();
    m_env.R = plugin->m_knobEnvR.value();
    m_env.trigger(); 
    m_vca.in_audio.signal = nullptr; m_vca.in_cv.signal = nullptr;
    m_vca.driveAmount = plugin->m_knobDrive.value();
    wireUp(patch);
}
void ModularVoice::wireUp(std::shared_ptr<const ModularPatch> patch)
{
    m_osc1.in_pitch.signal=nullptr; m_osc1.in_fm.signal=nullptr; m_osc1.in_pwm.signal=nullptr;
    m_osc2.in_pitch.signal=nullptr; m_osc2.in_fm.signal=nullptr; m_osc2.in_pwm.signal=nullptr;
    m_lfo.in_rate.signal=nullptr;
    m_vcf.in_audio1.signal=nullptr; m_vcf.in_audio2.signal=nullptr; m_vcf.in_mod.signal=nullptr;
    m_env.in_gate.signal=nullptr;
    m_vca.in_audio.signal=nullptr; m_vca.in_cv.signal=nullptr;
    for (const auto& cable : patch->cables) {
        const float* sourcePtr = &ZERO;
        switch (cable.sourceID) {
            case ID_OSC1_OUT_SINE: sourcePtr = &m_osc1.out_sine; break;
            case ID_OSC1_OUT_SAW:  sourcePtr = &m_osc1.out_saw; break;
            case ID_OSC1_OUT_SQR:  sourcePtr = &m_osc1.out_sqr; break;
            case ID_OSC2_OUT_SAW:  sourcePtr = &m_osc2.out_saw; break;
            case ID_OSC2_OUT_SQR:  sourcePtr = &m_osc2.out_sqr; break;
            case ID_LFO_OUT_TRI:  sourcePtr = &m_lfo.out_tri; break;
            case ID_LFO_OUT_SQR:  sourcePtr = &m_lfo.out_square; break;
            case ID_VCF_OUT:      sourcePtr = &m_vcf.out_low; break;
            case ID_ENV_OUT:      sourcePtr = &m_env.out_env; break;
        }
        switch (cable.destID) {
            case ID_OSC1_IN_PITCH: m_osc1.in_pitch.signal = sourcePtr; break;
            case ID_OSC1_IN_FM:    m_osc1.in_fm.signal = sourcePtr; break;
            case ID_OSC1_IN_PWM:   m_osc1.in_pwm.signal = sourcePtr; break;
            case ID_LFO_IN_RATE:  m_lfo.in_rate.signal = sourcePtr; break;
            case ID_VCF_IN_AUDIO1: m_vcf.in_audio1.signal = sourcePtr; break;
            case ID_VCF_IN_AUDIO2: m_vcf.in_audio2.signal = sourcePtr; break;
            case ID_VCF_IN_MOD:   m_vcf.in_mod.signal = sourcePtr; break;
            case ID_ENV_GATE:     m_env.in_gate.signal = sourcePtr; break;
            case ID_VCA_IN_AUDIO: m_vca.in_audio.signal = sourcePtr; break;
            case ID_VCA_IN_CV:    m_vca.in_cv.signal = sourcePtr; break;
        }
    }
}
void ModularVoice::nextSample(float* outL, float* outR)
{
    const float rate = Engine::audioEngine()->outputSampleRate();
    m_env.process(rate);
    m_lfo.process(rate);

    m_osc1.process(rate);

    m_osc2.process(rate);

    m_vcf.process(rate);

    m_vca.process();

    *outL = m_vca.finalSignal;

    *outR = m_vca.finalSignal;
}
// =============================================================
// PLUGIN
// =============================================================
ModularOnePlugin::ModularOnePlugin(InstrumentTrack* t) : Instrument(t, &ModularOne_plugin_descriptor),

    m_knobOscFreq(440.0f, 20.0f, 2000.0f, 1.0f, this, tr("Freq")),

    m_knobPWM(0.5f, 0.05f, 0.95f, 0.01f, this, tr("PW")),

    m_knobVCO2Detune(0.0f, -12.0f, 12.0f, 0.1f, this, tr("Detune")),

    m_knobLfoRate(5.0f, 0.1f, 20.0f, 0.1f, this, tr("Rate")),

    m_knobCutoff(2000.0f, 20.0f, 18000.0f, 10.0f, this, tr("Cutoff")),

    m_knobResonance(0.0f, 0.0f, 0.9f, 0.01f, this, tr("Res")),

    m_knobEnvA(0.01f, 0.0f, 2.0f, 0.01f, this, tr("A")),

    m_knobEnvD(0.4f, 0.0f, 2.0f, 0.01f, this, tr("D")),

    m_knobEnvS(0.5f, 0.0f, 1.0f, 0.01f, this, tr("S")),

    m_knobEnvR(0.5f, 0.0f, 2.0f, 0.01f, this, tr("R")),

    m_knobDrive(0.0f, 0.0f, 1.0f, 0.01f, this, tr("Drive"))

{

    auto p = std::make_shared<ModularPatch>();

    p->cables.push_back({ID_OSC1_OUT_SAW, ID_VCF_IN_AUDIO1});

    p->cables.push_back({ID_VCF_OUT,      ID_VCA_IN_AUDIO});

    p->cables.push_back({ID_ENV_OUT,      ID_VCA_IN_CV});

    std::atomic_store(&m_currentPatch, p);

}
void ModularOnePlugin::playNote(NotePlayHandle* _n, SampleFrame* _working_buffer)

{

    if (!_n->m_pluginData) {

        auto patch = std::atomic_load(&m_currentPatch);

        _n->m_pluginData = new ModularVoice(patch, Engine::audioEngine()->outputSampleRate(), _n->frequency(), this);

    }

    auto voice = static_cast<ModularVoice*>(_n->m_pluginData);

    const fpp_t frames = _n->framesLeftForCurrentPeriod();

    const f_cnt_t offset = _n->noteOffset();

    if (_n->isReleased()) voice->releaseNote();

    for(fpp_t frame = offset; frame < frames + offset; ++frame) {

        float L, R;

        voice->nextSample(&L, &R);

        _working_buffer[frame] = SampleFrame(L, R);

    }

}

void ModularOnePlugin::deleteNotePluginData(NotePlayHandle* _n) {

    delete static_cast<ModularVoice*>(_n->m_pluginData);

}

void ModularOnePlugin::saveSettings(QDomDocument& doc, QDomElement& parent) {

    m_knobOscFreq.saveSettings(doc, parent, "osc_freq");

    m_knobPWM.saveSettings(doc, parent, "pwm");

    m_knobVCO2Detune.saveSettings(doc, parent, "vco2_det");

    m_knobLfoRate.saveSettings(doc, parent, "lfo_rate");

    m_knobCutoff.saveSettings(doc, parent, "cut");

    m_knobResonance.saveSettings(doc, parent, "res");

    m_knobEnvA.saveSettings(doc, parent, "envA");

    m_knobEnvD.saveSettings(doc, parent, "envD");

    m_knobEnvS.saveSettings(doc, parent, "envS");

    m_knobEnvR.saveSettings(doc, parent, "envR");

    m_knobDrive.saveSettings(doc, parent, "drive");

    auto patch = getPatch();

    for(const auto& cable : patch->cables) {

        QDomElement c = doc.createElement("cable");

        c.setAttribute("src", cable.sourceID);

        c.setAttribute("dst", cable.destID);

        parent.appendChild(c);

    }

}

void ModularOnePlugin::loadSettings(const QDomElement& thisElement) {

    m_knobOscFreq.loadSettings(thisElement, "osc_freq");

    m_knobPWM.loadSettings(thisElement, "pwm");

    m_knobVCO2Detune.loadSettings(thisElement, "vco2_det");

    m_knobLfoRate.loadSettings(thisElement, "lfo_rate");

    m_knobCutoff.loadSettings(thisElement, "cut");

    m_knobResonance.loadSettings(thisElement, "res");

    m_knobEnvA.loadSettings(thisElement, "envA");

    m_knobEnvD.loadSettings(thisElement, "envD");

    m_knobEnvS.loadSettings(thisElement, "envS");

    m_knobEnvR.loadSettings(thisElement, "envR");

    m_knobDrive.loadSettings(thisElement, "drive");

    auto newP = std::make_shared<ModularPatch>();

    QDomNode node = thisElement.firstChild();

    while(!node.isNull()) {

        QDomElement e = node.toElement();

        if(!e.isNull() && e.tagName() == "cable") {

            newP->cables.push_back({e.attribute("src").toInt(), e.attribute("dst").toInt()});

        }

        node = node.nextSibling();

    }

    std::atomic_store(&m_currentPatch, newP);

}

std::shared_ptr<ModularPatch> ModularOnePlugin::getPatch() { return std::atomic_load(&m_currentPatch); }

void ModularOnePlugin::addCable(int src, int dst) {

    auto oldP = std::atomic_load(&m_currentPatch);

    auto newP = std::make_shared<ModularPatch>(*oldP);

    auto it = std::remove_if(newP->cables.begin(), newP->cables.end(), [dst](const PatchCable& c){ return c.destID == dst; });

    newP->cables.erase(it, newP->cables.end());

    newP->cables.push_back({src, dst});

    std::atomic_store(&m_currentPatch, newP);

}

void ModularOnePlugin::removeCablesAt(int jackID) {

    auto oldP = std::atomic_load(&m_currentPatch);

    auto newP = std::make_shared<ModularPatch>(*oldP);

    auto it = std::remove_if(newP->cables.begin(), newP->cables.end(), 

        [jackID](const PatchCable& c){ return c.destID == jackID || c.sourceID == jackID; });

    newP->cables.erase(it, newP->cables.end());

    std::atomic_store(&m_currentPatch, newP);

}

void ModularOnePlugin::removeCable(int src, int dst) {

    auto oldP = std::atomic_load(&m_currentPatch);

    auto newP = std::make_shared<ModularPatch>(*oldP);

    auto it = std::remove_if(newP->cables.begin(), newP->cables.end(), [dst, src](const PatchCable& c){ return c.destID == dst && c.sourceID == src; });

    newP->cables.erase(it, newP->cables.end());

    std::atomic_store(&m_currentPatch, newP);

}

gui::PluginView* ModularOnePlugin::instantiateView(QWidget* p) { return new gui::ModularOneView(this, p); }

// =============================================================
// GUI VIEW
// =============================================================

namespace gui {

ModularOneView::ModularOneView(Instrument* i, QWidget* p) : InstrumentViewFixedSize(i, p), m_isDragging(false), m_dragStartJack(-1)

{

    setFixedSize(250, 250); 

    initJacks();

    auto mod = castModel<ModularOnePlugin>(); 

  
    // Fixed lambda

    auto k = [&](FloatModel* m, int x, int y, QString l) {

        Knob* kb = new Knob(this); kb->setModel(m); kb->setLabel(l); kb->move(x, y);

    };

    // VCO 1

    k(&mod->m_knobOscFreq, 50, 25, "Tune"); 

    k(&mod->m_knobPWM, 15, 55, "PW"); 

    // VCO 2

    k(&mod->m_knobVCO2Detune, 60, 105, "Det");

    // LFO

    k(&mod->m_knobLfoRate, 20, 165, "Rate");

    // VCF

    k(&mod->m_knobCutoff, 150, 25, "Cut");

    k(&mod->m_knobResonance, 190, 25, "Res");

    // ADSR

    k(&mod->m_knobEnvA, 130, 115, "A"); k(&mod->m_knobEnvD, 155, 115, "D");

    k(&mod->m_knobEnvS, 180, 115, "S"); k(&mod->m_knobEnvR, 205, 115, "R");

    // Drive

    k(&mod->m_knobDrive, 105, 205, "Drive");

}

void ModularOneView::initJacks() {

    m_jacks.clear();

    auto add = [&](int id, int x, int y, QString name, bool out, QColor c) {

        JackDef j; j.id=id; j.rect=QRect(x,y,16,16); j.label=name; j.isOutput=out; j.color=c;

        m_jacks.push_back(j);

    };

    // VCO 1

    add(ID_OSC1_IN_FM,    10, 25, "FM",  false, Qt::white);

    add(ID_OSC1_IN_PWM,   50, 60, "PWM", false, Qt::white);

    add(ID_OSC1_OUT_SAW,  90, 25, "Saw", true,  Qt::cyan);

    add(ID_OSC1_OUT_SQR,  90, 45, "Sqr", true,  Qt::cyan);

    add(ID_OSC1_OUT_SINE, 90, 65, "Sin", true,  Qt::cyan);

    // VCO 2

    add(ID_OSC2_OUT_SAW,  10, 115, "Saw", true, Qt::magenta);

    add(ID_OSC2_OUT_SQR,  30, 115, "Sqr", true, Qt::magenta);

    // LFO

    add(ID_LFO_OUT_SQR,  60, 175, "Sqr", true,  Qt::yellow);

    add(ID_LFO_OUT_TRI,  80, 175, "Tri", true,  Qt::yellow);

    // VCF

    add(ID_VCF_IN_AUDIO1, 130, 30, "In1", false, Qt::red);

    add(ID_VCF_IN_AUDIO2, 130, 50, "In2", false, Qt::red);

    add(ID_VCF_IN_MOD,    130, 70, "CV",  false, Qt::white);

    add(ID_VCF_OUT,       230, 50, "Out", true,  Qt::green);

    // ADSR

    add(ID_ENV_OUT,       230, 130, "Env", true, Qt::blue);

    // VCA / MAIN (Compact Row)

    add(ID_VCA_IN_AUDIO,  10, 220, "In", false, Qt::red);

    add(ID_VCA_IN_CV,     40, 220, "CV", false, Qt::white);

    add(ID_VCA_IN_AUDIO,  200, 220, "Main", true, Qt::magenta); 

}

void ModularOneView::drawScrew(QPainter& p, int x, int y) {

    p.setPen(Qt::NoPen);

    p.setBrush(QColor(30, 30, 30)); p.drawEllipse(x, y, 8, 8);

    p.setBrush(QColor(180, 180, 180)); p.drawEllipse(x+1, y+1, 6, 6);

    p.setPen(QPen(QColor(50, 50, 50), 1)); p.drawLine(x+2, y+4, x+6, y+4);

}

void ModularOneView::drawPanel(QPainter& p, int x, int y, int w, int h, QString title) {

    p.fillRect(x, y, w, h, QColor(45, 45, 48)); 

    p.setPen(QPen(QColor(20, 20, 20), 1)); p.drawRect(x, y, w, h);

    p.setPen(QColor(150, 150, 150)); p.setFont(QFont("Arial", 7, QFont::Bold));

    p.drawText(x+5, y+12, title);

    drawScrew(p, x+2, y+2); drawScrew(p, x+w-10, y+2);

    drawScrew(p, x+2, y+h-10); drawScrew(p, x+w-10, y+h-10);

}

void ModularOneView::drawFancyJack(QPainter& p, const JackDef& j) {

    p.setPen(QPen(QColor(100, 100, 100), 1)); p.setBrush(QColor(180, 180, 180));

    p.drawEllipse(j.rect);

    p.setBrush(Qt::black); p.drawEllipse(j.rect.adjusted(3, 3, -3, -3));

    p.setPen(QPen(j.color, 2)); p.setBrush(Qt::NoBrush);

    p.drawEllipse(j.rect.adjusted(4, 4, -4, -4));

}

int ModularOneView::getJackAt(QPoint pos) {

    for(size_t i=0; i<m_jacks.size(); ++i) if (m_jacks[i].rect.contains(pos)) return i;

    return -1;

}

void ModularOneView::paintEvent(QPaintEvent* e) {

    InstrumentViewFixedSize::paintEvent(e);

    QPainter p(this);

    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(rect(), QColor(20, 20, 20));

    // Panels (Compacted)

    drawPanel(p, 5, 5, 110, 80, "VCO 1");

    drawPanel(p, 5, 90, 110, 55, "VCO 2");

    drawPanel(p, 5, 150, 110, 55, "LFO");
   
    drawPanel(p, 120, 5, 125, 80, "VCF");

    drawPanel(p, 120, 90, 125, 55, "ADSR");
 
    // OUTPUT Panel

    drawPanel(p, 5, 210, 240, 35, "OUT");

    // Jacks

    p.setFont(QFont("Arial", 6));

    for(const auto& j : m_jacks) {

        drawFancyJack(p, j);

        p.setPen(QColor(180,180,180));

        int tx = j.rect.center().x();

        int ty = j.rect.top() - 4;

        if (j.id == ID_VCA_IN_AUDIO || j.id == ID_VCA_IN_CV) ty = j.rect.bottom() + 10;

        else if (j.id == ID_OSC1_IN_PWM) ty = j.rect.bottom() + 8;
      
        QRect textRect(tx-20, ty-8, 40, 10);

        p.drawText(textRect, Qt::AlignCenter, j.label);

    }
  
    // Cables

    auto mod = castModel<ModularOnePlugin>();

    auto patch = mod->getPatch();

    p.setBrush(Qt::NoBrush);

    for(const auto& c : patch->cables) {

        QPoint pSrc, pDst;

        for(auto& j : m_jacks) {

            if(j.id == c.sourceID) pSrc = j.rect.center();

            if(j.id == c.destID)   pDst = j.rect.center();

        }

        if(pSrc.isNull() || pDst.isNull()) continue;

        QPainterPath path; path.moveTo(pSrc);

        float dist = std::abs(pDst.x() - pSrc.x()) * 0.5f;

        path.cubicTo(pSrc.x()+dist, pSrc.y()+50, pDst.x()-dist, pDst.y()+50, pDst.x(), pDst.y());
     
        p.setPen(QPen(QColor(0,0,0,100), 5)); p.drawPath(path.translated(2,2));

        p.setPen(QPen(QColor(100, 255, 100), 3)); p.drawPath(path);

        p.setPen(QPen(QColor(200, 255, 200, 150), 1)); p.drawPath(path.translated(-1,-1));

    }

    if (m_isDragging) {

        QPoint start = m_jacks[m_dragStartJack].rect.center();

        p.setPen(QPen(Qt::white, 2, Qt::DashLine)); p.drawLine(start, m_currentMousePos);

    }

}

void ModularOneView::mousePressEvent(QMouseEvent* e) {

    int idx = getJackAt(e->pos());

    if (e->button() == Qt::RightButton) {

        if (idx != -1) {

            castModel<ModularOnePlugin>()->removeCablesAt(m_jacks[idx].id);

            update();

        }

        return;

    }

    if (idx != -1) { m_isDragging=true; m_dragStartJack=idx; m_currentMousePos=e->pos(); }

    else InstrumentViewFixedSize::mousePressEvent(e);

}

void ModularOneView::mouseMoveEvent(QMouseEvent* e) {

    if (m_isDragging) { m_currentMousePos=e->pos(); update(); }

    else InstrumentViewFixedSize::mouseMoveEvent(e);

}

void ModularOneView::mouseReleaseEvent(QMouseEvent* e) {

    if (m_isDragging) {

        m_isDragging=false;

        int endIdx = getJackAt(e->pos());

        if (endIdx != -1 && endIdx != m_dragStartJack) {

            auto& j1 = m_jacks[m_dragStartJack];

            auto& j2 = m_jacks[endIdx];

            if (j1.isOutput != j2.isOutput) {

                castModel<ModularOnePlugin>()->addCable(j1.isOutput?j1.id:j2.id, j1.isOutput?j2.id:j1.id);

            }

        }

        update();

    } else InstrumentViewFixedSize::mouseReleaseEvent(e);

}

} // namespace gui

extern "C" { PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* m, void*) { return new ModularOnePlugin(static_cast<InstrumentTrack*>(m)); } }

} // namespace lmms
