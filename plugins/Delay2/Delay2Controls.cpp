#include "Delay2Controls.h"
#include "Delay2.h"
#include <QDomElement>

namespace lmms {

Delay2Controls::Delay2Controls(Delay2Effect* effect)
    : EffectControls(effect),
      // Input
      m_inputVolModel(100.0f, 0.0f, 200.0f, 1.0f, this, tr("Input Vol")),
      
      // Feedback
      m_feedbackModel(40.0f, 0.0f, 110.0f, 1.0f, this, tr("Vol")), // Up to 110% for self-oscillation
      m_cutoffModel(20000.0f, 100.0f, 20000.0f, 10.0f, this, tr("Cutoff")), // Hz
      m_pingPongModel(0.0f, 0.0f, 1.0f, 1.0f, this, tr("PingPong")), // Switch
      
      // Time
      m_timeModel(375.0f, 1.0f, 2000.0f, 1.0f, this, tr("Time")), // Default ~3/16ths at 120bpm
      m_offsetModel(0.0f, -500.0f, 500.0f, 1.0f, this, tr("Stereo")), // FL Style Offset
      
      // Mix
      m_dryModel(100.0f, 0.0f, 100.0f, 1.0f, this, tr("Dry")),
      m_wetModel(80.0f, 0.0f, 100.0f, 1.0f, this, tr("Wet")),
      
      m_effect(effect) {
}

void Delay2Controls::saveSettings(QDomDocument& doc, QDomElement& parent) {
    m_inputVolModel.saveSettings(doc, parent, "input_vol");
    m_feedbackModel.saveSettings(doc, parent, "feedback");
    m_cutoffModel.saveSettings(doc, parent, "cutoff");
    m_pingPongModel.saveSettings(doc, parent, "pingpong");
    m_timeModel.saveSettings(doc, parent, "time");
    m_offsetModel.saveSettings(doc, parent, "offset");
    m_dryModel.saveSettings(doc, parent, "dry");
    m_wetModel.saveSettings(doc, parent, "wet");
}

void Delay2Controls::loadSettings(const QDomElement& parent) {
    m_inputVolModel.loadSettings(parent, "input_vol");
    m_feedbackModel.loadSettings(parent, "feedback");
    m_cutoffModel.loadSettings(parent, "cutoff");
    m_pingPongModel.loadSettings(parent, "pingpong");
    m_timeModel.loadSettings(parent, "time");
    m_offsetModel.loadSettings(parent, "offset");
    m_dryModel.loadSettings(parent, "dry");
    m_wetModel.loadSettings(parent, "wet");
}

} // namespace lmms