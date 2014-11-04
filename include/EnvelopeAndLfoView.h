/*
 * EnvelopeAndLfoView.h - declaration of class EnvelopeAndLfoView which
 *                        is used by envelope/lfo/filter-tab of instrument track
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef ENVELOPE_AND_LFO_VIEW_H
#define ENVELOPE_AND_LFO_VIEW_H

#include <QtGui/QWidget>

#include "ModelView.h"

class QPaintEvent;
class QPixmap;

class EnvelopeAndLfoParameters;

class automatableButtonGroup;
class knob;
class ledCheckBox;
class pixmapButton;
class TempoSyncKnob;



class EnvelopeAndLfoView : public QWidget, public ModelView
{
	Q_OBJECT
public:
	EnvelopeAndLfoView( QWidget * _parent );
	virtual ~EnvelopeAndLfoView();


protected:
	virtual void modelChanged();

	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );


protected slots:
	void lfoUserWaveChanged();


private:
	static QPixmap * s_envGraph;
	static QPixmap * s_lfoGraph;

	EnvelopeAndLfoParameters * m_params;


	// envelope stuff
	knob * m_predelayKnob;
	knob * m_attackKnob;
	knob * m_holdKnob;
	knob * m_decayKnob;
	knob * m_sustainKnob;
	knob * m_releaseKnob;
	knob * m_amountKnob;

	// LFO stuff
	knob * m_lfoPredelayKnob;
	knob * m_lfoAttackKnob;
	TempoSyncKnob * m_lfoSpeedKnob;
	knob * m_lfoAmountKnob;
	pixmapButton * m_userLfoBtn;
	automatableButtonGroup * m_lfoWaveBtnGrp;

	ledCheckBox * m_x100Cb;
	ledCheckBox * m_controlEnvAmountCb;
	
	float m_randomGraph;
} ;

#endif
