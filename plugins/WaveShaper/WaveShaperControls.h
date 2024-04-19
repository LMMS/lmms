/*
 * WaveShaperControls.h - controls for WaveShaper effect
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

#ifndef WAVESHAPER_CONTROLS_H
#define WAVESHAPER_CONTROLS_H

#include <vector>

#include "EffectControls.h"
#include "WaveShaperControlDialog.h"
#include "VectorGraph.h"

namespace lmms
{


class WaveShaperEffect;


class WaveShaperControls : public EffectControls
{
	Q_OBJECT
public:
	WaveShaperControls( WaveShaperEffect * _eff );
	~WaveShaperControls() override = default;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return( "waveshapercontrols" );
	}

	virtual void setDefaultShape();

	int controlCount() override
	{
		return( 4 );
	}

	gui::EffectControlDialog* createView() override
	{
		return( new gui::WaveShaperControlDialog( this ) );
	}

	std::vector<float>* getGraphSamples();



private slots:
	void resetClicked();

	void vectorGraphChanged();
signals:
	// connected to dataArrayChanged()
	// shouldUseGetLastValuesIn: when the graph is redrawn
	// should it use the last updated values instead of updating again
	void vectorGraphUpdateView(bool shouldUseGetLastValuesIn);
private:
	WaveShaperEffect * m_effect;
	FloatModel m_inputModel;
	FloatModel m_outputModel;
	//graphModel m_wavegraphModel;
	VectorGraphModel m_vectorGraphModel;
	BoolModel  m_clipModel;

	friend class gui::WaveShaperControlDialog;
	friend class WaveShaperEffect;

	std::vector<float> m_vectorGraphSampleBuffer;
} ;


} // namespace lmms

#endif
