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

#include <iostream>

#include "WaveShaperControls.h"
#include "WaveShaper.h"
#include "VectorGraph.h"
#include "Engine.h"
#include "Song.h"

namespace lmms
{


#define onedB 1.1220184543019633f

WaveShaperControls::WaveShaperControls( WaveShaperEffect * _eff ) :
	EffectControls( _eff ),
	m_effect( _eff ),
	m_inputModel( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Input gain" ) ),
	m_outputModel( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Output gain" ) ),
	m_vectorGraphModel(1024, this, false),
	m_clipModel( false, this )
{

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
	m_vectorGraphModel.getDataArray(arrayLocation)->setLineColor(QColor(210, 50, 50, 255));
	m_vectorGraphModel.getDataArray(arrayLocation)->setActiveColor(QColor(255, 30, 20, 255));
	m_vectorGraphModel.getDataArray(arrayLocation)->setFillColor(QColor(170, 25, 25, 40));
	m_vectorGraphModel.getDataArray(arrayLocation)->setAutomatedColor(QColor(144, 107, 255, 255));

	unsigned int arrayLocationB = m_vectorGraphModel.addArray();
	m_vectorGraphModel.getDataArray(arrayLocationB)->setIsSelectable(true);
	m_vectorGraphModel.getDataArray(arrayLocationB)->setIsEditableAttrib(true);
	m_vectorGraphModel.getDataArray(arrayLocationB)->setIsAutomatableEffectable(true);
	m_vectorGraphModel.getDataArray(arrayLocationB)->setIsSaveable(true);
	m_vectorGraphModel.getDataArray(arrayLocationB)->setIsNonNegative(true);
	m_vectorGraphModel.getDataArray(arrayLocationB)->setLineColor(QColor(10, 50, 210, 255));
	m_vectorGraphModel.getDataArray(arrayLocationB)->setActiveColor(QColor(70, 170, 255, 255));
	m_vectorGraphModel.getDataArray(arrayLocationB)->setFillColor(QColor(70, 100, 180, 40));
	m_vectorGraphModel.getDataArray(arrayLocationB)->setAutomatedColor(QColor(144, 107, 255, 255));

	// connect VectorGraphDataArrays so the 2. will be able to effect the 1.:
	m_vectorGraphModel.getDataArray(arrayLocation)->setEffectorArrayLocation(arrayLocationB, true);
	// draw default shape
	setDefaultShape();

	connect(&m_vectorGraphModel, SIGNAL(dataChanged()),
			this, SLOT(vectorGraphChanged()));
	connect(this, SIGNAL(vectorGraphUpdateView(bool)),
			&m_vectorGraphModel, SLOT(updateGraphModel(bool)));
}




void WaveShaperControls::samplesChanged( int _begin, int _end)
{
	Engine::getSong()->setModified();
}




void WaveShaperControls::loadSettings( const QDomElement & _this )
{
//load input, output knobs
	m_inputModel.loadSettings( _this, "inputGain" );
	m_outputModel.loadSettings( _this, "outputGain" );

	m_clipModel.loadSettings( _this, "clipInput" );

	// loading VectorGraph
	m_vectorGraphModel.loadSettings(_this, "VectorGraph"); 
}




void WaveShaperControls::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
//save input, output knobs
	m_inputModel.saveSettings( _doc, _this, "inputGain" );
	m_outputModel.saveSettings( _doc, _this, "outputGain" );

	m_clipModel.saveSettings( _doc, _this, "clipInput" );

	// saving VectorGraph
	m_vectorGraphModel.saveSettings(_doc, _this, "VectorGraph"); 
}

std::vector<float> WaveShaperControls::getGraphSamples()
{
	if (m_vectorGraphModel.getDataArraySize() > 0)
	{
		std::vector<float> output = m_vectorGraphModel.getDataArray(0)->getValues(200);
		emit vectorGraphUpdateView(true);
		return output;
	}
	else
	{
		return std::vector<float>(200);
	}
}

void WaveShaperControls::setDefaultShape()
{
	for (unsigned int i = 0; i < m_vectorGraphModel.getDataArraySize(); i++)
	{
		// clearing the VectorGraphDataArray-s insied VectorGraphModel
		m_vectorGraphModel.getDataArray(i)->clear();
	}

	if (m_vectorGraphModel.getDataArraySize() > 0)
	{
		int addedLocation = m_vectorGraphModel.getDataArray(0)->add(0.0f);
		if (addedLocation >= 0)
		{
			m_vectorGraphModel.getDataArray(0)->setY(addedLocation, -1.0f);
		}

		addedLocation = m_vectorGraphModel.getDataArray(0)->add(1.0f);
		if (addedLocation >= 0)
		{
			m_vectorGraphModel.getDataArray(0)->setY(addedLocation, 1.0f);
		}
	}
}

void WaveShaperControls::resetClicked()
{
	Engine::getSong()->setModified();
}

void WaveShaperControls::smoothClicked()
{
	//m_wavegraphModel.smoothNonCyclic();
	Engine::getSong()->setModified();
}

void WaveShaperControls::addOneClicked()
{
	/*
	for( int i=0; i<200; i++ )
	{
		m_wavegraphModel.setSampleAt( i, qBound( 0.0f, m_wavegraphModel.samples()[i] * onedB, 1.0f ) );
	}
	*/
	Engine::getSong()->setModified();
}

void WaveShaperControls::subOneClicked()
{
	/*
	for( int i=0; i<200; i++ )
	{
		m_wavegraphModel.setSampleAt( i, qBound( 0.0f, m_wavegraphModel.samples()[i] / onedB, 1.0f ) );
	}
	*/
	Engine::getSong()->setModified();
}

void WaveShaperControls::vectorGraphChanged()
{
	Engine::getSong()->setModified();
}

} // namespace lmms
