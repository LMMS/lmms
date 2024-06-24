/*
 * Groove.h - classes for addinng swing/funk/groove/slide (you can't name it but you can feel it)
 *  to midi which is not precise enough at 192 ticks per tact to make your arse move.
 *
 * In it simplest terms a groove is a subtle delay on some notes in a pattern.
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
 *
 */
#ifndef GROOVE_H
#define GROOVE_H

#include <QWidget>

#include "lmms_basics.h"
#include "TimePos.h"
#include "Note.h"
#include "MidiClip.h"
#include "SerializingObject.h"

namespace lmms
{

class Groove : public QObject, public SerializingObject
{
	Q_OBJECT

public:
	Groove(QObject* parent = nullptr);

	/*
	 * Groove should return true if the note should be played in the curr_time tick,
	 * at the start of the tick or any time before the next tick.
	 *
	 * cur_start - the tick about to be played
	 * n - the note to be played, or not played
	 * p - the pattern to which the note belongs
	 *
	 * default implementation (no groove) would be   return n.pos() == cur_start ? 0 : -1;
	 *
	 * returns 0 to play now on the tick, -1 to not play at all and the new offset
	 *         that the note should be shifted if it is to be played later in this tick.
	 */
	virtual int isInTick(TimePos* curStart, fpp_t frames, f_cnt_t offset,
						Note* n, MidiClip* c);
	int amount() const { return m_amount; }

	virtual void saveSettings(QDomDocument & doc, QDomElement & element);
	virtual void loadSettings(const QDomElement & element);

	virtual QWidget* instantiateView(QWidget* parent);

	QString nodeName() const override
	{
		return "none";
	}

signals:
	void amountChanged(int newAmount);

public slots:
	void setAmount(int amount);

protected:
	int m_amount;
	float m_swingFactor; // = (m_amount / 127)
};

} // namespace lmms

#endif // GROOVE_H
