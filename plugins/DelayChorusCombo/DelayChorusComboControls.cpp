#include "DelayChorusComboControls.h"
#include "DelayChorusCombo.h"

namespace lmms {

DelayChorusComboControls::DelayChorusComboControls(DelayChorusComboEffect* effect)
    : EffectControls(effect),
      m_delayModel(15.0f, 0.1f, 30.0f, 0.1f, this, tr("Delay")),
      m_depthModel(2.0f, 0.0f, 10.0f, 0.1f, this, tr("Depth")),
      m_rateModel(0.5f, 0.01f, 10.0f, 0.01f, this, tr("Rate")),
      m_feedbackModel(10.0f, -99.0f, 99.0f, 1.0f, this, tr("Feed")),
      m_stereoModel(90.0f, 0.0f, 180.0f, 1.0f, this, tr("Phase")),
      m_mixModel(50.0f, 0.0f, 100.0f, 1.0f, this, tr("Mix")),
      
      // NEW KNOBS
      m_dampModel(0.0f, 0.0f, 100.0f, 1.0f, this, tr("Damp")), // 0% = Bright, 100% = Dark
      m_shapeModel(0.0f, 0.0f, 1.0f, 1.0f, this, tr("Shape")), // 0=Sine, 1=Triangle
      m_crossModel(0.0f, 0.0f, 100.0f, 1.0f, this, tr("Cross")), // Cross-feedback amount
      
      m_effect(effect) {
}

void DelayChorusComboControls::saveSettings(QDomDocument& doc, QDomElement& parent) {
    m_delayModel.saveSettings(doc, parent, "delay");
    m_depthModel.saveSettings(doc, parent, "depth");
    m_rateModel.saveSettings(doc, parent, "rate");
    m_feedbackModel.saveSettings(doc, parent, "feedback");
    m_stereoModel.saveSettings(doc, parent, "stereo");
    m_mixModel.saveSettings(doc, parent, "mix");
    // Save new
    m_dampModel.saveSettings(doc, parent, "damp");
    m_shapeModel.saveSettings(doc, parent, "shape");
    m_crossModel.saveSettings(doc, parent, "cross");
}

void DelayChorusComboControls::loadSettings(const QDomElement& parent) {
    m_delayModel.loadSettings(parent, "delay");
    m_depthModel.loadSettings(parent, "depth");
    m_rateModel.loadSettings(parent, "rate");
    m_feedbackModel.loadSettings(parent, "feedback");
    m_stereoModel.loadSettings(parent, "stereo");
    m_mixModel.loadSettings(parent, "mix");
    // Load new
    m_dampModel.loadSettings(parent, "damp");
    m_shapeModel.loadSettings(parent, "shape");
    m_crossModel.loadSettings(parent, "cross");
}

} // namespace lmms