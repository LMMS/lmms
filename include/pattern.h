/*
 * pattern.h - declaration of class pattern, which contains all informations
 *             about a pattern
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _PATTERN_H
#define _PATTERN_H

#include "qt3support.h"

#ifdef QT4

#include <QVector>
#include <QWidget>
#include <QMutex>
#include <QDialog>

#else

#include <qvaluevector.h>
#include <qwidget.h>
#include <qmutex.h>
#include <qdialog.h>

#endif


#include "note.h"
#include "track.h"
#include "mixer.h"


class channelTrack;
class sampleBuffer;
class audioSampleRecorder;
class QTimer;
class QProgressBar;
class QPushButton;
class QPixmap;
class patternFreezeStatusDialog; 



const int MAX_BEATS_PER_TACT = 16;
const int MAIN_BEATS_PER_TACT = 4;


class pattern : public trackContentObject
{
	Q_OBJECT
public:
	enum patternTypes
	{
		BEAT_PATTERN, MELODY_PATTERN/*, EVENT_PATTERN*/
	} ;

	pattern( channelTrack * _channel_track );
	pattern( const pattern & _pat_to_copy ) FASTCALL;
	virtual ~pattern();

	virtual void FASTCALL movePosition( const midiTime & _pos );


	virtual midiTime length( void ) const;
	note * FASTCALL addNote( const note & _new_note );
	void FASTCALL removeNote( const note * _note_to_del );
	note * FASTCALL rearrangeNote( const note * _note_to_proc );
	void clearNotes( void );
	
	inline noteVector & notes( void )
	{
		return( m_notes );
	}

	inline patternTypes type( void ) const
	{
		return( m_patternType );
	}
	void FASTCALL setType( patternTypes _new_pattern_type );
	inline const QString & name( void ) const
	{
		return( m_name );
	}
	inline void setName( const QString & _name )
	{
		m_name = _name;
		update();
	}
	inline channelTrack * getChannelTrack( void )
	{
		return( m_channelTrack );
	}

	// functions which are part of freezing-feature
	inline bool frozen( void ) const
	{
		return( m_frozenPattern != NULL );
	}
	void FASTCALL playFrozenData( sampleFrame * _ab, Uint32 _start_frame,
							Uint32 _frames );
	inline bool isFreezing( void ) const
	{
		return( m_freezeRecorder != NULL );
	}
	void finishFreeze( void );


	note * FASTCALL noteAt( int _note_num );
	void FASTCALL setNoteAt( int _note_num, note _new_note );
	void checkType( void );

	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "pattern" );
	}


protected slots:
	void openInPianoRoll( bool _c );
	void openInPianoRoll( void );
	void clear( void );
	void resetName( void );
	void changeName( void );
	void freeze( void );
	void unfreeze( void );
	void updateFreezeStatusDialog( void );
	void abortFreeze( void );


protected:
	void paintEvent( QPaintEvent * _pe );
	void mousePressEvent( QMouseEvent * _me );
	void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void constructContextMenu( QMenu * );

	void ensureBeatNotes( void );


private:
	static QPixmap * s_patternBg;
	static QPixmap * s_stepBtnOn;
	static QPixmap * s_stepBtnOff;
	static QPixmap * s_stepBtnOffLight;
	static QPixmap * s_frozen;

	static void initPixmaps( void );


	channelTrack * m_channelTrack;
	patternTypes m_patternType;
	QString m_name;
	noteVector m_notes;

	QMutex m_frozenPatternMutex;
	sampleBuffer * m_frozenPattern;
	audioSampleRecorder * m_freezeRecorder;
	patternFreezeStatusDialog * m_freezeStatusDialog;
	QTimer * m_freezeStatusUpdateTimer;
} ;




class patternFreezeStatusDialog : public QDialog
{
	Q_OBJECT
public:
	patternFreezeStatusDialog();
	~patternFreezeStatusDialog();

	void FASTCALL setProgress( int _p );


protected:
	void closeEvent( QCloseEvent * _ce );


protected slots:
	void cancelBtnClicked( void );


private:
	QProgressBar * m_progressBar;
	QPushButton * m_cancelBtn;


signals:
	void aborted( void );

} ;


#endif
