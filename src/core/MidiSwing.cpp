/*
 * MidiSwing.cpp - Swing algo using fixed integer step adjustments that
 *                 could be represented as midi.
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

#include <QLabel>

#include "MidiSwing.h"

namespace lmms
{

MidiSwing::MidiSwing(QObject * parent) :
	Groove(parent)
{
}

MidiSwing::~MidiSwing()
{
}

static int applyMidiSwing(int posInEight);

int MidiSwing::isInTick(TimePos * curStart, const fpp_t frames, const f_cnt_t offset,
					Note * n, MidiClip* c)
{
	return isInTick(curStart, n, c);
}

int MidiSwing::isInTick(TimePos * curStart, Note * n, MidiClip* c)
{

	// Where are we in the beat
	int posInBeat =  n->pos().getTicks() % 48; // assumes 48 ticks per beat, todo verify this


	// the Midi Swing algorithm.

	int posInEigth = -1;
	if (posInBeat >= 12 && posInBeat < 18)
	{
		// 1st half of second quarter
		// add a 0 - 24 tick shift
		posInEigth = posInBeat - 12;  // 0-5
	}
	else  if (posInBeat >= 36 && posInBeat < 42)
	{
		// 1st half of third quarter
		posInEigth = posInBeat - 36;  // 0-5
	}

	int swingTicks = applyMidiSwing(posInEigth);

	return curStart->getTicks() == swingTicks + n->pos().getTicks() ? 0 : -1;

}

QWidget * MidiSwing::instantiateView(QWidget * parent)
{
	return new QLabel("");
}

static int applyMidiSwing(int posInEight)
{
	// TODO case
	if (posInEight < 0) return 0;
	if (posInEight == 0) return 3;
	if (posInEight == 1) return 3;
	if (posInEight == 2) return 4;
	if (posInEight == 3) return 4;
	if (posInEight == 4) return 5;
	if (posInEight == 5) return 5;
	return 0;
}

} // namespace lmms
