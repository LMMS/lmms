/*
 * TransferFunctionControls.cpp - controls for TransferFunction effect
 */
#include "TransferFunctionControls.h"
#include "TransferFunction.h"
#include "TransferFunctionLogic.h"
// NEW INCLUDES FOR SAMPLE RATE
#include "Engine.h"
#include "AudioEngine.h"
#include <QDomElement>
#include <QDomDocument>
#include <vector>
#include <cmath>
#include <string>
#include <cctype>
#include <algorithm>
namespace lmms
{
// =======================
//   CONSTRUCTOR
// =======================
TransferFunctionControls::TransferFunctionControls(TransferFunctionEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_volumeModel(1.0f, 1.0f, 18.0f, 1.0f, this, tr("Preset")),
	m_formulaString("1.0 / (1.0 + 1j * (freq / 800.0))"), 
	m_bodeMagModel(0.0f, 1.0f, 512, this),
	m_bodePhaseModel(0.0f, 1.0f, 512, this)
{
    // Init with flat line
    m_drawPoints.push_back({0.0f, 0.5f});
    m_drawPoints.push_back({22050.0f, 0.5f});
    
	connect(&m_volumeModel, SIGNAL(dataChanged()), this, SLOT(updateBodePlot()));
	updateBodePlot();
}
// =======================
//   LOAD / SAVE
// =======================
void TransferFunctionControls::loadSettings(const QDomElement& parent)
{
	m_volumeModel.loadSettings(parent, "volume");
	if (parent.hasAttribute("formula"))
	{
		m_formulaString = parent.attribute("formula");
	}
    
    QDomNode node = parent.firstChild();
    m_drawPoints.clear();
    bool foundPoints = false;
    
    while (!node.isNull())
    {
        QDomElement el = node.toElement();
        if (!el.isNull() && el.tagName() == "point")
        {
            foundPoints = true;
            float f = el.attribute("f").toFloat();
            float m = el.attribute("m").toFloat();
            m_drawPoints.push_back({f, m});
        }
        node = node.nextSibling();
    }
    
    if(foundPoints) {
        std::sort(m_drawPoints.begin(), m_drawPoints.end());
    } else {
        m_drawPoints.push_back({0.0f, 0.5f});
        m_drawPoints.push_back({22050.0f, 0.5f});
    }
	updateBodePlot();
}
void TransferFunctionControls::saveSettings(QDomDocument& doc, QDomElement& parent)
{
	m_volumeModel.saveSettings(doc, parent, "volume");
	parent.setAttribute("formula", m_formulaString);
    
    if (!m_drawPoints.empty()) {
        for(const auto& p : m_drawPoints) {
            QDomElement pt = doc.createElement("point");
            pt.setAttribute("f", p.freq);
            pt.setAttribute("m", p.mag);
            parent.appendChild(pt);
        }
    }
}
// =======================
//   DATA HANDLING
// =======================
void TransferFunctionControls::setDrawPoint(float freq, float mag)
{
    m_drawPoints.push_back({freq, mag});
    std::sort(m_drawPoints.begin(), m_drawPoints.end());
    updateBodePlot();
}
// NEW: Clear Logic
void TransferFunctionControls::clearDrawing()
{
    m_drawPoints.clear();
    // Reset to flat line (Unity Gain)
    m_drawPoints.push_back({0.0f, 0.5f});
    m_drawPoints.push_back({22050.0f, 0.5f});
    updateBodePlot();
}
// =======================
//   BODE PLOT GENERATION
// =======================
void TransferFunctionControls::updateBodePlot()
{
	const int N = 512;
    
    float fs = 44100.0f;
    if (Engine::audioEngine()) {
        fs = Engine::audioEngine()->baseSampleRate();
    }
    
	const float minFreq = 20.0f;        
	const float maxFreq = fs * 0.5f;    
	int presetIdx = static_cast<int>(m_volumeModel.value());
	if (presetIdx < 1) presetIdx = 1;
	if (presetIdx > 18) presetIdx = 18;
	std::vector<float> mag(N);
	std::vector<float> phase(N);
	SimpleParser parser;
	const float PI = 3.14159265358979f; 
	for (int i = 0; i < N; ++i)
	{
		float percent = static_cast<float>(i) / static_cast<float>(N - 1);
		float freq = minFreq * std::pow(maxFreq / minFreq, percent);
		Complex H = calculateTransferFunction(presetIdx, freq, m_formulaString.toStdString(), parser, &m_drawPoints);
		float m = std::abs(H);
		float p = std::arg(H);
		float db = 20.0f * std::log10(m + 1e-12f);
		float magNorm = (db + 40.0f) / 80.0f;
		if (magNorm < 0.0f) magNorm = 0.0f;
		if (magNorm > 1.0f) magNorm = 1.0f;
		mag[i] = magNorm;
		float phaseNorm = (p + PI) / (2.0f * PI);
		if (phaseNorm < 0.0f) phaseNorm = 0.0f;
		if (phaseNorm > 1.0f) phaseNorm = 1.0f;
		phase[i] = phaseNorm;
	}
	m_bodeMagModel.setSamples(mag.data());
	m_bodePhaseModel.setSamples(phase.data());
}
// =======================
//   SLOTS
// =======================
void TransferFunctionControls::setFormula(const QString& text)
{
	if (m_formulaString != text)
	{
		m_formulaString = text;
		updateBodePlot();
	}
}
} // namespace lmms
