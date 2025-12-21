/*

 * WaveScanner.cpp

 * Implements MipMapping, Unison, and Stereo Filter

 */



#include <cmath>

#include <algorithm>

#include <memory>

#include <atomic>

#include <QDomElement>

#include <QPainter>

#include <QLinearGradient>



#include "WaveScanner.h"

#include "AudioEngine.h"

#include "base64.h"

#include "Engine.h"

#include "InstrumentTrack.h"

#include "Knob.h"

#include "LedCheckBox.h"

#include "NotePlayHandle.h"

#include "lmms_math.h"

#include "embed.h"

#include "plugin_export.h"



namespace lmms

{



static const int TABLE_FRAMES = 64;

static const int WAVE_SIZE = 256;

static const int MIP_LEVELS = 8;



extern "C"

{

    Plugin::Descriptor PLUGIN_EXPORT wavescanner_plugin_descriptor =

    {

        "WaveScanner",

        "WaveScanner",

        QT_TRANSLATE_NOOP("PluginBrowser", "Hyper Wavetable Synthesizer"),

        "Ewan",

        0x0100,

        Plugin::Type::Instrument,

        new PluginPixmapLoader("logo"),

        nullptr,

        nullptr,

    };

}



// =============================================================

// MATH HELPER: Band-Limiting

// =============================================================



std::vector<float> generateMipLevel(const std::vector<float>& src) {

    std::vector<float> dst;

    dst.resize(src.size());

    size_t sz = src.size();

    for(size_t i=0; i<sz; ++i) {

        float v1 = src[i];

        float v2 = src[(i+1) % sz];

        dst[i] = (v1 + v2) * 0.5f;

    }

    return dst;

}



float WaveScanner::getMathWave(int bank, int waveIndex, int sampleIndex, bool uwMode)

{

    float ph = (float)sampleIndex / (float)WAVE_SIZE;

    float mix = (float)waveIndex / (float)(TABLE_FRAMES - 1);

    float val = 0.0f;



    if (uwMode) {

        float modFreq = 1.0f + mix * 4.0f;

        float modulator = std::sin(ph * 6.283f * modFreq);

        val = std::sin(ph * 6.283f + modulator * mix * 2.5f);

    }

    else if (bank == 1) {

        float width = 0.5f + (mix * 0.45f);

        val = (ph < width) ? 1.0f : -1.0f;

    }

    else if (bank == 2) {

        float freq = 1.0f + (mix * 12.0f);

        float p = ph * freq;

        p = p - std::floor(p);

        float win = 1.0f - std::pow(ph, 3.0f);

        val = (1.0f - 2.0f * p) * win;

    }

    else if (bank == 3) {

        float f1 = std::sin(ph * 6.283f * (1.0f + mix));

        float f2 = std::sin(ph * 6.283f * (4.0f + mix*2.0f)) * (1.0f - mix);

        val = std::tanh(f1 + f2);

    }

    return val;

}



// =============================================================

// WTSynth Implementation

// =============================================================



WTSynth::WTSynth(std::shared_ptr<const WavetableMipMap> map, NotePlayHandle* _nph, bool _vintageMode,

                 const sample_rate_t _sample_rate,

                 float _subTune, float _subMix,

                 float _cutoff, float _emphasis,

                 float _basis, float _stereoWidth,

                 int _uniVoices, float _uniDetune, float _uniSpread,

                 float _envA, float _envD, float _envS, float _envR,

                 float _modEnvWave, float _modEnvVcf,

                 float _modLfoWave, float _modLfoPitch, float _lfoSpeed) :

    m_map(map), m_nph(_nph), m_vintageMode(_vintageMode), m_sampleRate(_sample_rate),

    m_subMix(_subMix), m_fCutoff(_cutoff), m_fResonance(_emphasis),

    m_lfoSpeed(_lfoSpeed),

    m_basis(_basis), m_stereoWidth(_stereoWidth),

    m_envA(_envA), m_envD(_envD), m_envS(_envS), m_envR(_envR),

    m_modEnvWave(_modEnvWave), m_modEnvVcf(_modEnvVcf),

    m_modLfoWave(_modLfoWave), m_modLfoPitch(_modLfoPitch)

{

    m_subStepFactor = std::pow(2.0f, _subTune);

    m_subPhase = 0.0f;



    int voices = std::clamp(_uniVoices, 1, 9);

    m_voices.resize(voices);



    for(int i=0; i<voices; ++i) {

        VoiceState& v = m_voices[i];

        // Note: Using rand() in plugins is generally discouraged, but retained here for consistency with original

        v.phase = (float)rand() / (float)RAND_MAX; 



        if (voices == 1) {

            v.speedScale = 1.0f;

            v.panL = 0.707f; v.panR = 0.707f;

        } else {

            float spread = (float)i / (float)(voices - 1);

            float detuneBias = (spread * 2.0f) - 1.0f;

            float detuneAmt = detuneBias * _uniDetune * 0.15f;

            v.speedScale = std::pow(2.0f, detuneAmt);

            

            float panPos = detuneBias * _uniSpread;

            v.panL = std::cos((panPos + 1.0f) * 0.785398f);

            v.panR = std::sin((panPos + 1.0f) * 0.785398f);

        }

    }



    for(int i=0; i<4; ++i) { m_filterStateL[i] = 0.0f; m_filterStateR[i] = 0.0f; }

    m_lfoPhase = 0.0f;

    m_adsr.trigger();

}



inline float readWavetable(const WavetableMipMap* map, float phase, float position, float pitchHz, float sampleRate, bool vintage) {

    int mipLevel = 0;

    if (!vintage) {

        float cyclesPerSample = pitchHz / sampleRate;

        float index = std::log2(cyclesPerSample * WAVE_SIZE);

        mipLevel = (int)std::max(0.0f, index);

        if (mipLevel >= MIP_LEVELS) mipLevel = MIP_LEVELS - 1;

    }



    float safe_pos = std::clamp(position, 0.0f, 1.0f);

    float table_idx_float = safe_pos * (TABLE_FRAMES - 1);

    int idx_A = (int)table_idx_float;

    int idx_B = (idx_A < TABLE_FRAMES - 1) ? idx_A + 1 : idx_A;

    float fracY = table_idx_float - idx_A;



    float sample_idx_float = phase * WAVE_SIZE;

    int s_idx = (int)sample_idx_float;



    const auto& frameA = map->levels[mipLevel][idx_A];

    const auto& frameB = map->levels[mipLevel][idx_B];



    int s0 = s_idx % WAVE_SIZE;

    

    // CHANGED: Vintage Logic

    // If Vintage is ON, we perform Nearest Neighbor interpolation on the Sample (X) axis

    // This preserves the "stepped" digital quality of early wavetables.

    if (vintage) {

        float valA = frameA[s0]; 

        float valB = frameB[s0];

        // We still interpolate between Frames (Y) because the PPG did that.

        return valA * (1.0f - fracY) + valB * fracY;

    } 

    else {

        // Modern Linear Interpolation

        int s1 = (s0 + 1) % WAVE_SIZE;

        float fracX = sample_idx_float - s_idx;



        float valA = frameA[s0] * (1.0f - fracX) + frameA[s1] * fracX;

        float valB = frameB[s0] * (1.0f - fracX) + frameB[s1] * fracX;

        return valA * (1.0f - fracY) + valB * fracY;

    }

}



void WTSynth::nextSample(float position_modulation, float pressure, float* outL, float* outR)

{

    float envVal = m_adsr.process(m_envA, m_envD, m_envS, m_envR, m_sampleRate);



    m_lfoPhase += (m_lfoSpeed / m_sampleRate);

    if (m_lfoPhase >= 1.0f) m_lfoPhase -= 1.0f;

    float lfo = std::sin(m_lfoPhase * 6.283f);



    float modPos = (position_modulation / 63.0f) + (envVal * m_modEnvWave) + (lfo * m_modLfoWave * 0.1f);

    modPos = std::clamp(modPos, 0.0f, 1.0f);



    float modCutoff = std::clamp(m_fCutoff + (envVal * m_modEnvVcf), 0.001f, 0.99f);



    float baseFreq = m_nph->frequency();

    float baseStep = baseFreq / m_sampleRate;

    float vibrato = 1.0f + (lfo * m_modLfoPitch * 0.01f);



    float sumL = 0.0f;

    float sumR = 0.0f;



    // CHANGED: Use m_map.get() to access the shared pointer raw pointer

    const WavetableMipMap* rawMap = m_map.get();



    for(auto& v : m_voices) {

        float step = baseStep * v.speedScale * vibrato;

        v.phase += step;

        if(v.phase >= 1.0f) v.phase -= 1.0f;



        float voicePitch = baseFreq * v.speedScale;

        float sample = readWavetable(rawMap, v.phase, modPos, voicePitch, m_sampleRate, m_vintageMode);

        

        sumL += sample * v.panL;

        sumR += sample * v.panR;

    }



    float norm = 1.0f / std::sqrt((float)m_voices.size());

    sumL *= norm;

    sumR *= norm;



    if (m_subMix > 0.0f) {

        float step = baseStep * m_subStepFactor;

        m_subPhase += step; if (m_subPhase >= 1.0f) m_subPhase -= 1.0f;

        float subSig = (m_subPhase < 0.5f ? 1.0f : -1.0f) * m_subMix;

        sumL += subSig;

        sumR += subSig;

    }



    float omega = modCutoff * 0.9f;

    float res = m_fResonance * 3.8f;



    // Filter L

    float inL = sumL - res * m_filterStateL[3];

    if (inL > 1.2f) inL = 1.2f; else if (inL < -1.2f) inL = -1.2f;

    m_filterStateL[0] += omega * (inL - m_filterStateL[0]);

    m_filterStateL[1] += omega * (m_filterStateL[0] - m_filterStateL[1]);

    m_filterStateL[2] += omega * (m_filterStateL[1] - m_filterStateL[2]);

    m_filterStateL[3] += omega * (m_filterStateL[2] - m_filterStateL[3]);



    // Filter R

    float inR = sumR - res * m_filterStateR[3];

    if (inR > 1.2f) inR = 1.2f; else if (inR < -1.2f) inR = -1.2f;

    m_filterStateR[0] += omega * (inR - m_filterStateR[0]);

    m_filterStateR[1] += omega * (m_filterStateR[0] - m_filterStateR[1]);

    m_filterStateR[2] += omega * (m_filterStateR[1] - m_filterStateR[2]);

    m_filterStateR[3] += omega * (m_filterStateR[2] - m_filterStateR[3]);



    *outL = m_filterStateL[3] * envVal * m_basis;

    *outR = m_filterStateR[3] * envVal * m_basis;

}



// =============================================================

// WaveScanner Data Handling

// =============================================================



WaveScanner::WaveScanner(InstrumentTrack* t) : Instrument(t, &wavescanner_plugin_descriptor),

    m_wavePosition(0.0f, 0.0f, 63.0f, 0.1f, this, tr("Pos")),

    m_sampleLength(WAVE_SIZE, 10, WAVE_SIZE, 1, this, tr("Grit")),

    m_bank(0.0f, 0.0f, 3.0f, 1.0f, this, tr("Bank")),

    m_vintageMode(false, this, tr("Vintage")),

    m_uwMode(false, this, tr("FM Mode")),

    m_uniVoices(1.0f, 1.0f, 9.0f, 1.0f, this, tr("Voices")),

    m_uniDetune(0.2f, 0.0f, 1.0f, 0.01f, this, tr("Detune")),

    m_uniSpread(0.8f, 0.0f, 1.0f, 0.01f, this, tr("Spread")),

    m_subMix(0.0f, 0.0f, 1.0f, 0.01f, this, tr("Sub Mix")),

    m_subTune(-1.0f, -2.0f, 1.0f, 1.0f, this, tr("Sub Oct")),

    m_filterCutoff(1.0f, 0.0f, 1.0f, 0.01f, this, tr("Cutoff")),

    m_filterEmphasis(0.0f, 0.0f, 1.0f, 0.01f, this, tr("Res")),

    m_basis(0.8f, 0.0f, 1.0f, 0.01f, this, tr("Gain")),

    m_stereoWidth(1.0f, 0.0f, 1.0f, 0.01f, this, tr("Wide")),

    m_envA(0.0f, 0.0f, 2.0f, 0.01f, this, tr("Att")),

    m_envD(0.4f, 0.0f, 2.0f, 0.01f, this, tr("Dec")),

    m_envS(0.5f, 0.0f, 1.0f, 0.01f, this, tr("Sus")),

    m_envR(0.4f, 0.0f, 3.0f, 0.01f, this, tr("Rel")),

    m_modEnvWave(0.0f, -1.0f, 1.0f, 0.01f, this, tr("Env->W")),

    m_modEnvVcf(0.0f, -1.0f, 1.0f, 0.01f, this, tr("Env->F")),

    m_modLfoWave(0.0f, 0.0f, 1.0f, 0.01f, this, tr("Lfo->W")),

    m_modLfoPitch(0.0f, 0.0f, 1.0f, 0.01f, this, tr("Lfo->P")),

    m_lfoSpeed(5.0f, 0.1f, 20.0f, 0.1f, this, tr("Rate")),

    m_modEnable(true, this, tr("Mods")),

    m_graphStart(-1.0f, 1.0f, WAVE_SIZE, this),

    m_graphEnd(-1.0f, 1.0f, WAVE_SIZE, this),

    m_graphVisual(-1.0f, 1.0f, WAVE_SIZE, this)

{

    // CHANGED: Initialize the shared pointer with an empty map to prevent null access

    m_currentMap = std::make_shared<WavetableMipMap>();



    m_graphStart.setWaveToSaw();

    m_graphEnd.setWaveToSine();



    connect(&m_graphStart, SIGNAL(samplesChanged(int,int)), this, SLOT(updateWavetable()));

    connect(&m_graphEnd, SIGNAL(samplesChanged(int,int)), this, SLOT(updateWavetable()));

    connect(&m_uwMode, SIGNAL(dataChanged()), this, SLOT(updateWavetable()));

    connect(&m_bank, SIGNAL(dataChanged()), this, SLOT(updateWavetable()));

    connect(&m_wavePosition, SIGNAL(dataChanged()), this, SLOT(updateVisuals()));



    updateWavetable();

}



void WaveScanner::updateWavetable() {

    // CHANGED: THREAD SAFETY FIX

    // Create a NEW map completely detached from the one currently being played

    auto newMap = std::make_shared<WavetableMipMap>();

    

    newMap->levels.resize(MIP_LEVELS);

    newMap->levels[0].resize(TABLE_FRAMES);



    int bank = static_cast<int>(m_bank.value());

    bool uw = m_uwMode.value();



    for(int i = 0; i < TABLE_FRAMES; ++i) {

        newMap->levels[0][i].resize(WAVE_SIZE);

        float mix = static_cast<float>(i) / (TABLE_FRAMES - 1);

        for(int s = 0; s < WAVE_SIZE; ++s) {

            if (bank == 0 && !uw) {

                newMap->levels[0][i][s] = std::lerp(m_graphStart.samples()[s], m_graphEnd.samples()[s], mix);

            } else {

                newMap->levels[0][i][s] = getMathWave(bank, i, s, uw);

            }

        }

    }



    for(int L = 1; L < MIP_LEVELS; ++L) {

        newMap->levels[L].resize(TABLE_FRAMES);

        for(int i = 0; i < TABLE_FRAMES; ++i) {

            newMap->levels[L][i] = generateMipLevel(newMap->levels[L-1][i]);

        }

    }



    // ATOMIC SWAP: This is the magic moment. We swap the pointer.

    // Any existing notes keep holding the OLD pointer. New notes get the NEW pointer.

    std::atomic_store(&m_currentMap, newMap);



    updateVisuals();

}



void WaveScanner::updateVisuals() {
    // Safe to read current map here (GUI thread only)
    if (!m_currentMap) return;
    
    int frame = std::clamp((int)m_wavePosition.value(), 0, TABLE_FRAMES-1);
    float buf[WAVE_SIZE];
    
    // Check bounds just in case
    // FIX: Cast 'frame' to size_t to match the vector size type (unsigned long)
    if (m_currentMap->levels.size() > 0 && 
        m_currentMap->levels[0].size() > static_cast<size_t>(frame)) {
            
        for(int i=0; i<WAVE_SIZE; ++i) buf[i] = m_currentMap->levels[0][frame][i];
        m_graphVisual.setSamples(buf);
    }
}



void WaveScanner::playNote(NotePlayHandle* _n, SampleFrame* _working_buffer) {

    if (!_n->m_pluginData) {

        // CHANGED: Pass m_currentMap (the shared_ptr) to the synth

        _n->m_pluginData = new WTSynth(

            m_currentMap, 

            _n, m_vintageMode.value(), Engine::audioEngine()->outputSampleRate(),

            m_subTune.value(), m_subMix.value(),

            m_filterCutoff.value(), m_filterEmphasis.value(),

            m_basis.value(), m_stereoWidth.value(),

            (int)m_uniVoices.value(), m_uniDetune.value(), m_uniSpread.value(),

            m_envA.value(), m_envD.value(), m_envS.value(), m_envR.value(),

            m_modEnvWave.value(), m_modEnvVcf.value(),

            m_modLfoWave.value(), m_modLfoPitch.value(), m_lfoSpeed.value()

        );

    }



    auto synth = static_cast<WTSynth*>(_n->m_pluginData);

    const fpp_t frames = _n->framesLeftForCurrentPeriod();

    const f_cnt_t offset = _n->noteOffset();



    if (_n->isReleased()) synth->release();



    for(fpp_t frame = offset; frame < frames + offset; ++frame) {

        float L, R;

        synth->nextSample(m_wavePosition.value(frame), 0.0f, &L, &R);

        _working_buffer[frame] = SampleFrame(L, R);

    }

    applyRelease(_working_buffer, _n);

}



void WaveScanner::deleteNotePluginData(NotePlayHandle* _n) {

    delete static_cast<WTSynth*>(_n->m_pluginData);

}



void WaveScanner::saveSettings(QDomDocument& d, QDomElement& p) {

    p.setAttribute("version", "4.0");

    m_wavePosition.saveSettings(d, p, "pos");

    m_bank.saveSettings(d, p, "bank");

    m_uwMode.saveSettings(d, p, "uw");

    m_vintageMode.saveSettings(d, p, "vintage");

    m_uniVoices.saveSettings(d, p, "uniV");

    m_uniDetune.saveSettings(d, p, "uniD");

    m_uniSpread.saveSettings(d, p, "uniS");

    m_subMix.saveSettings(d, p, "subM");

    m_subTune.saveSettings(d, p, "subT");

    m_filterCutoff.saveSettings(d, p, "cut");

    m_filterEmphasis.saveSettings(d, p, "res");

    m_basis.saveSettings(d, p, "gain");

    m_envA.saveSettings(d, p, "envA"); m_envD.saveSettings(d, p, "envD");

    m_envS.saveSettings(d, p, "envS"); m_envR.saveSettings(d, p, "envR");



    QString s, e;

    base64::encode((const char*)m_graphStart.samples(), WAVE_SIZE*4, s);

    base64::encode((const char*)m_graphEnd.samples(), WAVE_SIZE*4, e);

    p.setAttribute("g1", s); p.setAttribute("g2", e);

}



void WaveScanner::loadSettings(const QDomElement& p) {

    m_wavePosition.loadSettings(p, "pos");

    m_bank.loadSettings(p, "bank");

    m_uwMode.loadSettings(p, "uw");

    m_vintageMode.loadSettings(p, "vintage");

    m_uniVoices.loadSettings(p, "uniV");

    m_uniDetune.loadSettings(p, "uniD");

    m_uniSpread.loadSettings(p, "uniS");

    m_subMix.loadSettings(p, "subM");

    m_subTune.loadSettings(p, "subT");

    m_filterCutoff.loadSettings(p, "cut");

    m_filterEmphasis.loadSettings(p, "res");

    m_basis.loadSettings(p, "gain");

    m_envA.loadSettings(p, "envA"); m_envD.loadSettings(p, "envD");

    m_envS.loadSettings(p, "envS"); m_envR.loadSettings(p, "envR");



    int sz=0; char* dst=nullptr;

    base64::decode(p.attribute("g1"), &dst, &sz); if(dst) { m_graphStart.setSamples((float*)dst); delete[] dst; }

    base64::decode(p.attribute("g2"), &dst, &sz); if(dst) { m_graphEnd.setSamples((float*)dst); delete[] dst; }



    updateWavetable();

}



QString WaveScanner::nodeName() const { return wavescanner_plugin_descriptor.name; }

gui::PluginView* WaveScanner::instantiateView(QWidget* p) { return new gui::WaveScannerView(this, p); }



// =============================================================

// CUSTOM GUI COMPONENTS

// =============================================================



namespace gui

{



SevenSegmentDisplay::SevenSegmentDisplay(QWidget* parent) : QWidget(parent), m_value(0) { setFixedSize(50, 24); }

void SevenSegmentDisplay::setValue(int v) { if (m_value != v) { m_value = v; update(); } }

void SevenSegmentDisplay::paintEvent(QPaintEvent*) {

    QPainter p(this); p.fillRect(rect(), QColor(10, 10, 10));

    p.setFont(QFont("Courier", 16, QFont::Bold));

    p.setPen(QColor(60, 60, 60)); p.drawText(rect(), Qt::AlignCenter, "88");

    p.setPen(QColor(255, 60, 60, 100)); p.drawText(rect().adjusted(-1,-1,1,1), Qt::AlignCenter, QString::number(m_value).rightJustified(2, '0'));

    p.setPen(QColor(255, 60, 60)); p.drawText(rect(), Qt::AlignCenter, QString::number(m_value).rightJustified(2, '0'));

}



// Renamed from PPGKnob to ScannerKnob

ScannerKnob::ScannerKnob(QWidget* parent, const QString& label) : Knob(KnobType::Styled, parent), m_textLabel(label) {

    setFixedSize(40, 50);

    setLabel(label);

    setCenterPointX(20); setCenterPointY(20);

    setInnerRadius(6);   setOuterRadius(14);

    setTotalAngle(270.0); setLineWidth(4);

    QPalette p = palette();

    p.setColor(QPalette::Button, QColor(30, 30, 30));

    p.setColor(QPalette::Mid, QColor(60, 60, 60));

    p.setColor(QPalette::ButtonText, QColor(255, 255, 255));

    p.setColor(QPalette::Highlight, QColor(0, 200, 255));

    setPalette(p);

}



void ScannerKnob::paintEvent(QPaintEvent* event)

{

    QPainter p(this);

    p.setRenderHint(QPainter::Antialiasing);



    int d = 34; // Diameter

    int x = (width() - d) / 2;

    int y = 2;

    QRectF knobRect(x, y, d, d);



    int startAngle = 225 * 16;

    int spanAngle = -270 * 16;



    p.setPen(QPen(palette().color(QPalette::Mid), 4, Qt::SolidLine, Qt::RoundCap));

    p.setBrush(Qt::NoBrush);

    p.drawArc(knobRect, startAngle, spanAngle);



    auto m = castModel<FloatModel>();

    float v = 0.0f;

    if (m) {

        float val = m->value();

        float min = m->minValue();

        float max = m->maxValue();

        if (max > min) {

            v = (val - min) / (max - min);

        }

    }

    v = std::clamp(v, 0.0f, 1.0f);

    int valSpan = (int)(-270.0f * v * 16.0f);



    p.setPen(QPen(palette().color(QPalette::Highlight), 4, Qt::SolidLine, Qt::RoundCap));

    if (std::abs(valSpan) > 0) {

        p.drawArc(knobRect, startAngle, valSpan);

    }



    p.setPen(palette().color(QPalette::ButtonText));

    p.setFont(QFont("Arial", 7));

    p.drawText(rect().adjusted(0,0,0,-2), Qt::AlignBottom | Qt::AlignHCenter, m_textLabel);

}



// 3. The Main View

WaveScannerView::WaveScannerView(Instrument* i, QWidget* p) : InstrumentViewFixedSize(i, p), m_currentTab(0) {

    setFixedSize(250, 260);



    setAutoFillBackground( true );

    QPalette pal;

    setPalette( pal );



    QString ts = "QPushButton { background: #333; color: #AAA; border: 1px solid #222; } QPushButton:checked { background: #555; color: #FFF; border-bottom: 2px solid #0AF; }";

    m_btnTabDigital = new QPushButton("WAVE", this); m_btnTabDigital->setGeometry(5,5,78,20);

    m_btnTabDigital->setCheckable(true); m_btnTabDigital->setChecked(true); m_btnTabDigital->setStyleSheet(ts);

    connect(m_btnTabDigital, SIGNAL(clicked()), this, SLOT(showPageDigital()));



    m_btnTabUnison = new QPushButton("UNISON", this); m_btnTabUnison->setGeometry(85,5,78,20);

    m_btnTabUnison->setCheckable(true); m_btnTabUnison->setStyleSheet(ts);

    connect(m_btnTabUnison, SIGNAL(clicked()), this, SLOT(showPageUnison()));



    m_btnTabMods = new QPushButton("MODS", this); m_btnTabMods->setGeometry(165,5,78,20);

    m_btnTabMods->setCheckable(true); m_btnTabMods->setStyleSheet(ts);

    connect(m_btnTabMods, SIGNAL(clicked()), this, SLOT(showPageMods()));



    QString subTs = "QPushButton { background: #444; color: #BBB; border: none; font-size: 10px; } QPushButton:checked { color: #0AF; font-weight: bold; }";

    m_btnShowStart = new QPushButton("Start Wave", this); m_btnShowStart->setGeometry(10,35,115,20);

    m_btnShowStart->setCheckable(true); m_btnShowStart->setChecked(true); m_btnShowStart->setStyleSheet(subTs);

    connect(m_btnShowStart, SIGNAL(clicked()), this, SLOT(showGraphStart()));



    m_btnShowEnd = new QPushButton("End Wave", this); m_btnShowEnd->setGeometry(125,35,115,20);

    m_btnShowEnd->setCheckable(true); m_btnShowEnd->setStyleSheet(subTs);

    connect(m_btnShowEnd, SIGNAL(clicked()), this, SLOT(showGraphEnd()));



    m_viewGraph = new Graph(this, Graph::Style::Nearest, 230, 80); m_viewGraph->move(10, 60);

    m_viewGraph->setGraphColor(QColor(0, 200, 255));

    QPalette gp = m_viewGraph->palette(); gp.setColor(m_viewGraph->backgroundRole(), QColor(10, 10, 15)); m_viewGraph->setPalette(gp);



    m_vintageToggle = new LedCheckBox("Raw", this, tr("Vintage Step"), LedCheckBox::LedColor::Red);

    m_vintageToggle->move(15, 145);

    

    m_uwToggle = new LedCheckBox("FM", this, tr("FM Morph"), LedCheckBox::LedColor::Green);

    m_uwToggle->move(15, 170);



    m_positionKnob = new ScannerKnob(this, "Scan");

    m_positionKnob->move(105, 145);



    m_ledDisplay = new SevenSegmentDisplay(this);

    m_ledDisplay->move(180, 150);



    int rowY = 200;

    m_resKnob = new ScannerKnob(this, "Grit"); m_resKnob->move(110, rowY);

    m_bankKnob = new ScannerKnob(this, "Bank"); m_bankKnob->move(180, rowY);



    m_uniVoicesKnob = new ScannerKnob(this, "Voices"); m_uniVoicesKnob->move(20, 50);

    m_uniDetuneKnob = new ScannerKnob(this, "Detune"); m_uniDetuneKnob->move(80, 50);

    m_uniSpreadKnob = new ScannerKnob(this, "Spread"); m_uniSpreadKnob->move(140, 50);

    m_subMixKnob = new ScannerKnob(this, "SubVol"); m_subMixKnob->move(200, 50);

    m_subTuneKnob = new ScannerKnob(this, "SubOct"); m_subTuneKnob->move(200, 110);



    int row2Y = 140;

    m_vcfCutoffKnob = new ScannerKnob(this, "Cutoff"); m_vcfCutoffKnob->move(20, row2Y);

    m_vcfEmphasisKnob = new ScannerKnob(this, "Res"); m_vcfEmphasisKnob->move(80, row2Y);

    m_basisKnob = new ScannerKnob(this, "Vol"); m_basisKnob->move(140, row2Y);



    m_modSwitch = new LedCheckBox("On", this, tr("Internal Mods"), LedCheckBox::LedColor::Yellow);

    m_modSwitch->move(15, 35);



    m_envAKnob = new ScannerKnob(this, "A"); m_envAKnob->move(30, 60);

    m_envDKnob = new ScannerKnob(this, "D"); m_envDKnob->move(80, 60);

    m_envSKnob = new ScannerKnob(this, "S"); m_envSKnob->move(130, 60);

    m_envRKnob = new ScannerKnob(this, "R"); m_envRKnob->move(180, 60);



    int row3Y = 140;

    m_modEnvWaveKnob = new ScannerKnob(this, "Env->W"); m_modEnvWaveKnob->move(10, row3Y);

    m_modEnvVcfKnob = new ScannerKnob(this, "Env->F"); m_modEnvVcfKnob->move(60, row3Y);

    m_lfoSpeedKnob = new ScannerKnob(this, "Rate"); m_lfoSpeedKnob->move(130, row3Y);

    m_modLfoWaveKnob = new ScannerKnob(this, "Lfo->W"); m_modLfoWaveKnob->move(180, row3Y);

    m_modLfoPitchKnob = new ScannerKnob(this, "Lfo->P"); m_modLfoPitchKnob->move(180, row3Y + 50);



    updateVisibleWidgets();

}



void WaveScannerView::showPageDigital() { m_currentTab = 0; m_btnTabDigital->setChecked(true); m_btnTabUnison->setChecked(false); m_btnTabMods->setChecked(false); updateVisibleWidgets(); }

void WaveScannerView::showPageUnison() { m_currentTab = 1; m_btnTabDigital->setChecked(false); m_btnTabUnison->setChecked(true); m_btnTabMods->setChecked(false); updateVisibleWidgets(); }

void WaveScannerView::showPageMods() { m_currentTab = 2; m_btnTabDigital->setChecked(false); m_btnTabUnison->setChecked(false); m_btnTabMods->setChecked(true); updateVisibleWidgets(); }



void WaveScannerView::updateVisibleWidgets() {

    bool d = (m_currentTab == 0);

    bool u = (m_currentTab == 1);

    bool m = (m_currentTab == 2);

    auto p = castModel<WaveScanner>();



    int bank = static_cast<int>(p->m_bank.value());

    bool isBank0 = (bank == 0 && !p->m_uwMode.value());



    m_btnShowStart->setVisible(d && isBank0);

    m_btnShowEnd->setVisible(d && isBank0);

    m_viewGraph->setVisible(d);

    

    m_positionKnob->setVisible(d); m_ledDisplay->setVisible(d);

    m_vintageToggle->setVisible(d); m_uwToggle->setVisible(d);

    m_resKnob->setVisible(d); m_bankKnob->setVisible(d);



    m_uniVoicesKnob->setVisible(u); m_uniDetuneKnob->setVisible(u); m_uniSpreadKnob->setVisible(u);

    m_subMixKnob->setVisible(u); m_subTuneKnob->setVisible(u);

    m_vcfCutoffKnob->setVisible(u); m_vcfEmphasisKnob->setVisible(u); m_basisKnob->setVisible(u);



    m_modSwitch->setVisible(m);

    m_envAKnob->setVisible(m); m_envDKnob->setVisible(m);

    m_envSKnob->setVisible(m); m_envRKnob->setVisible(m);

    m_modEnvWaveKnob->setVisible(m); m_modEnvVcfKnob->setVisible(m);

    m_lfoSpeedKnob->setVisible(m); m_modLfoWaveKnob->setVisible(m); m_modLfoPitchKnob->setVisible(m);

}



void WaveScannerView::showGraphStart() { auto p=castModel<WaveScanner>(); m_viewGraph->setModel(&p->m_graphStart); m_btnShowStart->setChecked(true); m_btnShowEnd->setChecked(false); }

void WaveScannerView::showGraphEnd() { auto p=castModel<WaveScanner>(); m_viewGraph->setModel(&p->m_graphEnd); m_btnShowStart->setChecked(false); m_btnShowEnd->setChecked(true); }



void WaveScannerView::modelChanged() {

    auto p = castModel<WaveScanner>();

    if (m_currentTab == 0) {

        int bank = static_cast<int>(p->m_bank.value());

        bool isBank0 = (bank == 0 && !p->m_uwMode.value());

        if (!isBank0) m_viewGraph->setModel(&p->m_graphVisual);

        else if (m_btnShowEnd->isChecked()) m_viewGraph->setModel(&p->m_graphEnd);

        else m_viewGraph->setModel(&p->m_graphStart);

    }

    updateVisibleWidgets();



    m_positionKnob->setModel(&p->m_wavePosition); m_resKnob->setModel(&p->m_sampleLength);

    m_vintageToggle->setModel(&p->m_vintageMode); m_uwToggle->setModel(&p->m_uwMode);

    m_bankKnob->setModel(&p->m_bank);



    m_uniVoicesKnob->setModel(&p->m_uniVoices); m_uniDetuneKnob->setModel(&p->m_uniDetune); m_uniSpreadKnob->setModel(&p->m_uniSpread);

    m_subMixKnob->setModel(&p->m_subMix); m_subTuneKnob->setModel(&p->m_subTune);

    m_vcfCutoffKnob->setModel(&p->m_filterCutoff); m_vcfEmphasisKnob->setModel(&p->m_filterEmphasis);

    m_basisKnob->setModel(&p->m_basis);



    m_modSwitch->setModel(&p->m_modEnable);

    m_envAKnob->setModel(&p->m_envA); m_envDKnob->setModel(&p->m_envD);

    m_envSKnob->setModel(&p->m_envS); m_envRKnob->setModel(&p->m_envR);

    m_modEnvWaveKnob->setModel(&p->m_modEnvWave); m_modEnvVcfKnob->setModel(&p->m_modEnvVcf);

    m_modLfoWaveKnob->setModel(&p->m_modLfoWave); m_modLfoPitchKnob->setModel(&p->m_modLfoPitch);

    m_lfoSpeedKnob->setModel(&p->m_lfoSpeed);



    connect(&p->m_wavePosition, &FloatModel::dataChanged, [=, this](){ m_ledDisplay->setValue((int)(p->m_wavePosition.value())); });

    connect(&p->m_bank, &FloatModel::dataChanged, [=, this](){ this->modelChanged(); });

    connect(&p->m_uwMode, &BoolModel::dataChanged, [=, this](){ this->modelChanged(); });



    m_ledDisplay->setValue((int)(p->m_wavePosition.value()));

}



} // namespace gui



extern "C" {

    PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* m, void*)

    {

        return new WaveScanner(static_cast<InstrumentTrack*>(m));

    }

}



} // namespace lmms
