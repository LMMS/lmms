/*
 * HalfSwing.cpp - Swing algo that varies adjustments form 0-127
 *              The algorythm is just the latter half of the HydrogenSwing groove..
 *
 * Copyright (c) 2004-2014 teknopaul <teknopaul/at/users.sourceforge.net>
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
#include <QtCore/QObject>
#include <QtXml/QDomElement>
#include <QtGui/QLabel>

#include "Engine.h"
#include "Groove.h"
#include "HalfSwing.h"
#include "Knob.h"
#include "lmms_basics.h"
#include "MidiTime.h"
#include "Note.h"
#include "Pattern.h"
#include "Song.h"

#include "stdio.h"


HalfSwing::HalfSwing(QObject * _parent) :
	QObject( _parent ),
	Groove()
{
	m_swingAmount = 0;
	m_swingFactor = 0;
	init();
	update();
}

HalfSwing::~HalfSwing()
{
}


void HalfSwing::init()
{

	Song * s = Engine::getSong();
	connect( s, SIGNAL(projectLoaded()),        this, SLOT(update()) );
	connect( s, SIGNAL(lengthChanged(int)),        this, SLOT(update()) );
	connect( s, SIGNAL(tempoChanged(bpm_t)),         this, SLOT(update()) );
	connect( s, SIGNAL(timeSignatureChanged(int, int)), this, SLOT(update()) );

}

int HalfSwing::amount()
{
	return m_swingAmount;
}

void HalfSwing::update()
{
	m_frames_per_tick =  Engine::framesPerTick();
}

void HalfSwing::setAmount(int _amount)
{

	if (_amount > 0 && _amount <= 127)
	{
		m_swingAmount = _amount;
		m_swingFactor =  (((float)m_swingAmount) / 127.0);
		emit swingAmountChanged(m_swingAmount);
	}
	else if (_amount  == 0)
	{
		m_swingAmount = 0;
		m_swingFactor =  0.0;
		emit swingAmountChanged(m_swingAmount);
	}
	else
	{
		m_swingAmount = 127;
		m_swingFactor =  1.0;
		emit swingAmountChanged(m_swingAmount);
	}

}


int HalfSwing::isInTick(MidiTime * _cur_start, const fpp_t _frames, const f_cnt_t _offset,
					 Note * _n, Pattern * _p )
{
	// TODO why is this wrong on boot how do we set it once not every loop
	if ( m_frames_per_tick == 0 )
	{
		m_frames_per_tick =  Engine::framesPerTick(); // e.g. 500 at 120BPM 4/4
	}

	// only ever delay notes by 7 ticks, so if the tick is earlier don't play
	if ( _n->pos().getTicks() + 7 < _cur_start->getTicks())
	{
		return -1;
	}

	// else work out how much to offset the start point.

	// Where are we in the beat
	// 48 ticks to the beat, 192 ticks to the bar
	int pos_in_beat =  _n->pos().getTicks() % 48;


	// The Half Swing algorthym.
	// Basically we delay (shift) notes on the the 4th quarter of the beat.

	int pos_in_eigth = -1;
	if ( pos_in_beat >= 36 && pos_in_beat < 42 )
	{
		// 1st half of third quarter
		pos_in_eigth = pos_in_beat - 36;  // 0-5
	}

	if ( pos_in_eigth >= 0 ) 
	{

		float ticks_to_shift = ((pos_in_eigth - 6) * -m_swingFactor);
		
		f_cnt_t frames_to_shift = (int)(ticks_to_shift * m_frames_per_tick);
		
		int tick_offset = (int)(frames_to_shift / m_frames_per_tick); // round down

		if ( _cur_start->getTicks() == (_n->pos().getTicks() + tick_offset) ) 
		{
			// play in this tick

			f_cnt_t new_offset = (frames_to_shift % m_frames_per_tick) + _offset;

			return new_offset;
		}
		else
		{ 
			// this note does not play in this tick
			return -1;
		}
	}

	// else no groove adjustments
	return _n->pos().getTicks() == _cur_start->getTicks() ? 0 : -1;
}

void HalfSwing::saveSettings( QDomDocument & _doc, QDomElement & _element )
{
	_element.setAttribute("swingAmount", m_swingAmount);
}

void HalfSwing::loadSettings( const QDomElement & _this )
{
	bool ok;
	int amount =  _this.attribute("swingAmount").toInt(&ok);
	if (ok)
	{
		setAmount(amount);
	}
	else
	{
		setAmount(0);
	}
}

QWidget * HalfSwing::instantiateView( QWidget * _parent )
{
	return new HalfSwingView(this, _parent);
}



// VIEW //

HalfSwingView::HalfSwingView(HalfSwing * _hy_swing, QWidget * _parent) :
	QWidget( _parent )
{
	m_nobModel = new FloatModel(0.0, 0.0, 127.0, 1.0); // Unused
	m_nob = new Knob(knobBright_26, this, "swingFactor");
	m_nob->setModel( m_nobModel );
	m_nob->setLabel( tr( "Swinginess" ) );
	m_nob->setEnabled(true);
	m_nobModel->setValue(_hy_swing->amount());

	m_hy_swing = _hy_swing;

	connect(m_nob, SIGNAL(sliderMoved(float)), this, SLOT(valueChanged(float)));
	connect(m_nobModel, SIGNAL( dataChanged() ), this, SLOT(modelChanged()) );

}

HalfSwingView::~HalfSwingView()
{
	delete m_nob;
	delete m_nobModel;
}

void HalfSwingView::modelChanged()
{
	m_hy_swing->setAmount((int)m_nobModel->value());
}

void HalfSwingView::valueChanged(float _f) // this value passed is gibberish
{
	m_hy_swing->setAmount((int)m_nobModel->value());
}
