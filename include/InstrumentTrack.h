/*
 * InstrumentTrack.h - declaration of class InstrumentTrack, a track which
 *                     holds an instrument-plugin
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

#ifndef LMMS_INSTRUMENT_TRACK_H
#define LMMS_INSTRUMENT_TRACK_H


#include "AudioBusHandle.h"
#include "InstrumentFunctions.h"
#include "InstrumentSoundShaping.h"
#include "Microtuner.h"
#include "Midi.h"
#include "MidiEventProcessor.h"
#include "MidiPort.h"
#include "NotePlayHandle.h"
#include "Piano.h"
#include "Plugin.h"
#include "Track.h"


namespace lmms
{


class Instrument;
class DataFile;

namespace gui
{

class InstrumentTrackView;
class InstrumentTrackWindow;
class InstrumentTuningView;
class MidiCCRackView;

} // namespace gui


class LMMS_EXPORT InstrumentTrack : public Track, public MidiEventProcessor
{
	Q_OBJECT
	mapPropertyFromModel(int,getVolume,setVolume,m_volumeModel);
public:
	InstrumentTrack( TrackContainer* tc );
	~InstrumentTrack() override;

	// used by instrument
	void processAudioBuffer( SampleFrame* _buf, const fpp_t _frames,
							NotePlayHandle * _n );

	MidiEvent applyMasterKey( const MidiEvent& event );

	void processInEvent( const MidiEvent& event, const TimePos& time = TimePos(), f_cnt_t offset = 0 ) override;
	void processOutEvent( const MidiEvent& event, const TimePos& time = TimePos(), f_cnt_t offset = 0 ) override;
	// silence all running notes played by this track
	void silenceAllNotes( bool removeIPH = false );

	bool isSustainPedalPressed() const
	{
		return m_sustainPedalPressed;
	}

	f_cnt_t beatLen( NotePlayHandle * _n ) const;


	// for capturing note-play-events -> need that for arpeggio,
	// filter and so on
	void playNote( NotePlayHandle * _n, SampleFrame* _working_buffer );

	QString instrumentName() const;
	const Instrument *instrument() const
	{
		return m_instrument;
	}

	Instrument *instrument()
	{
		return m_instrument;
	}

	void deleteNotePluginData( NotePlayHandle * _n );

	// name-stuff
	void setName( const QString & _new_name ) override;

	// translate given key of a note-event to absolute key (i.e.
	// add global master-pitch and base-note of this instrument track)
	int masterKey( int _midi_key ) const;

	// translate pitch to midi-pitch [0,16383]
	int midiPitch() const
	{
		return static_cast<int>( ( ( m_pitchModel.value() + m_pitchModel.range()/2 ) * MidiMaxPitchBend ) / m_pitchModel.range() );
	}

	/*! \brief Returns current range for pitch bend in semitones */
	int midiPitchRange() const
	{
		return m_pitchRangeModel.value();
	}

	// play everything in given frame-range - creates note-play-handles
	bool play( const TimePos & _start, const fpp_t _frames,
						const f_cnt_t _frame_base, int _clip_num = -1 ) override;
	// create new view for me
	gui::TrackView* createView( gui::TrackContainerView* tcv ) override;

	// create new track-content-object = clip
	Clip* createClip(const TimePos & pos) override;


	// called by track
	void saveTrackSpecificSettings(QDomDocument& doc, QDomElement& parent, bool presetMode) override;
	void loadTrackSpecificSettings( const QDomElement & _this ) override;

	using Track::setJournalling;


	// load instrument whose name matches given one
	Instrument * loadInstrument(const QString & _instrument_name,
				const Plugin::Descriptor::SubPluginFeatures::Key* key = nullptr,
				bool keyFromDnd = false);

	AudioBusHandle* audioBusHandle()
	{
		return &m_audioBusHandle;
	}

	MidiPort * midiPort()
	{
		return &m_midiPort;
	}

	const IntModel *baseNoteModel() const
	{
		return &m_baseNoteModel;
	}

	IntModel *baseNoteModel()
	{
		return &m_baseNoteModel;
	}

	IntModel *firstKeyModel()
	{
		return &m_firstKeyModel;
	}

	IntModel *lastKeyModel()
	{
		return &m_lastKeyModel;
	}

	bool keyRangeImport() const;
	bool isKeyMapped(int key) const;
	int firstKey() const;
	int lastKey() const;
	int baseNote() const;
	float baseFreq() const;

	Piano *pianoModel()
	{
		return &m_piano;
	}

	Microtuner *microtuner()
	{
		return &m_microtuner;
	}

	bool isArpeggioEnabled() const
	{
		return m_arpeggio.m_arpEnabledModel.value();
	}

	// simple helper for removing midiport-XML-node when loading presets
	static void removeMidiPortNode( DataFile& dataFile );

	FloatModel * pitchModel()
	{
		return &m_pitchModel;
	}

	FloatModel * volumeModel()
	{
		return &m_volumeModel;
	}

	FloatModel * panningModel()
	{
		return &m_panningModel;
	}

	IntModel* pitchRangeModel()
	{
		return &m_pitchRangeModel;
	}

	IntModel * mixerChannelModel()
	{
		return &m_mixerChannelModel;
	}

	BoolModel* useMasterPitchModel()
	{
		return &m_useMasterPitchModel;
	}

	void setPreviewMode( const bool );

	bool isPreviewMode() const
	{
		return m_previewMode;
	}
	
	void replaceInstrument(DataFile dataFile);

	void autoAssignMidiDevice( bool );

signals:
	void instrumentChanged();
	void midiNoteOn( const lmms::Note& );
	void midiNoteOff( const lmms::Note& );
	void newNote();
	void endNote();

protected:
	QString nodeName() const override
	{
		return "instrumenttrack";
	}

	// get the name of the instrument in the saved data
	QString getSavedInstrumentName(const QDomElement & thisElement) const;


protected slots:
	void updateBaseNote();
	void updatePitch();
	void updatePitchRange();
	void updateMixerChannel();


private:
	void processCCEvent(int controller);

	MidiPort m_midiPort;

	NotePlayHandle* m_notes[NumKeys];
	NotePlayHandleList m_sustainedNotes;

	int m_runningMidiNotes[NumKeys];
	QMutex m_midiNotesMutex;

	bool m_sustainPedalPressed;

	bool m_silentBuffersProcessed;

	bool m_previewMode;

	IntModel m_baseNoteModel;	//!< The "A4" or "440 Hz" key (default 69)
	IntModel m_firstKeyModel;	//!< First key the instrument reacts to
	IntModel m_lastKeyModel;	//!< Last key the instrument reacts to

	bool m_hasAutoMidiDev;
	static InstrumentTrack *s_autoAssignedTrack;

	NotePlayHandleList m_processHandles;

	FloatModel m_volumeModel;
	FloatModel m_panningModel;

	AudioBusHandle m_audioBusHandle;

	FloatModel m_pitchModel;
	IntModel m_pitchRangeModel;
	IntModel m_mixerChannelModel;
	BoolModel m_useMasterPitchModel;

	Instrument * m_instrument;
	InstrumentSoundShaping m_soundShaping;
	InstrumentFunctionArpeggio m_arpeggio;
	InstrumentFunctionNoteStacking m_noteStacking;

	Piano m_piano;

	Microtuner m_microtuner;

	std::unique_ptr<BoolModel> m_midiCCEnable;
	std::unique_ptr<FloatModel> m_midiCCModel[MidiControllerCount];

	friend class gui::InstrumentTrackView;
	friend class gui::InstrumentTrackWindow;
	friend class NotePlayHandle;
	friend class gui::InstrumentTuningView;
	friend class gui::MidiCCRackView;

} ;



} // namespace lmms

#endif // LMMS_INSTRUMENT_TRACK_H
