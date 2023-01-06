/*
 * MidiSwing.h - A swing groove that adjusts by whole ticks
 *
 * Copyright (c) 2005-2008 teknopaul <teknopaul/at/users.sourceforge.net>
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
 **/

#ifndef MIDISWING_H
#define MIDISWING_H

#include <QObject>

#include "Groove.h"
#include "lmms_basics.h"
#include "TimePos.h"
#include "Note.h"
#include "MidiClip.h"

namespace lmms
{

/*
 * A swing groove that adjusts by whole ticks.
 * Someone might like it, also might be able to save the output to a midi file later.
 */
class MidiSwing : public Groove
{
	Q_OBJECT
public:
	MidiSwing(QObject* parent = nullptr);
	~MidiSwing() override;

	// TODO why declaring this should it not come from super class?
	int isInTick(TimePos* cur_start, const fpp_t frames, const f_cnt_t offset, Note* n, MidiClip* c);
	int isInTick(TimePos* curStart, Note* n, MidiClip* c);

	QString nodeName() const override
	{
		return "midi";
	}

	QWidget* instantiateView(QWidget* parent);
};

}

#endif // MIDISWING_H
