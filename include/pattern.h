/*
 * pattern.h - declaration of class pattern, which contains all informations
 *             about a pattern
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _PATTERN_H
#define _PATTERN_H

#include "qt3support.h"

#ifdef QT4

#include <QtCore/QVector>
#include <QtGui/QWidget>
#include <QtCore/QMutex>
#include <QtGui/QDialog>
#include <QtCore/QThread>
#include <QtGui/QPixmap>

#else

#include <qvaluevector.h>
#include <qwidget.h>
#include <qmutex.h>
#include <qdialog.h>
#include <qthread.h>
#include <qpixmap.h>

#endif


#include "note.h"
#include "track.h"
#include "mixer.h"


class QAction;
class QProgressBar;
class QPushButton;

class instrumentTrack;
class patternFreezeThread;
class sampleBuffer;


const int DEFAULT_STEPS_PER_TACT = 16;
const int BEATS_PER_TACT = 4;



class pattern : public trackContentObject
{
	Q_OBJECT
public:
	enum patternTypes
	{
		BEAT_PATTERN, MELODY_PATTERN/*, AUTOMATION_PATTERN*/
	} ;

	pattern( instrumentTrack * _channel_track );
	pattern( const pattern & _pat_to_copy );
	virtual ~pattern();

	void init( void );


	virtual midiTime length( void ) const;

	note * FASTCALL addNote( const note & _new_note,
					const bool _quant_pos = TRUE );

	void FASTCALL removeNote( const note * _note_to_del );

	note * FASTCALL rearrangeNote( const note * _note_to_proc,
					const bool _quant_pos = TRUE );

	void clearNotes( void );
	
	inline noteVector & notes( void )
	{
		return( m_notes );
	}

	note * FASTCALL noteAt( int _note_num );

	void FASTCALL setNoteAt( int _note_num, note _new_note );

	// pattern-type stuff
	inline patternTypes type( void ) const
	{
		return( m_patternType );
	}
	void FASTCALL setType( patternTypes _new_pattern_type );
	void checkType( void );


	// pattern-name functions
	inline const QString & name( void ) const
	{
		return( m_name );
	}

	inline void setName( const QString & _name )
	{
		m_name = _name;
		update();
	}


	// functions which are part of freezing-feature
	inline bool freezing( void ) const
	{
		return( m_freezing );
	}
	
	inline bool frozen( void ) const
	{
		return( m_frozenPattern != NULL );
	}

	sampleBuffer * getFrozenPattern( void )
	{
		return( m_frozenPattern );
	}

	// settings-management
	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "pattern" );
	}

	inline instrumentTrack * getInstrumentTrack( void )
	{
		return( m_instrumentTrack );
	}

	bool empty( void );


public slots:
	virtual void update( void );


protected slots:
	void openInPianoRoll( bool _c );
	void openInPianoRoll( void );

	void clear( void );
	void resetName( void );
	void changeName( void );
	void freeze( void );
	void unfreeze( void );
	void abortFreeze( void );

	void addSteps( QAction * _item );
	void removeSteps( QAction * _item );
	void addSteps( int _n );
	void removeSteps( int _n );


protected:
	virtual void constructContextMenu( QMenu * );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re )
	{
		m_needsUpdate = TRUE;
		trackContentObject::resizeEvent( _re );
	}
	virtual void wheelEvent( QWheelEvent * _we );

	void ensureBeatNotes( void );
	void updateBBTrack( void );


private:
	static QPixmap * s_stepBtnOn;
	static QPixmap * s_stepBtnOverlay;
	static QPixmap * s_stepBtnOff;
	static QPixmap * s_stepBtnOffLight;
	static QPixmap * s_frozen;

	QPixmap m_paintPixmap;
	bool m_needsUpdate;

	// general stuff
	instrumentTrack * m_instrumentTrack;

	patternTypes m_patternType;
	QString m_name;

	// data-stuff
	noteVector m_notes;
	int m_steps;

	// pattern freezing
	QMutex m_frozenPatternMutex;
	sampleBuffer * m_frozenPattern;
	bool m_freezing;
	volatile bool m_freezeAborted;


	// as in Qt4 QThread is inherits from QObject and our base
	// trackContentObject is a QWidget (=QObject), we cannot inherit from
	// QThread. That's why we have to put pattern-freezing into separate
	// thread-class -> patternFreezeThread
	friend class patternFreezeThread;

} ;




class patternFreezeStatusDialog : public QDialog
{
	Q_OBJECT
public:
	patternFreezeStatusDialog( QThread * _thread );
	~patternFreezeStatusDialog();

	void FASTCALL setProgress( int _p );


protected:
	void closeEvent( QCloseEvent * _ce );


protected slots:
	void cancelBtnClicked( void );
	void updateProgress( void );


private:
	QProgressBar * m_progressBar;
	QPushButton * m_cancelBtn;

	QThread * m_freezeThread;

	int m_progress;


signals:
	void aborted( void );

} ;





class patternFreezeThread : public QThread, public engineObject
{
public:
	patternFreezeThread( pattern * _pattern );
	virtual ~patternFreezeThread();


protected:
	virtual void run( void );


private:
	pattern * m_pattern;
	patternFreezeStatusDialog * m_statusDlg;

} ;


#endif
