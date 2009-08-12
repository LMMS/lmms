/*
 * instrument_track.h - declaration of class instrumentTrack, a track + window
 *                      which holds an instrument-plugin
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#ifndef _INSTRUMENT_TRACK_H
#define _INSTRUMENT_TRACK_H

#include <QtGui/QPushButton>

#include "AudioPort.h"
#include "instrument_functions.h"
#include "instrument_sound_shaping.h"
#include "MidiEventProcessor.h"
#include "MidiPort.h"
#include "note_play_handle.h"
#include "piano.h"
#include "track.h"


class QLineEdit;
template<class T> class QQueue;
class arpeggiatorView;
class chordCreatorView;
class effectRackView;
class instrumentSoundShapingView;
class fadeButton;
class instrument;
class instrumentTrackWindow;
class instrumentMidiIOView;
class lcdSpinBox;
class midiPortMenu;
class multimediaProject;
class notePlayHandle;
class pluginView;
class tabWidget;
class trackLabelButton;


class EXPORT instrumentTrack : public track, public MidiEventProcessor
{
	Q_OBJECT
	mapPropertyFromModel(int,getVolume,setVolume,m_volumeModel);
public:
	instrumentTrack( trackContainer * _tc );
	virtual ~instrumentTrack();

	// used by instrument
	void processAudioBuffer( sampleFrame * _buf, const fpp_t _frames,
							notePlayHandle * _n );

	midiEvent applyMasterKey( const midiEvent & _me );

	virtual void processInEvent( const midiEvent & _me,
					const midiTime & _time );
	virtual void processOutEvent( const midiEvent & _me,
						const midiTime & _time );
	// silence all running notes played by this track
	void silenceAllNotes();

	f_cnt_t beatLen( notePlayHandle * _n ) const;


	// for capturing note-play-events -> need that for arpeggio,
	// filter and so on
	void playNote( notePlayHandle * _n, sampleFrame * _working_buffer );

	QString instrumentName() const;
	inline const instrument * getInstrument() const
	{
		return m_instrument;
	}

	inline instrument * getInstrument()
	{
		return m_instrument;
	}

	void deleteNotePluginData( notePlayHandle * _n );

	// name-stuff
	virtual void setName( const QString & _new_name );

	// translate given key of a note-event to absolute key (i.e.
	// add global master-pitch and base-note of this instrument track)
	int masterKey( int _midi_key ) const;

	// translate pitch to midi-pitch [0,16383]
	inline int midiPitch() const
	{
		return (int)( ( m_pitchModel.value()+100 ) * 16383 ) / 200;
	}

	// play everything in given frame-range - creates note-play-handles
	virtual bool play( const midiTime & _start, const fpp_t _frames,
					const f_cnt_t _frame_base,
							Sint16 _tco_num = -1 );
	// create new view for me
	virtual trackView * createView( trackContainerView * _tcv );

	// create new track-content-object = pattern
	virtual trackContentObject * createTCO( const midiTime & _pos );


	// called by track
	virtual void saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void loadTrackSpecificSettings( const QDomElement & _this );

	using track::setJournalling;


	// load instrument whose name matches given one
	instrument * loadInstrument( const QString & _instrument_name );

	inline AudioPort * audioPort()
	{
		return &m_audioPort;
	}

	inline MidiPort * midiPort()
	{
		return &m_midiPort;
	}

	Piano * pianoModel()
	{
		return &m_piano;
	}

	bool arpeggiatorEnabled() const
	{
		return m_arpeggiator.m_arpEnabledModel.value();
	}

	// simple helper for removing midiport-XML-node when loading presets
	static void removeMidiPortNode( multimediaProject & _mmp );

	floatModel * volumeModel()
	{
		return &m_volumeModel;
	}

	floatModel * panningModel()
	{
		return &m_panningModel;
	}

	floatModel * pitchModel()
	{
		return &m_pitchModel;
	}

	intModel * pitchRangeModel()
	{
		return &m_pitchRangeModel;
	}

	intModel * effectChannelModel()
	{
		return &m_effectChannelModel;
	}


signals:
	void instrumentChanged();
	void newNote();
	void noteOn( const note & _n );
	void noteOff( const note & _n );
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

	notePlayHandle * m_notes[NumKeys];
	int m_runningMidiNotes[NumKeys];

	NotePlayHandleList m_processHandles;


	floatModel m_volumeModel;
	floatModel m_panningModel;
	floatModel m_pitchModel;
	intModel m_pitchRangeModel;
	intModel m_effectChannelModel;


	instrument * m_instrument;
	instrumentSoundShaping m_soundShaping;
	arpeggiator m_arpeggiator;
	chordCreator m_chordCreator;

	Piano m_piano;


	friend class instrumentTrackView;
	friend class instrumentTrackWindow;
	friend class notePlayHandle;
	friend class flpImport;

} ;




class instrumentTrackView : public trackView
{
	Q_OBJECT
public:
	instrumentTrackView( instrumentTrack * _it, trackContainerView * _tc );
	virtual ~instrumentTrackView();

	instrumentTrackWindow * getInstrumentTrackWindow();

	instrumentTrack * model()
	{
		return castModel<instrumentTrack>();
	}

	const instrumentTrack * model() const
	{
		return castModel<instrumentTrack>();
	}


	QMenu * midiMenu()
	{
		return m_midiMenu;
	}

	void freeInstrumentTrackWindow();

	static void cleanupWindowPool();


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
	instrumentTrackWindow * m_window;

	static QQueue<instrumentTrackWindow *> s_windows;

	// widgets in track-settings-widget
	trackLabelButton * m_tlb;
	knob * m_volumeKnob;
	knob * m_panningKnob;
	fadeButton * m_activityIndicator;

	QMenu * m_midiMenu;

	QAction * m_midiInputAction;
	QAction * m_midiOutputAction;

	QPoint m_lastPos;


	friend class instrumentTrackWindow;

} ;




class instrumentTrackWindow : public QWidget, public modelView,
						public serializingObjectHook
{
	Q_OBJECT
public:
	instrumentTrackWindow( instrumentTrackView * _tv );
	virtual ~instrumentTrackWindow();

	// parent for all internal tab-widgets
	tabWidget * tabWidgetParent()
	{
		return m_tabWidget;
	}

	instrumentTrack * model()
	{
		return castModel<instrumentTrack>();
	}

	const instrumentTrack * model() const
	{
		return castModel<instrumentTrack>();
	}

	void setInstrumentTrackView( instrumentTrackView * _tv )
	{
		m_itv = _tv;
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

	instrumentTrack * m_track;
	instrumentTrackView * m_itv;

	// widgets on the top of an instrument-track-window
	tabWidget * m_generalSettingsWidget;
	QLineEdit * m_nameLineEdit;
	knob * m_volumeKnob;
	knob * m_panningKnob;
	knob * m_pitchKnob;
	lcdSpinBox * m_pitchRange;
	lcdSpinBox * m_effectChannelNumber;
	QPushButton * m_saveSettingsBtn;


	// tab-widget with all children
	tabWidget * m_tabWidget;
	pluginView * m_instrumentView;
	instrumentSoundShapingView * m_ssView;
	chordCreatorView * m_chordView;
	arpeggiatorView * m_arpView;
	instrumentMidiIOView * m_midiView;
	effectRackView * m_effectView;

	// test-piano at the bottom of every instrument-settings-window
	PianoView * m_pianoView;

	friend class InstrumentView;

} ;



#endif
