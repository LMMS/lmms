/*
 * ClapTransport.h - CLAP transport events
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_CLAP_TRANSPORT_H
#define LMMS_CLAP_TRANSPORT_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include "lmms_basics.h"

#include <clap/events.h>

namespace lmms
{

//! Static class for managing CLAP transport events
class ClapTransport
{
public:
	static void update();
	static void setPlaying(bool isPlaying);
	static void setRecording(bool isRecording);
	static void setLooping(bool isLooping);
	static void setBeatPosition();
	static void setTimePosition(int elapsedMilliseconds);
	static void setTempo(bpm_t tempo);
	static void setTimeSignature(int num, int denom);

	static auto get() -> const clap_event_transport* { return &s_transport; }

private:
	static clap_event_transport s_transport;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_TRANSPORT_H
