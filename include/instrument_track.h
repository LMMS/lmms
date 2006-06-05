/*
 * instrument_track.h - declaration of class instrumentTrack, a track + window
 *                      which holds an instrument-plugin
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _INSTRUMENT_TRACK_H
#define _INSTRUMENT_TRACK_H

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QApplication>
#include <QtGui/QPushButton>
#include <QtGui/QPainter>
#include <QtCore/QMutex>

#else

#include <qapplication.h>
#include <qpushbutton.h>
#include <qpainter.h>
#include <qmutex.h>

#endif

#include "track.h"
#include "mixer.h"
#include "midi_event_processor.h"
#include "gui_templates.h"
#include "tab_widget.h"
#include "engine.h"
#include "volume_knob.h"

class QLineEdit;
class arpAndChordsTabWidget;
class audioPort;
class instrumentTrackButton;
class envelopeTabWidget;
class fadeButton;
class instrument;
class lcdSpinBox;
class midiPort;
class midiTabWidget;
class notePlayHandle;
class pianoWidget;
class presetPreviewPlayHandle;
class surroundArea;

class flpImport;



class instrumentTrack : public QWidget, public track, public midiEventProcessor
{
	Q_OBJECT
public:
	instrumentTrack( trackContainer * _tc );
	virtual ~instrumentTrack();

	inline virtual trackTypes type( void ) const
	{
		return( m_trackType );
	}


	// used by instrument
	void FASTCALL processAudioBuffer( sampleFrame * _buf,
							const fpab_t _frames,
							notePlayHandle * _n );

	virtual void FASTCALL processInEvent( const midiEvent & _me,
						const midiTime & _time );
	virtual void FASTCALL processOutEvent( const midiEvent & _me,
						const midiTime & _time );

	// returns the frequency of a given tone & octave.
	// This function also includes base_tone & base_octave in
	// its calculations
	float FASTCALL frequency( notePlayHandle * _n ) const;
	f_cnt_t FASTCALL beatLen( notePlayHandle * _n ) const;


	// for capturing note-play-events -> need that for arpeggio,
	// filter and so on
	void FASTCALL playNote( notePlayHandle * _n );

	QString instrumentName( void ) const;
	void FASTCALL deleteNotePluginData( notePlayHandle * _n );

	// name-stuff
	inline const QString & name( void ) const
	{
		return( m_name );
	}
	void FASTCALL setName( const QString & _new_name );

	// volume & surround-position-stuff
	void FASTCALL setVolume( volume _new_volume );
	volume getVolume( void ) const;
	void FASTCALL setSurroundAreaPos( const QPoint & _p );
	const QPoint & surroundAreaPos( void ) const;

	void FASTCALL setBaseNote( Uint32 _new_note );

	inline tones baseTone( void ) const
	{
		return( m_baseTone );
	}

	inline octaves baseOctave( void ) const
	{
		return( m_baseOctave );
	}

	int FASTCALL masterKey( notePlayHandle * _n ) const;


	// play everything in given frame-range - creates note-play-handles
	virtual bool FASTCALL play( const midiTime & _start,
						const f_cnt_t _start_frame,
						const fpab_t _frames,
						const f_cnt_t _frame_base,
							Sint16 _tco_num = -1 );
	// create new track-content-object = pattern
	virtual trackContentObject * FASTCALL createTCO( const midiTime &
									_pos );


	// called by track
	virtual void FASTCALL saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadTrackSpecificSettings( const QDomElement &
									_this );

	using track::setJournalling;


	// load instrument whose name matches given one
	instrument * FASTCALL loadInstrument( const QString &
							_instrument_name );

	// parent for all internal tab-widgets
	QWidget * tabWidgetParent( void )
	{
		return( m_tabWidget );
	}


public slots:
	void surroundAreaPosChanged( const QPoint & _new_p );
	void textChanged( const QString & _new_name );
	void toggledInstrumentTrackButton( bool _on );


signals:
	void baseNoteChanged( void );


protected:
	// capture close-events for toggling instrument-track-button
	virtual void closeEvent( QCloseEvent * _ce );
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void focusInEvent( QFocusEvent * _fe );

	inline virtual QString nodeName( void ) const
	{
		return( "instrumenttrack" );
	}
	// invalidates all note-play-handles linked to this instrument
	void invalidateAllMyNPH( void );


protected slots:
	void saveSettingsBtnClicked( void );
	void activityIndicatorPressed( void );
	void activityIndicatorReleased( void );
	void midiInSelected( void );
	void midiOutSelected( void );
	void midiConfigChanged( bool );


private:
	trackTypes m_trackType;

	midiPort * m_midiPort;

	audioPort * m_audioPort;


	notePlayHandle * m_notes[NOTES_PER_OCTAVE * OCTAVES];

	QMutex m_notesMutex;


	tones m_baseTone;
	octaves m_baseOctave;

	QString m_name;


	// widgets on the top of a instrument-track-window
	tabWidget * m_generalSettingsWidget;
	QLineEdit * m_instrumentNameLE;
	volumeKnob * m_volumeKnob;
	surroundArea * m_surroundArea;
	lcdSpinBox * m_effectChannelNumber;
	QPushButton * m_saveSettingsBtn;

	
	// tab-widget with all children
	tabWidget * m_tabWidget;
	instrument * m_instrument;
	envelopeTabWidget * m_envWidget;
	arpAndChordsTabWidget * m_arpWidget;
	midiTabWidget * m_midiWidget;


	// test-piano at the bottom of every instrument-settings-window
	pianoWidget * m_pianoWidget;


	// widgets in track-settings-widget
	volumeKnob * m_tswVolumeKnob;
	fadeButton * m_tswActivityIndicator;
	instrumentTrackButton * m_tswInstrumentTrackButton;
	QMenu * m_tswMidiMenu;
#ifdef QT4
	QAction * m_midiInputAction;
	QAction * m_midiOutputAction;
#else
	int m_midiInputID;
	int m_midiOutputID;
#endif

	friend class instrumentTrackButton;
	friend class notePlayHandle;
	friend class presetPreviewPlayHandle;
	friend class flpImport;

	// base-tone stuff
	void FASTCALL setBaseTone( tones _new_tone );
	void FASTCALL setBaseOctave( octaves _new_octave );


signals:
	void noteDone( const note & _n );

} ;




class instrumentTrackButton : public QPushButton
{
public:
	instrumentTrackButton( instrumentTrack * _instrument_track );
	virtual ~instrumentTrackButton();

#ifdef QT3
	inline void setChecked( bool _on )
	{
		setOn( _on );
	}

	inline bool isChecked( void ) const
	{
		return( isOn() );
	}

	inline void setCheckable( bool _on )
	{
		QPushButton::setToggleButton( _on );
	}
#endif


protected:
	// since we want to draw a special label (instrument- and instrument-
	// name) on our button, we have to re-implement this for doing so
	virtual void drawButtonLabel( QPainter * _p );

	// allow drops on this button - we simply forward them to
	// instrument-track
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );


private:
	instrumentTrack * m_instrumentTrack;

} ;


#endif
