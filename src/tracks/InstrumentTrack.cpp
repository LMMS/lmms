/*
 * InstrumentTrack.cpp - implementation of InstrumentTrack class
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "InstrumentTrack.h"

#include "AudioEngine.h"
#include "AutomationClip.h"
#include "ConfigManager.h"
#include "ControllerConnection.h"
#include "DataFile.h"
#include "GuiApplication.h"
#include "Mixer.h"
#include "InstrumentTrackView.h"
#include "Instrument.h"
#include "Keymap.h"
#include "MidiClient.h"
#include "MidiClip.h"
#include "MixHelpers.h"
#include "PatternStore.h"
#include "PatternTrack.h"
#include "PianoRoll.h"
#include "Pitch.h"
#include "Song.h"

namespace lmms
{


InstrumentTrack::InstrumentTrack(TrackContainer* tc) :
	Track(Track::Type::Instrument, tc),
	MidiEventProcessor(),
	m_midiPort(tr("unnamed_track"), Engine::audioEngine()->midiClient(), this, this),
	m_notes(),
	m_sustainPedalPressed(false),
	m_silentBuffersProcessed(false),
	m_previewMode(false),
	m_baseNoteModel(0, 0, NumKeys - 1, this, tr("Base note")),
	m_firstKeyModel(0, 0, NumKeys - 1, this, tr("First note")),
	m_lastKeyModel(0, 0, NumKeys - 1, this, tr("Last note")),
	m_hasAutoMidiDev(false),
	m_volumeModel(DefaultVolume, MinVolume, MaxVolume, 0.1f, this, tr("Volume")),
	m_panningModel(DefaultPanning, PanningLeft, PanningRight, 0.1f, this, tr("Panning")),
	m_audioBusHandle(tr("unnamed_track"), true, &m_volumeModel, &m_panningModel, &m_mutedModel),
	m_pitchModel(0, MinPitchDefault, MaxPitchDefault, 1, this, tr("Pitch")),
	m_pitchRangeModel(1, 1, 60, this, tr("Pitch range")),
	m_mixerChannelModel(0, 0, 0, this, tr("Mixer channel")),
	m_useMasterPitchModel(true, this, tr("Master pitch")),
	m_instrument(nullptr),
	m_soundShaping(this),
	m_arpeggio(this),
	m_noteStacking(this),
	m_piano(this),
	m_microtuner()
{
	m_pitchModel.setCenterValue( 0 );
	m_pitchModel.setStrictStepSize(true);
	m_panningModel.setCenterValue( DefaultPanning );
	m_baseNoteModel.setInitValue(DefaultBaseKey);
	m_firstKeyModel.setInitValue(0);
	m_lastKeyModel.setInitValue(NumKeys - 1);

	m_mixerChannelModel.setRange( 0, Engine::mixer()->numChannels()-1, 1);

	for( int i = 0; i < NumKeys; ++i )
	{
		m_notes[i] = nullptr;
		for (int channel = 0; channel < 16; ++channel)
		{
			m_runningMidiNotes[channel][i] = 0;
		}
	}


	// Initialize the m_midiCCEnabled variable, but it's actually going to be connected
	// to a LedButton
	m_midiCCEnable = std::make_unique<BoolModel>(false, nullptr, tr("Enable/Disable MIDI CC"));

	// Initialize the MIDI CC controller models and connect them to the method that processes
	// the midi cc events
	for (int i = 0; i < MidiControllerCount; ++i)
	{
		m_midiCCModel[i] = std::make_unique<FloatModel>(0.0f, 0.0f, 127.0f, 1.0f,
			nullptr, tr("CC Controller %1").arg(i));

		connect(m_midiCCModel[i].get(), &FloatModel::dataChanged,
			this, [this, i]{ processCCEvent(i); }, Qt::DirectConnection);
	}

	setName( tr( "Default preset" ) );

	connect(&m_baseNoteModel, SIGNAL(dataChanged()), this, SLOT(updateBaseNote()), Qt::DirectConnection);
	connect(&m_pitchModel, SIGNAL(dataChanged()), this, SLOT(updatePitch()), Qt::DirectConnection);
	connect(&m_pitchRangeModel, SIGNAL(dataChanged()), this, SLOT(updatePitchRange()), Qt::DirectConnection);
	connect(&m_mixerChannelModel, SIGNAL(dataChanged()), this, SLOT(updateMixerChannel()), Qt::DirectConnection);

	// Send signals when the base note or master transpose changes so that the piano widget can redraw itself..
	connect(&m_baseNoteModel, &AutomatableModel::dataChanged, this, &InstrumentTrack::transposeChanged);
	connect(&m_useMasterPitchModel, &AutomatableModel::dataChanged, this, &InstrumentTrack::transposeChanged);
	connect(Engine::getSong()->masterPitchModel(), &AutomatableModel::dataChanged, this, &InstrumentTrack::transposeChanged);

	// Resend the master pitch knob and pitch range midi signals just in case it was originally sent on a non-manager channel before MPE was enabled
	// With MPE, all the master pitch wheel signals must go to either channel 0 or channel 15.
	connect(&m_midiPort, &MidiPort::MPEConfigurationChanged, this, &InstrumentTrack::updatePitchRange);
	connect(&m_midiPort, &MidiPort::MPEConfigurationChanged, this, &InstrumentTrack::updatePitch);

	autoAssignMidiDevice(true);
}



bool InstrumentTrack::keyRangeImport() const
{
	return m_microtuner.enabled() && m_microtuner.keyRangeImport();
}


/** \brief Check if there is a valid mapping for the given key and it is within defined of range.
 */
bool InstrumentTrack::isKeyMapped(int physicalKey) const
{
	if (physicalKey < firstKey() || physicalKey > lastKey()) {return false;}
	if (!m_microtuner.enabled()) {return true;}

	Song *song = Engine::getSong();
	if (!song) {return false;}

	return song->getKeymap(m_microtuner.currentKeymap())->getDegree(physicalKey) != -1;
}


/** \brief Return first mapped key, based on currently selected keymap or user selection.
 *	\return Number ranging from 0 to NumKeys -1
 */
int InstrumentTrack::firstKey() const
{
	if (keyRangeImport())
	{
		return Engine::getSong()->getKeymap(m_microtuner.currentKeymap())->getFirstKey();
	}
	else
	{
		return m_firstKeyModel.value();
	}
}


/** \brief Return last mapped key, based on currently selected keymap or user selection.
 *	\return Number ranging from 0 to NumKeys -1
 */
int InstrumentTrack::lastKey() const
{
	if (keyRangeImport())
	{
		return Engine::getSong()->getKeymap(m_microtuner.currentKeymap())->getLastKey();
	}
	else
	{
		return m_lastKeyModel.value();
	}
}


/** \brief Return base key number, based on currently selected keymap or user selection.
 *	\return Number ranging from 0 to NumKeys -1
 */
int InstrumentTrack::baseNote() const
{
	if (keyRangeImport())
	{
		return Engine::getSong()->getKeymap(m_microtuner.currentKeymap())->getBaseKey();
	}
	else
	{
		return m_baseNoteModel.value();
	}
}


/** \brief Return frequency assigned to the base key, based on currently selected keymap.
 *	\return Frequency in Hz
 */
float InstrumentTrack::baseFreq() const
{
	if (m_microtuner.enabled())
	{
		return Engine::getSong()->getKeymap(m_microtuner.currentKeymap())->getBaseFreq();
	}
	else
	{
		return DefaultBaseFreq;
	}
}



InstrumentTrack::~InstrumentTrack()
{
	// De-assign midi device
	if (m_hasAutoMidiDev)
	{
		autoAssignMidiDevice(false);
		s_autoAssignedTrack = nullptr;
	}

	// kill all running notes and the iph
	silenceAllNotes( true );

	// now we're save deleting the instrument
	if( m_instrument ) delete m_instrument;
}




void InstrumentTrack::processAudioBuffer( SampleFrame* buf, const fpp_t frames, NotePlayHandle* n )
{
	// we must not play the sound if this InstrumentTrack is muted...
	if( isMuted() || ( Engine::getSong()->playMode() != Song::PlayMode::MidiClip &&
				n && n->isPatternTrackMuted() ) || ! m_instrument )
	{
		return;
	}

	// Test for silent input data if instrument provides a single stream only (i.e. driven by InstrumentPlayHandle)
	// We could do that in all other cases as well but the overhead for silence test is bigger than
	// what we potentially save. While playing a note, a NotePlayHandle-driven instrument will produce sound in
	// 99 of 100 cases so that test would be a waste of time.
	if (m_instrument->isSingleStreamed() && MixHelpers::isSilent(buf, frames))
	{
		// at least pass one silent buffer to allow
		if( m_silentBuffersProcessed )
		{
			// skip further processing
			return;
		}
		m_silentBuffersProcessed = true;
	}
	else
	{
		m_silentBuffersProcessed = false;
	}

	// if effects "went to sleep" because there was no input, wake them up
	// now
	m_audioBusHandle.effects()->startRunning();

	// get volume knob data
	static const float DefaultVolumeRatio = 1.0f / DefaultVolume;
	/*ValueBuffer * volBuf = m_volumeModel.valueBuffer();
	float v_scale = volBuf
		? 1.0f
		: getVolume() * DefaultVolumeRatio;*/

	// instruments using instrument-play-handles will call this method
	// without any knowledge about notes, so they pass NULL for n, which
	// is no problem for us since we just bypass the envelopes+LFOs
	if (!m_instrument->isSingleStreamed() && n != nullptr)
	{
		const f_cnt_t offset = n->noteOffset();
		m_soundShaping.processAudioBuffer( buf + offset, frames - offset, n );
		const float vol = ( (float) n->getVolume() * DefaultVolumeRatio );
		const panning_t pan = std::clamp(n->getPanning(), PanningLeft, PanningRight);
		StereoVolumeVector vv = panningToVolumeVector( pan, vol );
		for( f_cnt_t f = offset; f < frames; ++f )
		{
			for( int c = 0; c < 2; ++c )
			{
				buf[f][c] *= vv.vol[c];
			}
		}
	}
}






void InstrumentTrack::processCCEvent(int controller)
{
	// Does nothing if the LED is disabled
	if (!m_midiCCEnable->value()) { return; }

	auto channel = static_cast<uint8_t>(midiPort()->realOutputChannel());
	auto cc = static_cast<uint16_t>(controller);
	auto value = static_cast<uint16_t>(m_midiCCModel[controller]->value());

	// Process the MIDI CC event as an input event but with source set to Internal
	// so we can know LMMS generated the event, not a controller, and can process it during
	// the project export
	processInEvent(MidiEvent(MidiControlChange, channel, cc, value, nullptr, MidiEvent::Source::Internal));
}




void InstrumentTrack::processInEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset)
{
	if (Engine::getSong()->isExporting() && event.source() == MidiEvent::Source::External)
	{
		return;
	}

	const int physicalKey = event.key();
	const int transposedKey = physicalKey + transposeAmount();

	bool eventHandled = false;

	switch (event.type())
	{
		// we don't send MidiNoteOn, MidiNoteOff and MidiKeyPressure
		// events to instrument as NotePlayHandle will send them on its
		// own
		case MidiNoteOn:
			if (event.velocity() > 0)
			{
				// play a note only if it is not already playing and if it is within configured bounds
				if (m_notes[physicalKey] == nullptr && physicalKey >= firstKey() && physicalKey <= lastKey())
				{
					NotePlayHandle* nph =
						NotePlayHandleManager::acquire(
								this, offset,
								std::numeric_limits<f_cnt_t>::max() / 2,
								Note(TimePos(), Engine::getSong()->getPlayPos(Engine::getSong()->playMode()),
										transposedKey, event.volume(midiPort()->baseVelocity())),
								nullptr, event.channel(),
								NotePlayHandle::Origin::MidiInput);
					m_notes[physicalKey] = nph;
					if(!Engine::audioEngine()->addPlayHandle(nph))
					{
						m_notes[physicalKey] = nullptr;
					}
				}
				eventHandled = true;
				break;
			}

		case MidiNoteOff:
			if (m_notes[physicalKey] != nullptr)
			{
				// do actual note off and remove internal reference to NotePlayHandle (which itself will
				// be deleted later automatically)
				Engine::audioEngine()->requestChangeInModel();
				m_notes[physicalKey]->noteOff(offset);
				if (isSustainPedalPressed() && m_notes[physicalKey]->origin() == NotePlayHandle::Origin::MidiInput)
				{
					m_sustainedNotes << m_notes[physicalKey];
				}
				m_notes[physicalKey] = nullptr;
				Engine::audioEngine()->doneChangeInModel();
			}
			eventHandled = true;
			break;

		case MidiKeyPressure:
			if (m_notes[physicalKey] != nullptr)
			{
				// setVolume() calls processOutEvent() with MidiKeyPressure so the
				// attached instrument will receive the event as well
				m_notes[physicalKey]->setVolume(event.volume(midiPort()->baseVelocity()));
			}
			eventHandled = true;
			break;

		case MidiPitchBend:
			// updatePitch() is connected to m_pitchModel::dataChanged() which will send out
			// MidiPitchBend events
			m_pitchModel.setValue(m_pitchModel.minValue() + event.pitchBend() * m_pitchModel.range() / MidiMaxPitchBend);
			break;

		case MidiControlChange:
			if (event.controllerNumber() == MidiControllerSustain)
			{
				if (event.controllerValue() > MidiMaxControllerValue / 2)
				{
					m_sustainPedalPressed = true;
				}
				else if (isSustainPedalPressed())
				{
					for (NotePlayHandle* nph : m_sustainedNotes)
					{
						if (nph && nph->isReleased())
						{
							if( nph->origin() ==
								NotePlayHandle::Origin::MidiInput)
							{
								nph->setLength(
									TimePos( static_cast<f_cnt_t>(
									nph->totalFramesPlayed() /
									Engine::framesPerTick() ) ) );
								midiNoteOff( *nph );
							}
						}
					}
					m_sustainedNotes.clear();
					m_sustainPedalPressed = false;
				}
			}
			if( event.controllerNumber() == MidiControllerAllSoundOff ||
				event.controllerNumber() == MidiControllerAllNotesOff ||
				event.controllerNumber() == MidiControllerOmniOn ||
				event.controllerNumber() == MidiControllerOmniOff ||
				event.controllerNumber() == MidiControllerMonoOn ||
				event.controllerNumber() == MidiControllerPolyOn )
			{
				silenceAllNotes();
			}
			break;

		case MidiMetaEvent:
			// handle special cases such as note panning
			switch (event.metaEvent())
			{
				case MidiNotePanning:
					if (m_notes[physicalKey] != nullptr)
					{
						eventHandled = true;
						m_notes[physicalKey]->setPanning(event.panning());
					}
					break;
				default:
					qWarning("InstrumentTrack: unhandled MIDI meta event: %i", event.metaEvent());
					break;
			}
			break;

		default:
			break;
	}

	// If the event wasn't handled, check if there's a loaded instrument and if so send the
	// event to it. If it returns false means the instrument didn't handle the event, so we trigger a warning.
	if (eventHandled == false && !(instrument() && instrument()->handleMidiEvent(event, time, offset)))
	{
		qWarning("InstrumentTrack: unhandled MIDI event %d", event.type());
	}
}




void InstrumentTrack::processOutEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset)
{
	// Do nothing if we do not have an instrument instance (e.g. when loading settings)
	if (m_instrument == nullptr) { return; }

	const int channel = event.channel();
	const int key = event.key();

	// Before passing the event to the plugin, do some checks to make sure there isn't a note already on.
	// Lock the mutex to ensure the checks and handling are not interleaved with other threads.
	QMutexLocker lock(&m_midiOutputMutex);
	switch (event.type())
	{
		case MidiNoteOn:
			if (key < 0 || key >= NumKeys) { return; }
			// If there's already another note playing on this key, cut it off before this one starts.
			if (m_runningMidiNotes[channel][key] > 0)
			{
				m_instrument->handleMidiEvent(MidiEvent(MidiNoteOff, channel, key, 0), time, offset);
			}
			m_runningMidiNotes[channel][key]++;
			// Update the track activity indicator
			emit newNote();
			break;
		case MidiNoteOff:
			if (key < 0 || key >= NumKeys) { return; }
			m_runningMidiNotes[channel][key]--;
			// Don't send a note off signal unless this is the only note on this key--we don't want to cut off any other notes currently playing.
			if (m_runningMidiNotes[channel][key] > 0) { return; }
			// Update the track activity indicator
			emit endNote();
			break;
		default:
			break;
	}


	// Now for real, pass the event to the instrument
	m_instrument->handleMidiEvent(event, time, offset);

	// If appropriate, midi-port can do futher routing
	m_midiPort.processOutEvent(event, time);

	// And send the event to the piano widget to update the pressed keys
	m_piano.processInEvent(event, time, offset);
}




void InstrumentTrack::silenceAllNotes( bool removeIPH )
{
	Engine::audioEngine()->requestChangeInModel();
	// invalidate all NotePlayHandles and PresetPreviewHandles linked to this track
	m_processHandles.clear();

	auto flags = PlayHandle::Type::NotePlayHandle | PlayHandle::Type::PresetPreviewHandle;
	if( removeIPH )
	{
		flags |= PlayHandle::Type::InstrumentPlayHandle;
	}
	Engine::audioEngine()->removePlayHandlesOfTypes( this, flags );
	Engine::audioEngine()->doneChangeInModel();

	// The active note counts must be reset AFTER all NotePlayHandles have been destructed, since by default they also decrement the counter when they noteOff.
	m_midiOutputMutex.lock();
	for( int i = 0; i < NumKeys; ++i )
	{
		m_notes[i] = nullptr;
		for (int channel = 0; channel < 16; ++channel)
		{
			m_runningMidiNotes[channel][i] = 0;
		}
	}
	m_midiOutputMutex.unlock();
}




f_cnt_t InstrumentTrack::beatLen( NotePlayHandle * _n ) const
{
	if( m_instrument != nullptr )
	{
		const f_cnt_t len = m_instrument->beatLen( _n );
		if( len > 0 )
		{
			return len;
		}
	}
	return m_soundShaping.envFrames();
}




void InstrumentTrack::playNote( NotePlayHandle* n, SampleFrame* workingBuffer )
{
	// Note: under certain circumstances the working buffer is a nullptr.
	// These cases are triggered in PlayHandle::doProcessing when the play method is called with a nullptr.
	// TODO: Find out if we can skip processing at a higher level if the buffer is nullptr.

	// arpeggio- and chord-widget has to do its work -> adding sub-notes
	// for chords/arpeggios
	m_noteStacking.processNote( n );
	m_arpeggio.processNote( n );

	if( n->isMasterNote() == false && m_instrument != nullptr )
	{
		// all is done, so now lets play the note!
		m_instrument->playNote( n, workingBuffer );

		// This is effectively the same as checking if workingBuffer is not a nullptr.
		// Calling processAudioBuffer with a nullptr leads to crashes. Hence the check.
		if (n->usesBuffer())
		{
			const fpp_t frames = n->framesLeftForCurrentPeriod();
			const f_cnt_t offset = n->noteOffset();
			processAudioBuffer(workingBuffer, frames + offset, n);
		}
	}
}




QString InstrumentTrack::instrumentName() const
{
	if( m_instrument != nullptr )
	{
		return m_instrument->displayName();
	}
	return QString();
}




void InstrumentTrack::deleteNotePluginData( NotePlayHandle* n )
{
	if( m_instrument != nullptr )
	{
		m_instrument->deleteNotePluginData( n );
	}
}




void InstrumentTrack::setName(const QString& new_name)
{
	Track::setName(new_name);
	m_midiPort.setName(name());
	m_audioBusHandle.setName(name());
}






void InstrumentTrack::updateBaseNote()
{
	Engine::audioEngine()->requestChangeInModel();
	for (const auto& processHandle : m_processHandles)
	{
		processHandle->setFrequencyUpdate();
	}
	Engine::audioEngine()->doneChangeInModel();
}




void InstrumentTrack::updatePitch()
{
	updateBaseNote();

	int channel = midiPort()->MPEEnabled()
		? midiPort()->mpeManager()->managerChannel()
		: midiPort()->realOutputChannel(); // TODO this sends the signal on the output channel, regardless of whether midi output is enabled or not.

	processOutEvent(MidiEvent(MidiPitchBend, channel, midiPitch()));
}




void InstrumentTrack::updatePitchRange()
{
	const int r = m_pitchRangeModel.value();
	m_pitchModel.setRange( MinPitchDefault * r, MaxPitchDefault * r );

	int channel = midiPort()->MPEEnabled()
		? midiPort()->mpeManager()->managerChannel()
		: midiPort()->realOutputChannel(); // TODO this sends the signal on the output channel, regardless of whether midi output is enabled or not.

	processOutEvent(MidiEvent(MidiControlChange, channel,
								MidiControllerRegisteredParameterNumberLSB, MidiPitchBendSensitivityRPN & 0x7F));
	processOutEvent(MidiEvent(MidiControlChange, channel,
								MidiControllerRegisteredParameterNumberMSB, (MidiPitchBendSensitivityRPN >> 8) & 0x7F));
	processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerDataEntry, midiPitchRange()));
}




void InstrumentTrack::updateMixerChannel()
{
	m_audioBusHandle.setNextMixerChannel(m_mixerChannelModel.value());
}




/** \brief Returns the total transposition offset applied to outgoing notes
 * Accounts for transposition due to both the base note and the master pitch/transposition slider.
*/
int InstrumentTrack::transposeAmount() const
{
	int mp = m_useMasterPitchModel.value() ? Engine::getSong()->masterPitch() : 0;
	return DefaultBaseKey - baseNote() + mp;
}




void InstrumentTrack::removeMidiPortNode( DataFile & _dataFile )
{
	QDomNodeList n = _dataFile.elementsByTagName( "midiport" );
	n.item( 0 ).parentNode().removeChild( n.item( 0 ) );
}




bool InstrumentTrack::play( const TimePos & _start, const fpp_t _frames,
							const f_cnt_t _offset, int _clip_num )
{
	if( ! m_instrument || ! tryLock() )
	{
		return false;
	}
	const float frames_per_tick = Engine::framesPerTick();

	clipVector clips;
	class PatternTrack * pattern_track = nullptr;
	if( _clip_num >= 0 )
	{
		Clip * clip = getClip( _clip_num );
		clips.push_back( clip );
		if (trackContainer() == Engine::patternStore())
		{
			pattern_track = PatternTrack::findPatternTrack(_clip_num);
		}
	}
	else
	{
		getClipsInRange( clips, _start, _start + static_cast<int>(
					_frames / frames_per_tick ) );
	}

	// Handle automation: detuning
	for (const auto& processHandle : m_processHandles)
	{
		processHandle->processTimePos(
			_start, m_pitchModel.value(), gui::getGUI() && gui::getGUI()->pianoRoll()->isRecording());
	}

	if ( clips.size() == 0 )
	{
		unlock();
		return false;
	}

	bool played_a_note = false;	// will be return variable

	for (const auto& clip : clips)
	{
		auto c = dynamic_cast<MidiClip*>(clip);
		// everything which is not a MIDI clip won't be played
		// A MIDI clip playing in the Piano Roll window will always play
		if (c == nullptr || (Engine::getSong()->playMode() != Song::PlayMode::MidiClip && clip->isMuted()))
		{
			continue;
		}
		TimePos cur_start = _start;
		if( _clip_num < 0 )
		{
			cur_start -= c->startPosition() + c->startTimeOffset();
		}

		// get all notes from the given clip...
		const NoteVector & notes = c->notes();
		// ...and set our index to zero
		auto nit = notes.begin();

		// very effective algorithm for playing notes that are
		// posated within the current sample-frame

		if( cur_start > 0 )
		{
			// skip notes which end before start-bar
			while( nit != notes.end() && ( *nit )->endPos() < cur_start )
			{
				++nit;
			}
		}

		while (nit != notes.end() && (*nit)->pos() < c->length() - c->startTimeOffset())
		{
			const auto currentNote = *nit;
			// Skip any notes note at the current time pos or not overlapping with the start.
			if (!(currentNote->pos() == cur_start
				|| (cur_start == -c->startTimeOffset() && (*nit)->pos() < cur_start && (*nit)->endPos() > cur_start)))
			{
				++nit;
				continue;
			}

			// Calculate the overlap of the note over the clip end.
			const auto noteOverlap = std::max(0, currentNote->endPos() - (c->length() - c->startTimeOffset()));
			// If the note is a Step Note, frames will be 0 so the NotePlayHandle
			// plays for the whole length of the sample
			const auto noteFrames = currentNote->type() == Note::Type::Step
				? 0
				: (currentNote->endPos() - cur_start - noteOverlap) * frames_per_tick;

			const int physicalKey = currentNote->key();
			const int transposedKey = physicalKey + transposeAmount();
			// Create a shallow copy of the note to apply the transposition (do not deep copy the detuning clip)
			Note noteCopy = *currentNote;
			noteCopy.setKey(transposedKey);

			NotePlayHandle* notePlayHandle = NotePlayHandleManager::acquire(this, _offset, noteFrames, noteCopy);
			notePlayHandle->setPatternTrack(pattern_track);
			// are we playing global song?
			if( _clip_num < 0 )
			{
				// then set song-global offset of clip in order to
				// properly perform the note detuning
				notePlayHandle->setSongGlobalParentOffset( c->startPosition() + c->startTimeOffset());
			}

			Engine::audioEngine()->addPlayHandle( notePlayHandle );
			played_a_note = true;
			++nit;
		}
	}
	unlock();
	return played_a_note;
}




Clip* InstrumentTrack::createClip(const TimePos & pos)
{
	auto p = new MidiClip(this);
	p->movePosition(pos);
	return p;
}




gui::TrackView* InstrumentTrack::createView( gui::TrackContainerView* tcv )
{
	return new gui::InstrumentTrackView( this, tcv );
}




void InstrumentTrack::saveTrackSpecificSettings(QDomDocument& doc, QDomElement& thisElement, bool presetMode)
{
	m_volumeModel.saveSettings( doc, thisElement, "vol" );
	m_panningModel.saveSettings( doc, thisElement, "pan" );
	m_pitchModel.saveSettings( doc, thisElement, "pitch" );
	m_pitchRangeModel.saveSettings( doc, thisElement, "pitchrange" );

	m_mixerChannelModel.saveSettings( doc, thisElement, "mixch" );
	m_baseNoteModel.saveSettings( doc, thisElement, "basenote" );
	m_firstKeyModel.saveSettings(doc, thisElement, "firstkey");
	m_lastKeyModel.saveSettings(doc, thisElement, "lastkey");
	m_useMasterPitchModel.saveSettings( doc, thisElement, "usemasterpitch");
	m_microtuner.saveSettings(doc, thisElement);

	// Save MIDI CC stuff
	m_midiCCEnable->saveSettings(doc, thisElement, "enablecc");
	QDomElement midiCC = doc.createElement("midicontrollers");
	thisElement.appendChild(midiCC);
	for (int i = 0; i < MidiControllerCount; ++i)
	{
		m_midiCCModel[i]->saveSettings(doc, midiCC, "cc" + QString::number(i));
	}

	if( m_instrument != nullptr )
	{
		QDomElement i = doc.createElement( "instrument" );
		i.setAttribute( "name", m_instrument->descriptor()->name );
		QDomElement ins = m_instrument->saveState( doc, i );
		if(m_instrument->key().isValid()) {
			ins.appendChild( m_instrument->key().saveXML( doc ) );
		}
		thisElement.appendChild( i );
	}
	m_soundShaping.saveState( doc, thisElement );
	m_noteStacking.saveState( doc, thisElement );
	m_arpeggio.saveState( doc, thisElement );

	// Save the midi port info if we are not in song saving mode, e.g. in
	// track cloning mode or if we are in song saving mode and the user
	// has chosen not to discard the MIDI connections.
	if (!Engine::getSong()->isSavingProject() ||
	    !Engine::getSong()->getSaveOptions().discardMIDIConnections.value())
	{
		// Don't save auto assigned midi device connection
		bool hasAuto = m_hasAutoMidiDev;
		autoAssignMidiDevice(false);

		// Only save the MIDI port information if we are not saving a preset.
		if (!presetMode)
		{
			m_midiPort.saveState(doc, thisElement);
		}

		autoAssignMidiDevice(hasAuto);
	}

	m_audioBusHandle.effects()->saveState(doc, thisElement);
}




void InstrumentTrack::loadTrackSpecificSettings( const QDomElement & thisElement )
{
	// don't delete instrument in preview mode if it's the same
	// we can't do this for other situations due to some issues with linked models
	bool reuseInstrument = m_previewMode && m_instrument && m_instrument->nodeName() == getSavedInstrumentName(thisElement);
	// remove the InstrumentPlayHandle if and only if we need to delete the instrument
	silenceAllNotes(!reuseInstrument);

	lock();

	m_volumeModel.loadSettings( thisElement, "vol" );
	m_panningModel.loadSettings( thisElement, "pan" );
	m_pitchRangeModel.loadSettings( thisElement, "pitchrange" );
	m_pitchModel.loadSettings( thisElement, "pitch" );
	m_mixerChannelModel.setRange( 0, Engine::mixer()->numChannels()-1 );
	if ( !m_previewMode )
	{
		m_mixerChannelModel.loadSettings( thisElement, "mixch" );
	}
	m_baseNoteModel.loadSettings( thisElement, "basenote" );
	m_firstKeyModel.loadSettings(thisElement, "firstkey");
	m_lastKeyModel.loadSettings(thisElement, "lastkey");
	m_useMasterPitchModel.loadSettings( thisElement, "usemasterpitch");
	m_microtuner.loadSettings(thisElement);

	// clear effect-chain just in case we load an old preset without FX-data
	m_audioBusHandle.effects()->clear();

	// We set MIDI CC enable to false so the knobs don't trigger MIDI CC events while
	// they are being loaded. After all knobs are loaded we load the right value of m_midiCCEnable.
	m_midiCCEnable->setValue(false);

	QDomNode node = thisElement.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( m_soundShaping.nodeName() == node.nodeName() )
			{
				m_soundShaping.restoreState( node.toElement() );
			}
			else if( m_noteStacking.nodeName() == node.nodeName() )
			{
				m_noteStacking.restoreState( node.toElement() );
			}
			else if( m_arpeggio.nodeName() == node.nodeName() )
			{
				m_arpeggio.restoreState( node.toElement() );
			}
			else if( m_midiPort.nodeName() == node.nodeName() )
			{
				m_midiPort.restoreState( node.toElement() );
			}
			else if (m_audioBusHandle.effects()->nodeName() == node.nodeName())
			{
				m_audioBusHandle.effects()->restoreState(node.toElement());
			}
			else if(node.nodeName() == "instrument")
			{
				using PluginKey = Plugin::Descriptor::SubPluginFeatures::Key;
				PluginKey key(node.toElement().elementsByTagName("key").item(0).toElement());

				if (reuseInstrument)
				{
					m_instrument->restoreState(node.firstChildElement());
				}
				else
				{
					delete m_instrument;
					m_instrument = nullptr;
					m_instrument = Instrument::instantiate(
						node.toElement().attribute("name"), this, &key);
					m_instrument->restoreState(node.firstChildElement());
					emit instrumentChanged();
				}
			}
			else if (node.nodeName() == "midicontrollers")
			{
				for (int i = 0; i < MidiControllerCount; ++i)
				{
					m_midiCCModel[i]->loadSettings(node.toElement(), "cc" + QString::number(i));
				}
			}
			// compat code - if node-name doesn't match any known
			// one, we assume that it is an instrument-plugin
			// which we'll try to load
			else if(AutomationClip::classNodeName() != node.nodeName() &&
					ControllerConnection::classNodeName() != node.nodeName() &&
					!node.toElement().hasAttribute( "id" ))
			{
				delete m_instrument;
				m_instrument = nullptr;
				m_instrument = Instrument::instantiate(
					node.nodeName(), this, nullptr, true);
				if (m_instrument->nodeName() == node.nodeName())
				{
					m_instrument->restoreState(node.toElement());
				}
				emit instrumentChanged();
			}
		}
		node = node.nextSibling();
	}

	// Load the right value of m_midiCCEnable
	m_midiCCEnable->loadSettings(thisElement, "enablecc");

	updatePitchRange();
	unlock();
}




void InstrumentTrack::setPreviewMode( const bool value )
{
	m_previewMode = value;
}




void InstrumentTrack::replaceInstrument(DataFile dataFile)
{
	// loadSettings clears the mixer channel, so we save it here and set it back later
	int mixerChannel = mixerChannelModel()->value();

	InstrumentTrack::removeMidiPortNode(dataFile);
	
	//Replacing an instrument shouldn't change the solo/mute state.
	bool oldMute = isMuted();
	bool oldSolo = isSolo();
	bool oldMutedBeforeSolo = isMutedBeforeSolo();

	loadPreset(dataFile.content().toElement());
	
	setMuted(oldMute);
	setSolo(oldSolo);
	setMutedBeforeSolo(oldMutedBeforeSolo);
	
	m_mixerChannelModel.setValue(mixerChannel);
	Engine::getSong()->setModified();
}




QString InstrumentTrack::getSavedInstrumentName(const QDomElement &thisElement) const
{
	QDomElement elem = thisElement.firstChildElement("instrument");
	if (!elem.isNull())
	{
		return elem.attribute("name");
	}
	return "";
}




Instrument * InstrumentTrack::loadInstrument(const QString & _plugin_name,
	const Plugin::Descriptor::SubPluginFeatures::Key *key, bool keyFromDnd)
{
	if(keyFromDnd)
		Q_ASSERT(!key);

	silenceAllNotes( true );

	lock();
	delete m_instrument;
	m_instrument = Instrument::instantiate(_plugin_name, this,
					key, keyFromDnd);
	unlock();
	setName(m_instrument->displayName());

	emit instrumentChanged();

	return m_instrument;
}



InstrumentTrack *InstrumentTrack::s_autoAssignedTrack = nullptr;

/*! \brief Automatically assign a midi controller to this track, based on the midiautoassign setting
 *
 *  \param assign set to true to connect the midi device, set to false to disconnect
 */
void InstrumentTrack::autoAssignMidiDevice(bool assign)
{
	if (assign)
	{
		if (s_autoAssignedTrack)
		{
			s_autoAssignedTrack->autoAssignMidiDevice(false);
		}
		s_autoAssignedTrack = this;
	}

	const QString &device = ConfigManager::inst()->value("midi", "midiautoassign");
	if ( Engine::audioEngine()->midiClient()->isRaw() && device != "none" )
	{
		m_midiPort.setReadable( assign );
		return;
	}

	// Check if the device exists
	if ( Engine::audioEngine()->midiClient()->readablePorts().indexOf(device) >= 0 )
	{
		m_midiPort.subscribeReadablePort(device, assign);
		m_hasAutoMidiDev = assign;
	}
}


} // namespace lmms
