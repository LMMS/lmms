/*
 * InstrumentSoundShapingView.h - view for InstrumentSoundShaping class
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef INSTRUMENT_SOUND_SHAPING_VIEW_H
#define INSTRUMENT_SOUND_SHAPING_VIEW_H

#include <QWidget>

#include "InstrumentSoundShaping.h"
#include "ModelView.h"

class QLabel;

class EnvelopeAndLfoView;
class ComboBox;
class GroupBox;
class Knob;
class TabWidget;


class InstrumentSoundShapingView : public QWidget, public ModelView
{
	Q_OBJECT
public:
	InstrumentSoundShapingView( QWidget * _parent );
	virtual ~InstrumentSoundShapingView();

	void setFunctionsHidden( bool hidden );


private:
	virtual void modelChanged();


	InstrumentSoundShaping * m_ss;
	TabWidget * m_targetsTabWidget;
	EnvelopeAndLfoView * m_envLfoViews[InstrumentSoundShaping::NumTargets];

	// filter-stuff
	GroupBox * m_filterGroupBox;
	ComboBox * m_filterComboBox;
	Knob * m_filterCutKnob;
	Knob * m_filterResKnob;

	QLabel* m_singleStreamInfoLabel;

} ;

#endif
