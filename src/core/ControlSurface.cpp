/*
 * ControlSurface.cpp - Common control surface actions to lmms
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

#include "ControlSurface.h"

#include "Engine.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "MidiClip.h"
#include "PianoRoll.h"
#include "Song.h"
#include "SongEditor.h"

namespace lmms {

ControlSurface::ControlSurface()
{
	QObject::connect(this, &ControlSurface::requestPlay, this, &ControlSurface::play);
	QObject::connect(this, &ControlSurface::requestStop, this, &ControlSurface::stop);
	QObject::connect(this, &ControlSurface::requestLoop, this, &ControlSurface::loop);
	QObject::connect(this, &ControlSurface::requestRecord, this, &ControlSurface::record);
	QObject::connect(
		this, &ControlSurface::requestPreviousInstrumentTrack, this, &ControlSurface::previousInstrumentTrack);
	QObject::connect(this, &ControlSurface::requestNextInstrumentTrack, this, &ControlSurface::nextInstrumentTrack);
}

void ControlSurface::play()
{
	Engine::getSong()->playSong();
}

void ControlSurface::stop()
{
	auto piano_roll = gui::getGUI()->pianoRoll();
	if (piano_roll != nullptr) { piano_roll->stop(); }
	Engine::getSong()->stop();
}

void ControlSurface::loop()
{
	// Activate on MidiClip for piano roll and Song for whole song.
	auto& timeline_midi = Engine::getSong()->getTimeline(Song::PlayMode::MidiClip);
	timeline_midi.setLoopEnabled(!timeline_midi.loopEnabled());
	auto& timeline = Engine::getSong()->getTimeline(Song::PlayMode::Song);
	timeline.setLoopEnabled(!timeline.loopEnabled());
}

void ControlSurface::record()
{
	// Get the clip.
	auto assigned_instrument_track = InstrumentTrack::getAutoAssignedTrack();
	if (assigned_instrument_track != nullptr)
	{
		std::vector<Clip*> clips;
		auto current_time = Engine::getSong()->getPlayPos(Song::PlayMode::Song);
		assigned_instrument_track->getClipsInRange(clips, current_time, current_time);
		MidiClip* current_clip = nullptr;

		// If there are no available clips, create a clip.
		if (!clips.empty()) { current_clip = dynamic_cast<MidiClip*>(clips.front()); }
		else { current_clip = dynamic_cast<MidiClip*>(assigned_instrument_track->createClip(current_time)); }

		auto piano_roll = gui::getGUI()->pianoRoll();
		piano_roll->setCurrentMidiClip(current_clip);
		piano_roll->recordAccompany();
	}
}

void followingInstrumentTrack(bool reverse)
{
	// Get the clip.
	auto assigned_instrument_track = InstrumentTrack::getAutoAssignedTrack();
	const auto track_list = Engine::getSong()->tracks();
	int next = reverse ? -1 : 1;
	if (track_list.empty()) { return; }

	int start_ind = reverse ? track_list.size() - 1 : 0;
	if (assigned_instrument_track != nullptr)
	{
		for (size_t ind = 0; ind < track_list.size(); ++ind)
		{
			if (track_list[ind] == assigned_instrument_track)
			{
				start_ind = (ind + track_list.size() + next) % track_list.size();
				break;
			}
		}
	}
	for (size_t iteration = 0; iteration < track_list.size(); ++iteration)
	{
		size_t current_ind = (start_ind + track_list.size() + next * iteration) % track_list.size();
		if (track_list[current_ind]->type() == Track::Type::Instrument)
		{
			assigned_instrument_track = dynamic_cast<InstrumentTrack*>(track_list[current_ind]);
			assigned_instrument_track->autoAssignMidiDevice(true);
			break;
		}
	}
}

void ControlSurface::previousInstrumentTrack()
{
	followingInstrumentTrack(true);
}

void ControlSurface::nextInstrumentTrack()
{
	followingInstrumentTrack(false);
}
} // namespace lmms
