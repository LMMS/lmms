/*
 * TransferFunctionControls.cpp - controls for TransferFunction effect
 */

#include "TransferFunctionControls.h"
#include "TransferFunction.h"

// --- ADDED THIS INCLUDE TO FIX THE ERROR ---
#include <QDomElement> 
#include <QDomDocument>

namespace lmms
{

TransferFunctionControls::TransferFunctionControls(TransferFunctionEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	// Start at 1.0 (Preset 1), Min 1.0, Max 17.0, Step 1.0
	m_volumeModel(1.0f, 1.0f, 17.0f, 1.0f, this, tr("Preset")),
	m_formulaString("1.0 / (1.0 + 1j * (freq / 800.0))") // Default Custom Formula
{
}


void TransferFunctionControls::loadSettings(const QDomElement& parent)
{
	m_volumeModel.loadSettings(parent, "volume");
	
	// Load the string (if it exists)
	if (parent.hasAttribute("formula"))
	{
		m_formulaString = parent.attribute("formula");
	}
}


void TransferFunctionControls::saveSettings(QDomDocument& doc, QDomElement& parent)
{
	m_volumeModel.saveSettings(doc, parent, "volume"); 
	
	// Save the string
	parent.setAttribute("formula", m_formulaString);
}


} // namespace lmms