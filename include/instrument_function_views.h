/*
 * instrument_function_views.h - views for instrument-functions-tab
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


#ifndef _INSTRUMENT_FUNCTION_VIEWS_H
#define _INSTRUMENT_FUNCTION_VIEWS_H

#include "mv_base.h"

#include <QtGui/QWidget>

class QLabel;
class comboBox;
class groupBox;
class knob;
class tempoSyncKnob;

class arpeggiator;
class chordCreator;



class chordCreatorView : public QWidget, public modelView
{
	Q_OBJECT
public:
	chordCreatorView( chordCreator * _cc, QWidget * _parent );
	virtual ~chordCreatorView();


private:
	virtual void modelChanged( void );

	chordCreator * m_cc;

	groupBox * m_chordsGroupBox;
	comboBox * m_chordsComboBox;
	knob * m_chordRangeKnob;

} ;





class arpeggiatorView : public QWidget, public modelView
{
	Q_OBJECT
public:
	arpeggiatorView( arpeggiator * _arp, QWidget * _parent );
	virtual ~arpeggiatorView();


private:
	virtual void modelChanged( void );

	arpeggiator * m_a;
	groupBox * m_arpGroupBox;
	comboBox * m_arpComboBox;
	knob * m_arpRangeKnob;
	tempoSyncKnob * m_arpTimeKnob;
	knob * m_arpGateKnob;

	QLabel * m_arpDirectionLbl;

	comboBox * m_arpDirectionComboBox;
	comboBox * m_arpModeComboBox;

} ;


#endif
