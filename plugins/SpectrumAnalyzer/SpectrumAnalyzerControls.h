/*
 * SpectrumAnalyzerControls.h - controls for spectrum-analyzer
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - http://lmms.io
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

#ifndef SPECTRUM_ANALYZER_CONTROLS_H
#define SPECTRUM_ANALYZER_CONTROLS_H

#include "EffectControls.h"
#include "SpectrumAnalyzerControlDialog.h"
#include "knob.h"


class SpectrumAnalyzer;


class SpectrumAnalyzerControls : public EffectControls
{
	Q_OBJECT
public:
	SpectrumAnalyzerControls( SpectrumAnalyzer* effect );
	virtual ~SpectrumAnalyzerControls()
	{
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return "spectrumanaylzercontrols";
	}

	virtual int controlCount()
	{
		return 1;
	}

	virtual EffectControlDialog * createView()
	{
		return new SpectrumAnalyzerControlDialog( this );
	}


private:
	SpectrumAnalyzer* m_effect;
	BoolModel m_linearSpec;
	BoolModel m_linearYAxis;
	IntModel m_channelMode;

	friend class SpectrumAnalyzer;
	friend class SpectrumAnalyzerControlDialog;
	friend class SpectrumView;

} ;

#endif
