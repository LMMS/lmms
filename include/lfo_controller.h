/*
 * lfo_controller.h - A LFO-based controller and dialog
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
#include "controller.h"
#include "controller_dialog.h"
#include "tempo_sync_knob.h"
#include "oscillator.h"


class automatableButtonGroup;
class knob;
class tempoSyncKnob;
class pixmapButton;
class oscillator;

class lfoController : public controller 
{
	Q_OBJECT
public:
	lfoController( model * _parent );

	virtual ~lfoController();

	virtual QString publicName() const
	{
		return "LFO Controller";
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );
	virtual QString nodeName( void ) const;

public slots:
	virtual controllerDialog * createDialog( QWidget * _parent );

protected:

	// The internal per-controller get-value function
	virtual float value( int _offset );

	/*
slots:
	void trigger();

	*/

	floatModel m_lfoBaseModel;
	tempoSyncKnobModel m_lfoSpeedModel;
	floatModel m_lfoAmountModel;
	floatModel m_lfoPhaseModel;
	intModel m_lfoWaveModel;

	int m_duration;
	int m_phaseCorrection;
	int m_phaseOffset;
	
	sample_t (*m_sampleFunction)( const float );

protected slots:
	void updateSampleFunction( void );

	friend class lfoControllerDialog;
};



class lfoControllerDialog : public controllerDialog
{
	Q_OBJECT
public:
	lfoControllerDialog( controller * _controller, QWidget * _parent );
	virtual ~lfoControllerDialog();

public slots:
	//void editControls( void );
	//void deletePlugin( void );
	//void displayHelp( void );
	//void closeEffects( void );

	
signals:


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void modelChanged( void );

	lfoController * m_lfo;

	knob * m_lfoBaseKnob;
	tempoSyncKnob * m_lfoSpeedKnob;
	knob * m_lfoAmountKnob;
	knob * m_lfoPhaseKnob;

	pixmapButton * m_userLfoBtn;
	automatableButtonGroup * m_lfoWaveBtnGrp;

} ;

#endif
