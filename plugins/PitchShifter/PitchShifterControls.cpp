#include "PitchShifterControls.h"
#include "PitchShifter.h"
#include <QDomElement>

namespace lmms {

PitchShifterControls::PitchShifterControls(PitchShifterEffect* effect)
    : EffectControls(effect),
      m_semitoneModel(0.0f, -12.0f, 12.0f, 1.0f, this, tr("Pitch")),
      m_centModel(0.0f, -100.0f, 100.0f, 1.0f, this, tr("Fine")),
      m_grainModel(50.0f, 10.0f, 200.0f, 1.0f, this, tr("Grain Size")), // 50ms is standard
      m_mixModel(100.0f, 0.0f, 100.0f, 1.0f, this, tr("Mix")),
      m_effect(effect) {
}

void PitchShifterControls::saveSettings(QDomDocument& doc, QDomElement& parent) {
    m_semitoneModel.saveSettings(doc, parent, "semitones");
    m_centModel.saveSettings(doc, parent, "cents");
    m_grainModel.saveSettings(doc, parent, "grain");
    m_mixModel.saveSettings(doc, parent, "mix");
}

void PitchShifterControls::loadSettings(const QDomElement& parent) {
    m_semitoneModel.loadSettings(parent, "semitones");
    m_centModel.loadSettings(parent, "cents");
    m_grainModel.loadSettings(parent, "grain");
    m_mixModel.loadSettings(parent, "mix");
}

} // namespace lmms