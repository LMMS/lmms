/*
 * instrument_track.h - declaration of class instrumentTrack, a track + window
 *                      which holds an instrument-plugin
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "audio_port.h"
#include "instrument_functions.h"
#include "instrument_sound_shaping.h"
#include "midi_event_processor.h"
#include "midi_port.h"
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
class instrumentTrackButton;
class instrumentTrackWindow;
class instrumentMidiIOView;
class lcdSpinBox;
class midiPortMenu;
class notePlayHandle;
class pluginView;
class presetPreviewPlayHandle;
class tabWidget;


class EXPORT instrumentTrack : public track, public midiEventProcessor
{
	Q_OBJECT
	mapPropertyFromModel(int,getVolume,setVolume,m_volumeModel);
public:
	instrumentTrack( trackContainer * _tc );
	virtual ~instrumentTrack();

	// used by instrument
	void processAudioBuffer( sampleFrame * _buf, const fpp_t _frames,
							notePlayHandle * _n );

	virtual void processInEvent( const midiEvent & _me,
					const midiTime & _time );
	virtual void processOutEvent( const midiEvent & _me,
						const midiTime & _time );


	f_cnt_t beatLen( notePlayHandle * _n ) const;


	// for capturing note-play-events -> need that for arpeggio,
	// filter and so on
	void playNote( notePlayHandle * _n, bool _try_parallelizing,
						sampleFrame * _working_buffer );

	QString instrumentName( void ) const;
	inline const instrument * getInstrument( void ) const
	{
		return( m_instrument );
	}

	void deleteNotePluginData( notePlayHandle * _n );

	// name-stuff
	virtual void setName( const QString & _new_name );

	// translate key of given notePlayHandle to absolute key (i.e.
	// add global master-pitch and base-note in piano)
	int masterKey( notePlayHandle * _n ) const;

	// translate pitch to midi-pitch [0,16383]
	inline int midiPitch( void ) const
	{
		return( (int)( ( m_pitchModel.value()+100 ) * 81.92 ) );
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

	inline audioPort * getAudioPort( void )
	{
		return( &m_audioPort );
	}

	intModel * baseNoteModel( void )
	{
		return( &m_baseNoteModel );
	}

	piano * getPiano( void )
	{
		return( &m_piano );
	}

	bool arpeggiatorEnabled( void ) const
	{
		return( m_arpeggiator.m_arpEnabledModel.value() );
	}

	virtual QString displayName( void ) const;


signals:
	void instrumentChanged( void );
	void newNote( void );
	void noteDone( const note & _n );
	void nameChanged( void );


protected:
	virtual QString nodeName( void ) const
	{
		return( "instrumenttrack" );
	}
	// invalidates all note-play-handles linked to this instrument
	void invalidateAllMyNPH( void );


protected slots:
	void updateBaseNote( void );


private:
	audioPort m_audioPort;
	midiPort m_midiPort;

	notePlayHandle * m_notes[NumKeys];

	intModel m_baseNoteModel;

	QList<notePlayHandle *> m_processHandles;


	floatModel m_volumeModel;
	floatModel m_panningModel;
	floatModel m_pitchModel;
	intModel m_effectChannelModel;


	instrument * m_instrument;
	instrumentSoundShaping m_soundShaping;
	arpeggiator m_arpeggiator;
	chordCreator m_chordCreator;

	piano m_piano;


	friend class instrumentTrackView;
	friend class instrumentTrackWindow;
	friend class notePlayHandle;
	friend class presetPreviewPlayHandle;
	friend class flpImport;

} ;




class instrumentTrackView : public trackView
{
	Q_OBJECT
public:
	instrumentTrackView( instrumentTrack * _it, trackContainerView * _tc );
	virtual ~instrumentTrackView();

	instrumentTrackWindow * getInstrumentTrackWindow( void );

	instrumentTrack * model( void )
	{
		return( castModel<instrumentTrack>() );
	}

	const instrumentTrack * model( void ) const
	{
		return( castModel<instrumentTrack>() );
	}


	void freeInstrumentTrackWindow( void );

	static void cleanupWindowPool( void );


private slots:
	void toggledInstrumentTrackButton( bool _on );
	void activityIndicatorPressed( void );
	void activityIndicatorReleased( void );

	void midiInSelected( void );
	void midiOutSelected( void );
	void midiConfigChanged( void );


private:
	instrumentTrackWindow * m_window;
	
	static QQueue<instrumentTrackWindow *> s_windows;

	// widgets in track-settings-widget
	knob * m_tswVolumeKnob;
	knob * m_tswPanningKnob;
	fadeButton * m_tswActivityIndicator;
	instrumentTrackButton * m_tswInstrumentTrackButton;

	QMenu * m_tswMidiMenu;

	QAction * m_midiInputAction;
	QAction * m_midiOutputAction;


	friend class instrumentTrackButton;
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
	tabWidget * tabWidgetParent( void )
	{
		return( m_tabWidget );
	}

	instrumentTrack * model( void )
	{
		return( castModel<instrumentTrack>() );
	}

	const instrumentTrack * model( void ) const
	{
		return( castModel<instrumentTrack>() );
	}

	void setInstrumentTrackView( instrumentTrackView * _tv )
	{
		m_itv = _tv;
	}


public slots:
	void textChanged( const QString & _new_name );
	void toggledInstrumentTrackButton( bool _on );
	void updateName( void );
	void updateInstrumentView( void );


protected:
	// capture close-events for toggling instrument-track-button
	virtual void closeEvent( QCloseEvent * _ce );
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void focusInEvent( QFocusEvent * _fe );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );


protected slots:
	void saveSettingsBtnClicked( void );


private:
	virtual void modelChanged( void );

	instrumentTrack * m_track;
	instrumentTrackView * m_itv;

	// widgets on the top of an instrument-track-window
	tabWidget * m_generalSettingsWidget;
	QLineEdit * m_nameLineEdit;
	knob * m_volumeKnob;
	knob * m_panningKnob;
	knob * m_pitchKnob;
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
	pianoView * m_pianoView;

	friend class instrumentTrackButton;
	friend class instrumentView;

} ;




class instrumentTrackButton : public QPushButton
{
public:
	instrumentTrackButton( instrumentTrackView * _instrument_track_view );
	virtual ~instrumentTrackButton();


protected:
	// since we want to draw a special label (instrument- and instrument-
	// name) on our button, we have to re-implement this for doing so
	virtual void paintEvent( QPaintEvent * _pe );

	// allow drops on this button - we simply forward them to
	// instrument-track
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );


private:
	instrumentTrackView * m_instrumentTrackView;

} ;


#endif
