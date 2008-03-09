/*
 * instrument_sound_shaping_view.h - view for instrumentSoundShaping-class
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#ifndef _INSTRUMENT_SOUND_SHAPING_VIEW_H
#define _INSTRUMENT_SOUND_SHAPING_VIEW_H

#include <QtGui/QWidget>

#include "instrument_sound_shaping.h"

class envelopeAndLFOView;
class comboBox;
class groupBox;
class knob;
class tabWidget;


class instrumentSoundShapingView : public QWidget, public modelView
{
	Q_OBJECT
public:
	instrumentSoundShapingView( QWidget * _parent );
	virtual ~instrumentSoundShapingView();


private:
	virtual void modelChanged( void );


	instrumentSoundShaping * m_ss;
	tabWidget * m_targetsTabWidget;
	envelopeAndLFOView * m_envLFOViews[instrumentSoundShaping::NumTargets];

	// filter-stuff
	groupBox * m_filterGroupBox;
	comboBox * m_filterComboBox;
	knob * m_filterCutKnob;
	knob * m_filterResKnob;

} ;

#endif
