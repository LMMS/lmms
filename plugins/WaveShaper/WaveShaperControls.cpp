/*
 * WaveShaperControls.cpp - controls for WaveShaper effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <vector>
#include <QDomElement>

#include "WaveShaperControls.h"
#include "WaveShaper.h"
#include "VectorGraph.h"
#include "Engine.h"
#include "Song.h"
#include "base64.h"

namespace lmms
{


#define onedB 1.1220184543019633f

WaveShaperControls::WaveShaperControls( WaveShaperEffect * _eff ) :
	EffectControls( _eff ),
	m_effect( _eff ),
	m_inputModel( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Input gain" ) ),
	m_outputModel( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Output gain" ) ),
	m_vectorGraphModel(1024, this, false),
	m_clipModel( false, this ),
	m_vectorGraphSampleBuffer(200)
{

	unsigned int arrayLocationB = m_vectorGraphModel.addArray();
	m_vectorGraphModel.getDataArray(arrayLocationB)->setIsSelectable(true);
	m_vectorGraphModel.getDataArray(arrayLocationB)->setIsEditableAttrib(true);
	m_vectorGraphModel.getDataArray(arrayLocationB)->setIsAutomatableEffectable(true);
	m_vectorGraphModel.getDataArray(arrayLocationB)->setIsSaveable(true);

	unsigned int arrayLocation = m_vectorGraphModel.addArray();
	//m_vectorGraphModel.getDataArray(arrayLocation)->setIsFixedSize(true);
	//m_vectorGraphModel.getDataArray(arrayLocation)->setIsFixedX(true);
	//m_vectorGraphModel.getDataArray(arrayLocation)->setIsFixedY(true);
	//m_vectorGraphModel.getDataArray(arrayLocation)->setIsFixedEndPoints(true);
	m_vectorGraphModel.getDataArray(arrayLocation)->setIsSelectable(true);
	m_vectorGraphModel.getDataArray(arrayLocation)->setIsEditableAttrib(true);
	m_vectorGraphModel.getDataArray(arrayLocation)->setIsAutomatableEffectable(true);
	m_vectorGraphModel.getDataArray(arrayLocation)->setIsSaveable(true);
	m_vectorGraphModel.getDataArray(arrayLocation)->setIsNonNegative(true);

	// connect VectorGraphDataArrays so the 1. VectorGraphDataArray will be able to effect the 2.:
	m_vectorGraphModel.getDataArray(arrayLocation)->setEffectorArrayLocation(arrayLocationB, true);
	// draw default shape
	setDefaultShape();

	connect(&m_vectorGraphModel, SIGNAL(dataChanged()), this, SLOT(vectorGraphChanged()));
	connect(this, SIGNAL(vectorGraphUpdateView(bool)), &m_vectorGraphModel, SLOT(updateGraphModel(bool)));
}


void WaveShaperControls::loadSettings( const QDomElement & _this )
{
//load input, output knobs
	m_inputModel.loadSettings( _this, "inputGain" );
	m_outputModel.loadSettings( _this, "outputGain" );

	m_clipModel.loadSettings( _this, "clipInput" );

	if (_this.hasAttribute("waveShape") == true)
	{
		if (m_vectorGraphModel.getDataArraySize() > 1)
		{
			int size = 0;
			char * dst = 0;
			base64::decode(_this.attribute("waveShape"), &dst, &size);
			float* graphSampleArray = (float*)dst;

			// loading old graph data into new vectorGraph
			m_vectorGraphModel.getDataArray(1)->setDataArray(graphSampleArray, 200, false, false, false, true, true);

			delete[] dst;
		}
	}
	else
	{
		// loading VectorGraph
		m_vectorGraphModel.loadSettings(_this, "VectorGraph1");
	}
}




void WaveShaperControls::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
//save input, output knobs
	m_inputModel.saveSettings( _doc, _this, "inputGain" );
	m_outputModel.saveSettings( _doc, _this, "outputGain" );

	m_clipModel.saveSettings( _doc, _this, "clipInput" );

	// saving VectorGraph
	m_vectorGraphModel.saveSettings(_doc, _this, "VectorGraph1"); 
}

std::vector<float>* WaveShaperControls::getGraphSamples()
{
	if (m_vectorGraphModel.getDataArraySize() > 1)
	{
		// get m_vectorGraphSampleBuffer.size (200) samples from the 1. VectorGraphDataArray
		m_vectorGraphSampleBuffer = m_vectorGraphModel.getDataArray(1)->getValues(m_vectorGraphSampleBuffer.size());
		emit vectorGraphUpdateView(true);
	}
	return &m_vectorGraphSampleBuffer;
}

void WaveShaperControls::setDefaultShape()
{
	for (unsigned int i = 0; i < m_vectorGraphModel.getDataArraySize(); i++)
	{
		// clearing the VectorGraphDataArray-s insied VectorGraphModel
		m_vectorGraphModel.getDataArray(i)->clear();
	}

	if (m_vectorGraphModel.getDataArraySize() > 1)
	{
		int addedLocation = m_vectorGraphModel.getDataArray(1)->add(0.0f);
		if (addedLocation >= 0)
		{
			m_vectorGraphModel.getDataArray(1)->setY(addedLocation, -1.0f);
		}

		addedLocation = m_vectorGraphModel.getDataArray(1)->add(1.0f);
		if (addedLocation >= 0)
		{
			m_vectorGraphModel.getDataArray(1)->setY(addedLocation, 1.0f);
		}

		addedLocation = m_vectorGraphModel.getDataArray(0)->add(0.5f);
		if (addedLocation >= 0)
		{
			m_vectorGraphModel.getDataArray(0)->setY(addedLocation, -1.0f);
		}
	}
}

void WaveShaperControls::resetClicked()
{
	setDefaultShape();
	Engine::getSong()->setModified();
}

void WaveShaperControls::vectorGraphChanged()
{
	Engine::getSong()->setModified();
}

} // namespace lmms
