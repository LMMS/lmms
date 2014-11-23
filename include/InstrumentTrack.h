/*
 * InstrumentTrack.h - declaration of class InstrumentTrack, a track + window
 *                     which holds an instrument-plugin
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef INSTRUMENT_TRACK_H
#define INSTRUMENT_TRACK_H

#include "AudioPort.h"
#include "InstrumentFunctions.h"
#include "InstrumentSoundShaping.h"
#include "MidiEventProcessor.h"
#include "MidiPort.h"
#include "NotePlayHandle.h"
#include "Piano.h"
#include "PianoView.h"
#include "Pitch.h"
#include "track.h"


class QLineEdit;
template<class T> class QQueue;
class InstrumentFunctionArpeggioView;
class InstrumentFunctionNoteStackingView;
class EffectRackView;
class InstrumentSoundShapingView;
class fadeButton;
class Instrument;
class InstrumentTrackWindow;
class InstrumentMidiIOView;
class knob;
class LcdSpinBox;
class midiPortMenu;
class DataFile;
class PluginView;
class tabWidget;
class trackLabelButton;


class EXPORT InstrumentTrack : public track, public MidiEventProcessor
{
	Q_OBJECT
	mapPropertyFromModel(int,getVolume,setVolume,m_volumeModel);
public:
	InstrumentTrack( TrackContainer* tc );
	virtual ~InstrumentTrack();

	// used by instrument
	void processAudioBuffer( sampleFrame * _buf, const fpp_t _frames,
							NotePlayHandle * _n );

	MidiEvent applyMasterKey( const MidiEvent& event );

	virtual void processInEvent( const MidiEvent& event, const MidiTime& time = MidiTime(), f_cnt_t offset = 0 );
	virtual void processOutEvent( const MidiEvent& event, const MidiTime& time = MidiTime(), f_cnt_t offset = 0 );
	// silence all running notes played by this track
	void silenceAllNotes( bool removeIPH = false );

	bool isSustainPedalPressed() const
	{
		return m_sustainPedalPressed;
	}

	f_cnt_t beatLen( NotePlayHandle * _n ) const;


	// for capturing note-play-events -> need that for arpeggio,
	// filter and so on
	void playNote( NotePlayHandle * _n, sampleFrame * _working_buffer );

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
	virtual void setName( const QString & _new_name );

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
	virtual bool play( const MidiTime & _start, const fpp_t _frames,
						const f_cnt_t _frame_base, int _tco_num = -1 );
	// create new view for me
	virtual trackView * createView( TrackContainerView* tcv );

	// create new track-content-object = pattern
	virtual trackContentObject * createTCO( const MidiTime & _pos );


	// called by track
	virtual void saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void loadTrackSpecificSettings( const QDomElement & _this );

	using track::setJournalling;


	// load instrument whose name matches given one
	Instrument * loadInstrument( const QString & _instrument_name );

	AudioPort * audioPort()
	{
		return &m_audioPort;
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
	
	int baseNote() const;

	Piano *pianoModel()
	{
		return &m_piano;
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

	IntModel * effectChannelModel()
	{
		return &m_effectChannelModel;
	}


signals:
	void instrumentChanged();
	void newNote();
	void midiNoteOn( const note& );
	void midiNoteOff( const note& );
	void nameChanged();


protected:
	virtual QString nodeName() const
	{
		return "instrumenttrack";
	}


protected slots:
	void updateBaseNote();
	void updatePitch();
	void updatePitchRange();


private:
	AudioPort m_audioPort;
	MidiPort m_midiPort;

	NotePlayHandle* m_notes[NumKeys];
	int m_runningMidiNotes[NumKeys];
	bool m_sustainPedalPressed;

	bool m_silentBuffersProcessed;

	IntModel m_baseNoteModel;

	NotePlayHandleList m_processHandles;

	FloatModel m_volumeModel;
	FloatModel m_panningModel;
	FloatModel m_pitchModel;
	IntModel m_pitchRangeModel;
	IntModel m_effectChannelModel;


	Instrument * m_instrument;
	InstrumentSoundShaping m_soundShaping;
	InstrumentFunctionArpeggio m_arpeggio;
	InstrumentFunctionNoteStacking m_noteStacking;

	Piano m_piano;


	friend class InstrumentTrackView;
	friend class InstrumentTrackWindow;
	friend class NotePlayHandle;
	friend class FlpImport;

} ;




class InstrumentTrackView : public trackView
{
	Q_OBJECT
public:
	InstrumentTrackView( InstrumentTrack * _it, TrackContainerView* tc );
	virtual ~InstrumentTrackView();

	InstrumentTrackWindow * getInstrumentTrackWindow();

	InstrumentTrack * model()
	{
		return castModel<InstrumentTrack>();
	}

	const InstrumentTrack * model() const
	{
		return castModel<InstrumentTrack>();
	}

	static InstrumentTrackWindow * topLevelInstrumentTrackWindow();

	QMenu * midiMenu()
	{
		return m_midiMenu;
	}

	void freeInstrumentTrackWindow();

	static void cleanupWindowCache();


protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );


private slots:
	void toggleInstrumentWindow( bool _on );
	void activityIndicatorPressed();
	void activityIndicatorReleased();

	void midiInSelected();
	void midiOutSelected();
	void midiConfigChanged();


private:
	InstrumentTrackWindow * m_window;

	static QQueue<InstrumentTrackWindow *> s_windowCache;

	// widgets in track-settings-widget
	trackLabelButton * m_tlb;
	knob * m_volumeKnob;
	knob * m_panningKnob;
	fadeButton * m_activityIndicator;

	QMenu * m_midiMenu;

	QAction * m_midiInputAction;
	QAction * m_midiOutputAction;

	QPoint m_lastPos;


	friend class InstrumentTrackWindow;

} ;




class InstrumentTrackWindow : public QWidget, public ModelView,
								public SerializingObjectHook
{
	Q_OBJECT
public:
	InstrumentTrackWindow( InstrumentTrackView * _tv );
	virtual ~InstrumentTrackWindow();

	// parent for all internal tab-widgets
	tabWidget * tabWidgetParent()
	{
		return m_tabWidget;
	}

	InstrumentTrack * model()
	{
		return castModel<InstrumentTrack>();
	}

	const InstrumentTrack * model() const
	{
		return castModel<InstrumentTrack>();
	}

	void setInstrumentTrackView( InstrumentTrackView * _tv );

	PianoView * pianoView()
	{
		return m_pianoView;
	}

	static void dragEnterEventGeneric( QDragEnterEvent * _dee );

	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );


public slots:
	void textChanged( const QString & _new_name );
	void toggleVisibility( bool _on );
	void updateName();
	void updateInstrumentView();


protected:
	// capture close-events for toggling instrument-track-button
	virtual void closeEvent( QCloseEvent * _ce );
	virtual void focusInEvent( QFocusEvent * _fe );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );


protected slots:
	void saveSettingsBtnClicked();


private:
	virtual void modelChanged();

	InstrumentTrack * m_track;
	InstrumentTrackView * m_itv;

	// widgets on the top of an instrument-track-window
	QLineEdit * m_nameLineEdit;
	knob * m_volumeKnob;
	knob * m_panningKnob;
	knob * m_pitchKnob;
	LcdSpinBox* m_pitchRangeSpinBox;
	LcdSpinBox * m_effectChannelNumber;


	// tab-widget with all children
	tabWidget * m_tabWidget;
	PluginView * m_instrumentView;
	InstrumentSoundShapingView * m_ssView;
	InstrumentFunctionNoteStackingView* m_noteStackingView;
	InstrumentFunctionArpeggioView* m_arpeggioView;
	InstrumentMidiIOView * m_midiView;
	EffectRackView * m_effectView;

	// test-piano at the bottom of every instrument-settings-window
	PianoView * m_pianoView;

	friend class InstrumentView;

} ;



#endif
