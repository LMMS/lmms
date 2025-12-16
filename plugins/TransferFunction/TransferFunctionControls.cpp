/* * TransferFunctionControls.cpp */
#include "TransferFunctionControls.h"
#include "TransferFunction.h"
#include "TransferFunctionLogic.h"
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
using Complex = std::complex<float>;

TransferFunctionControls::TransferFunctionControls(TransferFunctionEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_volumeModel(1.0f, 1.0f, 20.0f, 1.0f, this, tr("Preset")),
	m_formulaString("1.0 / (1.0 + 1j * (freq / 800.0))"), 
	m_bodeMagModel(0.0f, 1.0f, 512, this),
	m_bodePhaseModel(0.0f, 1.0f, 512, this)
{
    m_drawPoints.push_back({0.0f, 0.5f}); m_drawPoints.push_back({22050.0f, 0.5f});
    m_poles.push_back(Complex(0.9f, 0.0f)); m_zeros.push_back(Complex(-1.0f, 0.0f));
	connect(&m_volumeModel, SIGNAL(dataChanged()), this, SLOT(updateBodePlot())); updateBodePlot();
}

void TransferFunctionControls::loadSettings(const QDomElement& parent)
{
	m_volumeModel.loadSettings(parent, "volume"); if (parent.hasAttribute("formula")) { m_formulaString = parent.attribute("formula"); }
    QDomNode node = parent.firstChild(); m_drawPoints.clear(); m_poles.clear(); m_zeros.clear(); bool foundPoints = false;
    while (!node.isNull()) {
        QDomElement el = node.toElement();
        if (!el.isNull()) {
            if(el.tagName() == "point") { foundPoints = true; float f = el.attribute("f").toFloat(); float m = el.attribute("m").toFloat(); m_drawPoints.push_back({f, m}); }
            else if(el.tagName() == "pole") { float r = el.attribute("re").toFloat(); float i = el.attribute("im").toFloat(); m_poles.push_back(Complex(r, i)); }
            else if(el.tagName() == "zero") { float r = el.attribute("re").toFloat(); float i = el.attribute("im").toFloat(); m_zeros.push_back(Complex(r, i)); }
        }

        node = node.nextSibling();
    }

    if(!foundPoints) { m_drawPoints.push_back({0.0f, 0.5f}); m_drawPoints.push_back({22050.0f, 0.5f}); }
	updateBodePlot();
}
void TransferFunctionControls::saveSettings(QDomDocument& doc, QDomElement& parent)
{
	m_volumeModel.saveSettings(doc, parent, "volume"); parent.setAttribute("formula", m_formulaString);
    for(const auto& p : m_drawPoints) { QDomElement pt = doc.createElement("point"); pt.setAttribute("f", p.freq); pt.setAttribute("m", p.mag); parent.appendChild(pt); }
    for(const auto& p : m_poles) { QDomElement pt = doc.createElement("pole"); pt.setAttribute("re", p.real()); pt.setAttribute("im", p.imag()); parent.appendChild(pt); }
    for(const auto& z : m_zeros) { QDomElement pt = doc.createElement("zero"); pt.setAttribute("re", z.real()); pt.setAttribute("im", z.imag()); parent.appendChild(pt); }
}

void TransferFunctionControls::setDrawPoint(float freq, float mag) { m_drawPoints.push_back({freq, mag}); std::sort(m_drawPoints.begin(), m_drawPoints.end()); updateBodePlot(); }
void TransferFunctionControls::clearDrawing() { m_drawPoints.clear(); m_drawPoints.push_back({0.0f, 0.5f}); m_drawPoints.push_back({22050.0f, 0.5f}); updateBodePlot(); }
void TransferFunctionControls::addPole(Complex p) { m_poles.push_back(p); updateBodePlot(); }
void TransferFunctionControls::addZero(Complex z) { m_zeros.push_back(z); updateBodePlot(); }
void TransferFunctionControls::updatePoleZero(int index, bool isPole, Complex newVal) {
    // FIXED: Added static_cast<int> to silence signed/unsigned mismatch warning
    if (isPole && index >= 0 && index < static_cast<int>(m_poles.size())) m_poles[index] = newVal;
    else if (!isPole && index >= 0 && index < static_cast<int>(m_zeros.size())) m_zeros[index] = newVal;
    updateBodePlot();
}

void TransferFunctionControls::removeClosestPoleZero(Complex pos) {
    float minDist = 1000.0f; int index = -1; bool isPole = true;
    // FIXED: Changed 'int i' to 'size_t i'
    for(size_t i=0; i<m_poles.size(); ++i) { 
        float d = std::abs(m_poles[i] - pos); 
        if(d < minDist) { minDist = d; index = static_cast<int>(i); isPole = true; } 
    }
    // FIXED: Changed 'int i' to 'size_t i'
    for(size_t i=0; i<m_zeros.size(); ++i) { 
        float d = std::abs(m_zeros[i] - pos); 
        if(d < minDist) { minDist = d; index = static_cast<int>(i); isPole = false; } 
    }
    if(index != -1 && minDist < 0.2f) { 
        if(isPole) m_poles.erase(m_poles.begin() + index); 
        else m_zeros.erase(m_zeros.begin() + index); 
        updateBodePlot(); 
    }
}
void TransferFunctionControls::clearPoleZero() { m_poles.clear(); m_zeros.clear(); updateBodePlot(); }
void TransferFunctionControls::updateBodePlot()
{
	const int N = 512; float fs = 44100.0f; if (Engine::audioEngine()) fs = Engine::audioEngine()->baseSampleRate();
	const float minFreq = 20.0f; const float maxFreq = fs * 0.5f;
	int presetIdx = static_cast<int>(m_volumeModel.value()); if (presetIdx < 1) presetIdx = 1; if (presetIdx > 20) presetIdx = 20; 
	std::vector<float> mag(N); std::vector<float> phase(N); SimpleParser parser; const float PI = 3.14159265358979f; 
	for (int i = 0; i < N; ++i) {
		float percent = static_cast<float>(i) / static_cast<float>(N - 1); float freq = minFreq * std::pow(maxFreq / minFreq, percent);
		Complex H = calculateTransferFunction(presetIdx, freq, m_formulaString.toStdString(), parser, &m_drawPoints, &m_poles, &m_zeros, fs);
		float m = std::abs(H); float p = std::arg(H);
        if (presetIdx == 20) { m = std::atan(m) * 2.0f / PI; } 
		float db = 20.0f * std::log10(m + 1e-12f); float magNorm = (db + 40.0f) / 80.0f; if (magNorm < 0.0f) magNorm = 0.0f; if (magNorm > 1.0f) magNorm = 1.0f; mag[i] = magNorm;
		float phaseNorm = (p + PI) / (2.0f * PI); if (phaseNorm < 0.0f) phaseNorm = 0.0f; if (phaseNorm > 1.0f) phaseNorm = 1.0f; phase[i] = phaseNorm;
	}
	m_bodeMagModel.setSamples(mag.data()); m_bodePhaseModel.setSamples(phase.data());
}
void TransferFunctionControls::setFormula(const QString& text) { if (m_formulaString != text) { m_formulaString = text; updateBodePlot(); } }
} // namespace lmms
