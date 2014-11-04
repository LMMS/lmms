/*
 * LfoController.h - A LFO-based controller and dialog
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#ifndef _LFO_CONTROLLER_H
#define _LFO_CONTROLLER_H

#include <QtGui/QWidget>

#include "Model.h"
#include "AutomatableModel.h"
#include "Controller.h"
#include "ControllerDialog.h"
#include "TempoSyncKnobModel.h"
#include "Oscillator.h"

class automatableButtonGroup;
class knob;
class ledCheckBox;
class TempoSyncKnob;
class pixmapButton;


class LfoController : public Controller 
{
	Q_OBJECT
public:
	LfoController( Model * _parent );

	virtual ~LfoController();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );
	virtual QString nodeName() const;


public slots:
	virtual ControllerDialog * createDialog( QWidget * _parent );


protected:
	// The internal per-controller get-value function
	virtual float value( int _offset );

	FloatModel m_baseModel;
	TempoSyncKnobModel m_speedModel;
	FloatModel m_amountModel;
	FloatModel m_phaseModel;
	IntModel m_waveModel;
	IntModel m_multiplierModel;

	int m_duration;
	int m_phaseCorrection;
	int m_phaseOffset;
	
	sample_t (*m_sampleFunction)( const float );

private:
	SampleBuffer * m_userDefSampleBuffer;

protected slots:
	void updateSampleFunction();

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
	virtual void modelChanged();

	LfoController * m_lfo;

	knob * m_baseKnob;
	TempoSyncKnob * m_speedKnob;
	knob * m_amountKnob;
	knob * m_phaseKnob;
	pixmapButton * m_userLfoBtn;
	automatableButtonGroup * m_waveBtnGrp;
	automatableButtonGroup * m_multiplierBtnGrp;


private:
	pixmapButton * m_userWaveBtn;

private slots:
	void askUserDefWave();

} ;

#endif
