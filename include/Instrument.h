/*
 * Instrument.h - declaration of class Instrument, which provides a
 *                standard interface for all instrument plugins
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

#ifndef LMMS_INSTRUMENT_H
#define LMMS_INSTRUMENT_H

#include <QString>
#include <cmath>

#include "Flags.h"
#include "SampleFrame.h"
#include "lmms_export.h"
#include "lmms_basics.h"
#include "Plugin.h"
#include "TimePos.h"

namespace lmms
{

// forward-declarations
class InstrumentTrack;
class MidiEvent;
class NotePlayHandle;
class PluginPinConnector;
class Track;


class LMMS_EXPORT Instrument : public Plugin
{
public:
	enum class Flag
	{
		NoFlags = 0x00,
		IsSingleStreamed = 0x01,	/*! Instrument provides a single audio stream for all notes */
		IsMidiBased = 0x02,			/*! Instrument is controlled by MIDI events rather than NotePlayHandles */
		IsNotBendable = 0x04,		/*! Instrument can't react to pitch bend changes */
	};

	using Flags = lmms::Flags<Flag>;

	Instrument(const Descriptor* _descriptor,
			InstrumentTrack* _instrument_track,
			const Descriptor::SubPluginFeatures::Key* key = nullptr,
			Flags flags = Flag::NoFlags);
	~Instrument() override = default;

	// if the plugin doesn't play each note, it can create an instrument-
	// play-handle and re-implement this method, so that it mixes its
	// output buffer only once per audio engine period
	void play(CoreAudioDataMut out)
	{
		playImpl(out);
	}

	void playNote(NotePlayHandle* notesToPlay, CoreAudioDataMut out)
	{
		playNoteImpl(notesToPlay, out);
	}

	// --------------------------------------------------------------------
	// functions that can/should be re-implemented:
	// --------------------------------------------------------------------

	//! Receives all incoming MIDI events; Return true if event was handled.
	virtual bool handleMidiEvent(const MidiEvent&, const TimePos& = TimePos(), f_cnt_t offset = 0)
	{
		return true;
	}

	/**
	 * Needed for deleting plugin-specific-data of a note - plugin has to
	 * cast void-ptr so that the plugin-data is deleted properly
	 * (call of dtor if it's a class etc.)
	 */
	virtual void deleteNotePluginData(NotePlayHandle* noteToPlay) {}

	/**
	 * Get number of sample-frames that should be used when playing beat
	 * (note with unspecified length)
	 * Per default this function returns 0. In this case, channel is using
	 * the length of the longest envelope (if one active).
	 */
	virtual auto beatLen(NotePlayHandle* nph) const -> f_cnt_t { return 0; }

	virtual bool hasNoteInput() const { return true; }

	// This method can be overridden by instruments that need a certain
	// release time even if no envelope is active. It returns the time
	// in milliseconds that these instruments would like to have for
	// their release stage.
	virtual float desiredReleaseTimeMs() const
	{
		return 0.f;
	}

	// Converts the desired release time in milliseconds to the corresponding
	// number of frames depending on the sample rate.
	f_cnt_t desiredReleaseFrames() const
	{
		const sample_rate_t sampleRate = getSampleRate();

		return static_cast<f_cnt_t>(std::ceil(desiredReleaseTimeMs() * sampleRate / 1000.f));
	}

	sample_rate_t getSampleRate() const;

	bool isSingleStreamed() const
	{
		return m_flags.testFlag(Instrument::Flag::IsSingleStreamed);
	}

	//! Returns whether the instrument is MIDI-based or NotePlayHandle-based
	bool isMidiBased() const
	{
		return !m_flags.testFlag(Instrument::Flag::IsMidiBased);
	}

	bool isBendable() const
	{
		return !m_flags.testFlag(Instrument::Flag::IsNotBendable);
	}

	//! Returns nullptr if the instrument does not have a pin connector
	virtual auto pinConnector() const -> const PluginPinConnector*
	{
		return nullptr;
	}

	QString fullDisplayName() const override;

	// --------------------------------------------------------------------
	// provided functions:
	// --------------------------------------------------------------------

	//! instantiate instrument-plugin with given name or return NULL
	//! on failure
	static Instrument * instantiate(const QString & _plugin_name,
		InstrumentTrack * _instrument_track,
		const Plugin::Descriptor::SubPluginFeatures::Key* key,
		bool keyFromDnd = false);

	virtual bool isFromTrack( const Track * _track ) const;

	inline InstrumentTrack * instrumentTrack() const
	{
		return m_instrumentTrack;
	}


protected:
	//! To be implemented by AudioPluginInterface or plugin implementation
	virtual void playImpl(CoreAudioDataMut out) {}

	//! To be implemented by AudioPluginInterface or plugin implementation
	virtual void playNoteImpl(NotePlayHandle* notesToPlay, CoreAudioDataMut out) {}

	// fade in to prevent clicks
	void applyFadeIn(SampleFrame* buf, NotePlayHandle * n);

	// instruments may use this to apply a soft fade out at the end of
	// notes - method does this only if really less or equal
	// desiredReleaseFrames() frames are left
	void applyRelease( SampleFrame* buf, const NotePlayHandle * _n );

	float computeReleaseTimeMsByFrameCount(f_cnt_t frames) const;


private:
	InstrumentTrack * m_instrumentTrack;
	Flags m_flags;
};


LMMS_DECLARE_OPERATORS_FOR_FLAGS(Instrument::Flag)


} // namespace lmms

#endif // LMMS_INSTRUMENT_H
