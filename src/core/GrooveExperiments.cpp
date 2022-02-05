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
#include <QLabel>

#include "Engine.h"
#include "Groove.h"
#include "GrooveExperiments.h"
#include "lmms_basics.h"
#include "TimePos.h"
#include "Note.h"
#include "MidiClip.h"
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
	connect(s, SIGNAL(projectLoaded()),        this, SLOT(update()));
	connect(s, SIGNAL(lengthChanged(int)),        this, SLOT(update()));
	connect(s, SIGNAL(tempoChanged(bpm_t)),         this, SLOT(update()));
	connect(s, SIGNAL(timeSignatureChanged(int, int)), this, SLOT(update()));

}

void GrooveExperiments::update()
{
	m_framesPerTick =  Engine::framesPerTick();
}

int GrooveExperiments::isInTick(TimePos * curStart, const fpp_t frames, const f_cnt_t offset, Note * n, MidiClip* c)
{
	// TODO why is this wrong on boot how do we set it once not every loop
	if (m_framesPerTick == 0)
	{
		m_framesPerTick =  Engine::framesPerTick(); // e.g. 500 at 120BPM 4/4
	}

	// only ever delay notes by 12 ticks, so if the tick is earlier don't play
	if (n->pos().getTicks() + 12 < curStart->getTicks())
	{
		return -1;
	}

	// else work out how much to offset the start point.

	// Where are we in the beat
	// 48 ticks to the beat, 192 ticks to the bar
	int posInBeat =  n->pos().getTicks() % 48;


	int posInEigth = -1;
	if (posInBeat >= 36 && posInBeat < 48)
	{
		// third quarter
		posInEigth = posInBeat - 36;  // 0-11
	}

	if (posInEigth >= 0)
	{

		float ticksToShift = ((posInEigth - 12) * -m_swingFactor);

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

QWidget * GrooveExperiments::instantiateView(QWidget * parent)
{
	return new GrooveExperimentsView(this, parent);
}



// VIEW //

GrooveExperimentsView::GrooveExperimentsView(GrooveExperiments * groove, QWidget * parent) :
	QWidget(parent)
{
	m_sliderModel = new IntModel(0, 0, 127); // Unused
	m_sliderModel->setValue(groove->amount());
	m_slider = new AutomatableSlider(this, tr("Swinginess"));
	m_slider->setOrientation(Qt::Horizontal);
	m_slider->setFixedSize(90, 26);
	m_slider->setPageStep(1);
	m_slider->setModel(m_sliderModel);

	m_groove = groove;

	connect(m_slider, SIGNAL(sliderMoved()), this, SLOT(valueChanged()));
	connect(m_sliderModel, SIGNAL(dataChanged()), this, SLOT(modelChanged()));
}

GrooveExperimentsView::~GrooveExperimentsView()
{
	delete m_slider;
	delete m_sliderModel;
}

void GrooveExperimentsView::valueChanged()
{
	m_groove->setAmount(m_sliderModel->value());
}

void GrooveExperimentsView::modelChanged()
{
	m_groove->setAmount(m_sliderModel->value());
}
