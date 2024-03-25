/*
 * FFTFilterCotnrols.cpp - controls for FFTFilter effect
 *
 * Copyright (c) 2024 Szeli1 </at/gmail/dot/com> TODO
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include <QDomElement>

#include "FFTFilterControls.h"
#include "FFTFilter.h"
#include "VectorGraph.h"

namespace lmms
{

FFTFilterControls::FFTFilterControls(FFTFilterEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_volumeModel(100.0f, 0.0f, 200.0f, 0.1f, this, tr("Volume")),
	m_freqControlModel(100.0f, 0.0f, 200.0f, 0.1f, this, tr("Freq")),
	m_effectControlModel(0.0f, -100.0f, 100.0f, 0.1f, this, tr("Effect")),
	m_bufferModel(16, 2, 2048, this, "Quality"),
	//m_filterModel(0.0f, 1.0f, 200, this),
	m_displayFFTModel( true, this, tr("Display fft")),
	m_graphModel(1024, this, false)
{
	unsigned int arrayLocation = m_graphModel.addArray();
	m_graphModel.getDataArray(arrayLocation)->setIsSelectable(true);
	m_graphModel.getDataArray(arrayLocation)->setIsEditableAttrib(true);
	m_graphModel.getDataArray(arrayLocation)->setIsAutomatableEffectable(true);
	m_graphModel.getDataArray(arrayLocation)->setLineColor(QColor(210, 50, 50, 255));
	m_graphModel.getDataArray(arrayLocation)->setActiveColor(QColor(255, 30, 20, 255));
	m_graphModel.getDataArray(arrayLocation)->setFillColor(QColor(170, 25, 25, 255));
	m_graphModel.getDataArray(arrayLocation)->setAutomatedColor(QColor(144, 107, 255, 255));

	unsigned int arrayLocationB = m_graphModel.addArray();
	m_graphModel.getDataArray(arrayLocationB)->setIsSelectable(true);
	m_graphModel.getDataArray(arrayLocationB)->setIsEditableAttrib(true);
	m_graphModel.getDataArray(arrayLocationB)->setIsAutomatableEffectable(true);
	m_graphModel.getDataArray(arrayLocationB)->setLineColor(QColor(10, 50, 210, 255));
	m_graphModel.getDataArray(arrayLocationB)->setActiveColor(QColor(70, 170, 255, 255));
	m_graphModel.getDataArray(arrayLocationB)->setFillColor(QColor(70, 100, 180, 255));
	m_graphModel.getDataArray(arrayLocationB)->setAutomatedColor(QColor(10, 50, 255, 255));
	m_graphModel.getDataArray(arrayLocationB)->setAutomatedColor(QColor(10, 50, 255, 255));

	// effectors:
	m_graphModel.getDataArray(arrayLocation)->setEffectorArrayLocation(arrayLocationB);
}


void FFTFilterControls::loadSettings(const QDomElement& parent)
{
	m_volumeModel.loadSettings(parent, "volume");
	m_freqControlModel.loadSettings(parent, "freqcontrol");
	m_effectControlModel.loadSettings(parent, "effectcontrol");
	m_bufferModel.loadSettings(parent, "buffer");
	m_displayFFTModel.loadSettings(parent, "display");
}


void FFTFilterControls::saveSettings(QDomDocument& doc, QDomElement& parent)
{
	m_volumeModel.saveSettings(doc, parent, "volume"); 
	m_freqControlModel.saveSettings(doc, parent, "freqcontrol"); 
	m_effectControlModel.saveSettings(doc, parent, "effectcontrol"); 
	m_bufferModel.saveSettings(doc, parent, "buffer"); 
	m_displayFFTModel.saveSettings(doc, parent, "display"); 
}

void FFTFilterControls::resetClicked()
{

}

} // namespace lmms
