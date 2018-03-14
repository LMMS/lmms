/*
 * GrooveExperiments.cpp - Try to find new groove algos that sound interesting
 *
 * Copyright (c) 2004-2014 teknopaul <teknopaul/at/users.sourceforge.net>
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
#include <QObject>
#include <QDomElement>
#include <QLabel>

//#include "AutomatableSlider.h"
#include "Engine.h"
#include "Groove.h"
#include "GrooveExperiments.h"
#include "lmms_basics.h"
#include "MidiTime.h"
#include "Note.h"
#include "Pattern.h"
#include "Song.h"

#include "stdio.h"

GrooveExperiments::GrooveExperiments(QObject * parent) :
	Groove(parent)
{
	m_amount = 0;
	m_swingFactor = 0.0;
	init();
	update();
}

GrooveExperiments::~GrooveExperiments()
{
}


void GrooveExperiments::init()
{

	Song * s = Engine::getSong();
	connect( s, SIGNAL(projectLoaded()),        this, SLOT(update()) );
	connect( s, SIGNAL(lengthChanged(int)),        this, SLOT(update()) );
	connect( s, SIGNAL(tempoChanged(bpm_t)),         this, SLOT(update()) );
	connect( s, SIGNAL(timeSignatureChanged(int, int)), this, SLOT(update()) );

}

void GrooveExperiments::update()
{
	m_frames_per_tick =  Engine::framesPerTick();
}

int GrooveExperiments::isInTick(MidiTime * _cur_start, const fpp_t _frames, const f_cnt_t _offset, Note * _n, Pattern * _p )
{
	// TODO why is this wrong on boot how do we set it once not every loop
	if ( m_frames_per_tick == 0 )
	{
		m_frames_per_tick =  Engine::framesPerTick(); // e.g. 500 at 120BPM 4/4
	}

	// only ever delay notes by 12 ticks, so if the tick is earlier don't play
	if ( _n->pos().getTicks() + 12 < _cur_start->getTicks())
	{
		return -1;
	}

	// else work out how much to offset the start point.

	// Where are we in the beat
	// 48 ticks to the beat, 192 ticks to the bar
	int pos_in_beat =  _n->pos().getTicks() % 48;


	int pos_in_eigth = -1;
	if ( pos_in_beat >= 36 && pos_in_beat < 48 )
	{
		// third quarter
		pos_in_eigth = pos_in_beat - 36;  // 0-11
	}

	if ( pos_in_eigth >= 0 ) 
	{

		float ticks_to_shift = ((pos_in_eigth - 12) * -m_swingFactor);
		
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

QWidget * GrooveExperiments::instantiateView( QWidget * _parent )
{
	return new GrooveExperimentsView(this, _parent);
}



// VIEW //

GrooveExperimentsView::GrooveExperimentsView(GrooveExperiments * _ge, QWidget * _parent) :
	QWidget(_parent)
{
	m_sliderModel = new IntModel(0, 0, 127); // Unused
	m_slider = new AutomatableSlider(this, tr("Swinginess"));
	m_slider->setOrientation(Qt::Horizontal);
	m_slider->setFixedSize( 90, 26 );
	m_slider->setPageStep(1);
	m_slider->setModel(m_sliderModel);
	m_sliderModel->setValue(_ge->amount());

	m_ge = _ge;

	connect(m_slider, SIGNAL(sliderMoved(int)), this, SLOT(valueChanged(int)));
	connect(m_sliderModel, SIGNAL(dataChanged()), this, SLOT(modelChanged()));
}

GrooveExperimentsView::~GrooveExperimentsView()
{
	delete m_slider;
	delete m_sliderModel;
}

void GrooveExperimentsView::modelChanged()
{
	m_ge->setAmount((int)m_sliderModel->value());
}

void GrooveExperimentsView::valueChanged(int _i) // this value passed is gibberish
{
	m_ge->setAmount((int)m_sliderModel->value());
}
