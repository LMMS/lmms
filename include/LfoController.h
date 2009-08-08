/*
 * LfoController.h - A LFO-based controller and dialog
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#ifndef _LFO_CONTROLLER_H
#define _LFO_CONTROLLER_H

#include <QtGui/QWidget>

#include "mv_base.h"
#include "automatable_model.h"
#include "Controller.h"
#include "ControllerDialog.h"
#include "tempo_sync_knob.h"
#include "oscillator.h"


class automatableButtonGroup;
class knob;
class ledCheckBox;
class tempoSyncKnob;
class pixmapButton;
class oscillator;

class LfoController : public Controller 
{
	Q_OBJECT
public:
	LfoController( model * _parent );

	virtual ~LfoController();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );
	virtual QString nodeName( void ) const;


public slots:
	virtual ControllerDialog * createDialog( QWidget * _parent );


protected:
	// The internal per-controller get-value function
	virtual float value( int _offset );

	floatModel m_baseModel;
	tempoSyncKnobModel m_speedModel;
	floatModel m_amountModel;
	floatModel m_phaseModel;
	intModel m_waveModel;
	intModel m_multiplierModel;

	int m_duration;
	int m_phaseCorrection;
	int m_phaseOffset;
	
	sample_t (*m_sampleFunction)( const float );


protected slots:
	void updateSampleFunction( void );

	friend class LfoControllerDialog;
} ;



class LfoControllerDialog : public ControllerDialog
{
	Q_OBJECT
public:
	LfoControllerDialog( Controller * _controller, QWidget * _parent );
	virtual ~LfoControllerDialog();


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void modelChanged( void );

	LfoController * m_lfo;

	knob * m_baseKnob;
	tempoSyncKnob * m_speedKnob;
	knob * m_amountKnob;
	knob * m_phaseKnob;
	pixmapButton * m_userLfoBtn;
	automatableButtonGroup * m_waveBtnGrp;
	automatableButtonGroup * m_multiplierBtnGrp;

} ;

#endif
