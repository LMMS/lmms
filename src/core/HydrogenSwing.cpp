/*
 * HydrogenSwing.cpp - Swing algo that varies adjustments form 0-127
 *              The algorythm mimics Hydrogen drum machines swing feature.
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
#include <QLabel>

#include "Engine.h"
#include "Groove.h"
#include "HydrogenSwing.h"
#include "lmms_basics.h"
#include "TimePos.h"
#include "Note.h"
#include "MidiClip.h"
#include "Song.h"

#include "stdio.h"


namespace lmms
{

HydrogenSwing::HydrogenSwing(QObject * _parent) :
	Groove(_parent)
{
	m_amount = 0;
	m_swingFactor = 0.0;
	init();
	update();
}

HydrogenSwing::~HydrogenSwing()
{
}


void HydrogenSwing::init()
{

	Song * s = Engine::getSong();
	connect(s, SIGNAL(projectLoaded()),        this, SLOT(update()));
	connect(s, SIGNAL(lengthChanged(int)),        this, SLOT(update()));
	connect(s, SIGNAL(tempoChanged(bpm_t)),         this, SLOT(update()));
	connect(s, SIGNAL(timeSignatureChanged(int, int)), this, SLOT(update()));

}

void HydrogenSwing::update()
{
	m_framesPerTick =  Engine::framesPerTick();
}

int HydrogenSwing::isInTick(TimePos * curStart, const fpp_t frames, const f_cnt_t offset,
					Note * n, MidiClip* c)
{
	// TODO why is this wrong on boot how do we set it once not every loop
	if (m_framesPerTick == 0)
	{
		m_framesPerTick =  Engine::framesPerTick(); // e.g. 500 at 120BPM 4/4
	}

	// only ever delay notes by 7 ticks, so if the tick is earlier don't play
	if (n->pos().getTicks() + 7 < curStart->getTicks())
	{
		return -1;
	}

	// else work out how much to offset the start point.

	// Where are we in the beat
	// 48 ticks to the beat, 192 ticks to the bar
	int posInBeat =  n->pos().getTicks() % 48;


	// The Hydrogen Swing algorithm.
	// Guessed by turning the knob and watching the possitions change in Audacity.
	// Basically we delay (shift) notes on the the 2nd and 4th quarter of the beat.

	int posInEigth = -1;
	if (posInBeat >= 12 && posInBeat < 18)
	{
		// 1st half of second quarter
		posInEigth = posInBeat - 12;  // 0-5
	}
	else  if (posInBeat >= 36 && posInBeat < 42)
	{
		// 1st half of third quarter
		posInEigth = posInBeat - 36;  // 0-5
	}

	if (posInEigth >= 0)
	{

		float ticksToShift = ((posInEigth - 6) * -m_swingFactor);

		f_cnt_t framesToShift = (int)(ticksToShift * m_framesPerTick);

		int tickOffset = (int)(framesToShift / m_framesPerTick); // round down

		if (curStart->getTicks() == (n->pos().getTicks() + tickOffset))
		{
			// play in this tick

			f_cnt_t newOffset = (framesToShift % m_framesPerTick) + offset;

			return newOffset;
		}
		else
		{
			// this note does not play in this tick
			return -1;
		}
	}

	// else no groove adjustments
	return n->pos().getTicks() == curStart->getTicks() ? 0 : -1;
}

QWidget * HydrogenSwing::instantiateView(QWidget * parent)
{
	return new gui::HydrogenSwingView(this, parent);
}

namespace gui
{

// VIEW //

HydrogenSwingView::HydrogenSwingView(HydrogenSwing * swing, QWidget * parent) :
	QWidget(parent)
{
	m_sliderModel = new IntModel(0, 0, 127); // Unused
	m_sliderModel->setValue(swing->amount());
	m_slider = new AutomatableSlider(this, tr("Swinginess"));
	m_slider->setOrientation(Qt::Horizontal);
	m_slider->setFixedSize(90, 26);
	m_slider->setPageStep(1);
	m_slider->setModel(m_sliderModel);

	m_swing = swing;

	connect(m_slider, SIGNAL(sliderMoved()), this, SLOT(valueChanged()));
	connect(m_sliderModel, SIGNAL(dataChanged()), this, SLOT(modelChanged()));
}

HydrogenSwingView::~HydrogenSwingView()
{
	delete m_slider;
	delete m_sliderModel;
}

void HydrogenSwingView::valueChanged()
{
	m_swing->setAmount(m_sliderModel->value());
}

void HydrogenSwingView::modelChanged()
{
	m_swing->setAmount(m_sliderModel->value());
}

} // namespace gui

} // namespace lmms
