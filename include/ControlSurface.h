/*
 * ControlSurface.h - Common control surface actions to lmms
 *
 * Copyright (c) 2025 - altrouge
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

#ifndef LMMS_CONTROL_SURFACE_H
#define LMMS_CONTROL_SURFACE_H

#include <QObject>

#include "lmms_export.h"

namespace lmms {
//! Implements functions linking controller to LMMS.
class LMMS_EXPORT ControlSurface : public QObject
{
	Q_OBJECT
public:
	ControlSurface();

public:
signals:
	void requestPlay();
	void requestStop();
	void requestLoop();
	void requestRecord();
	void requestPreviousInstrumentTrack();
	void requestNextInstrumentTrack();

	// Use slots to call from correct thread.
private slots:
	///! Starts playing.
	void play();
	///! Stops playing.
	void stop();
	///! Activate/deactivate the loop.
	void loop();
	///! Starts recording.
	void record();
	///! Selects previous instrument track.
	void previousInstrumentTrack();
	///! Selects next instrument track.
	void nextInstrumentTrack();
};
} // namespace lmms

#endif
