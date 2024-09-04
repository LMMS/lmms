/*
 * HalfSwing.cpp - Swing algo that varies adjustments form 0-127
 *              The algorythm is just the latter half of the HydrogenSwing groove..
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
#include "HalfSwing.h"
#include "lmms_basics.h"
#include "TimePos.h"
#include "Note.h"
#include "MidiClip.h"
#include "Song.h"

#include "stdio.h"


namespace lmms
{

HalfSwing::HalfSwing(QObject* parent) :
	Groove(parent)
{
	m_amount = 0;
	m_swingFactor = 0.0;
	init();
	update();
}

HalfSwing::~HalfSwing()
{
}


void HalfSwing::init()
{

	Song* s = Engine::getSong();
	connect(s, SIGNAL(projectLoaded()), this, SLOT(update()));
	connect(s, SIGNAL(lengthChanged(int)), this, SLOT(update()));
	connect(s, SIGNAL(tempoChanged(lmms::bpm_t)), this, SLOT(update()));
	connect(s, SIGNAL(timeSignatureChanged(int, int)), this, SLOT(update()));

}


void HalfSwing::update()
{
	m_framesPerTick =  Engine::framesPerTick();
}


int HalfSwing::isInTick(TimePos* curStart, const fpp_t frames, const f_cnt_t offset,
					Note* n, MidiClip* c)
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
	int pos_in_beat =  n->pos().getTicks() % 48;


	// The Half Swing algorithm.
	// Basically we delay (shift) notes on the the 4th quarter of the beat.

	int posInEigth = -1;
	if (pos_in_beat >= 36 && pos_in_beat < 42)
	{
		// 1st half of third quarter
		posInEigth = pos_in_beat - 36;  // 0-5
	}

	if (posInEigth >= 0)
	{

		float ticksToShift = ((posInEigth - 6) * -m_swingFactor);

		f_cnt_t framesToShift = (int)(ticksToShift* m_framesPerTick);

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

QWidget* HalfSwing::instantiateView(QWidget* parent)
{
	return new gui::HalfSwingView(this, parent);
}


namespace gui
{

// VIEW //

HalfSwingView::HalfSwingView(HalfSwing* halfSwing, QWidget* parent) :
	QWidget(parent)
{
	m_sliderModel = new IntModel(0, 0, 127); // Unused
	m_sliderModel->setValue(halfSwing->amount());
	m_slider = new AutomatableSlider(this, tr("Swinginess"));
	m_slider->setOrientation(Qt::Horizontal);
	m_slider->setFixedSize(90, 26);
	m_slider->setPageStep(1);
	m_slider->setModel(m_sliderModel);

	m_swing = halfSwing;

	connect(m_sliderModel, SIGNAL(dataChanged()), this, SLOT(valueChanged()));
}

HalfSwingView::~HalfSwingView()
{
	delete m_slider;
	delete m_sliderModel;
}

void HalfSwingView::valueChanged()
{
	m_swing->setAmount(m_sliderModel->value());
}

} // namespace gui

} // namespace lmms
