/*
 * InstrumentPlayHandle.h - play-handle for driving an instrument
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_INSTRUMENT_PLAY_HANDLE_H
#define LMMS_INSTRUMENT_PLAY_HANDLE_H

#include "PlayHandle.h"
#include "lmms_export.h"

namespace lmms
{

class Instrument;
class InstrumentTrack;

class LMMS_EXPORT InstrumentPlayHandle : public PlayHandle
{
public:
	InstrumentPlayHandle(Instrument * instrument, InstrumentTrack* instrumentTrack);

	~InstrumentPlayHandle() override = default;

	void play(CoreAudioDataMut buffer) override;

	bool isFinished() const override
	{
		return false;
	}

	bool isFromTrack(const Track* track) const override;

private:
	Instrument* m_instrument;
};

} // namespace lmms

#endif // LMMS_INSTRUMENT_PLAY_HANDLE_H
