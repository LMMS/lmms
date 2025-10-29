/*
 * DynamicsProcessorControls.h - controls for DynamicsProcessor-effect
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

#ifndef DYNPROC_CONTROLS_H
#define DYNPROC_CONTROLS_H

#include "EffectControls.h"
#include "DynamicsProcessorControlDialog.h"
#include "Graph.h"

namespace lmms
{


class DynProcEffect;


class DynProcControls : public EffectControls
{
	Q_OBJECT
public:
	enum class StereoMode
	{
		Maximum,
		Average,
		Unlinked
	};
	DynProcControls( DynProcEffect * _eff );
	~DynProcControls() override = default;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return( "dynamicsprocessor_controls" );
	}

	virtual void setDefaultShape();

	int controlCount() override
	{
		return( 6 );
	}

	gui::EffectControlDialog * createView() override
	{
		return( new gui::DynProcControlDialog( this ) );
	}


private slots:
	void samplesChanged( int, int );
	void sampleRateChanged();

	void resetClicked();
	void smoothClicked();

	void addOneClicked();
	void subOneClicked();

private:
	DynProcEffect * m_effect;
	
	FloatModel m_inputModel;
	FloatModel m_outputModel;
	FloatModel m_attackModel;
	FloatModel m_releaseModel;
	graphModel m_wavegraphModel;
	IntModel m_stereomodeModel;

	friend class gui::DynProcControlDialog;
	friend class DynProcEffect;

} ;


} // namespace lmms

#endif
