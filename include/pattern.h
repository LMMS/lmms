/*
 * pattern.h - declaration of class pattern, which contains all informations
 *             about a pattern
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


#ifndef _PATTERN_H
#define _PATTERN_H

#include <QtCore/QVector>
#include <QtGui/QWidget>
#include <QtGui/QDialog>
#include <QtCore/QThread>
#include <QtGui/QPixmap>


#include "note.h"
#include "track.h"


class QAction;
class QProgressBar;
class QPushButton;

class instrumentTrack;
class patternFreezeThread;
class sampleBuffer;



class EXPORT pattern : public trackContentObject
{
	Q_OBJECT
public:
	enum PatternTypes
	{
		BeatPattern,
		MelodyPattern
	} ;

	pattern( instrumentTrack * _instrument_track );
	pattern( const pattern & _pat_to_copy );
	virtual ~pattern();

	void init( void );


	virtual midiTime length( void ) const;
	midiTime beatPatternLength( void ) const;

	note * addNote( const note & _new_note, const bool _quant_pos = TRUE );

	void removeNote( const note * _note_to_del );

	note * rearrangeNote( const note * _note_to_proc,
						const bool _quant_pos = TRUE );
	void rearrangeAllNotes( void );
	void clearNotes( void );

	inline const noteVector & notes( void )
	{
		return( m_notes );
	}

	// pattern-type stuff
	inline PatternTypes type( void ) const
	{
		return( m_patternType );
	}
	void setType( PatternTypes _new_pattern_type );
	void checkType( void );


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
	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "pattern" );
	}

	inline instrumentTrack * getInstrumentTrack( void )
	{
		return( m_instrumentTrack );
	}

	bool empty( void );


	void addSteps( int _n );
	void removeSteps( int _n );

	virtual trackContentObjectView * createView( trackView * _tv );


	using model::dataChanged;


protected:
	void ensureBeatNotes( void );
	void updateBBTrack( void );

	void abortFreeze( void );


protected slots:
	void clear( void );
	void freeze( void );
	void unfreeze( void );
	void changeTimeSignature( void );


private:
	instrumentTrack * m_instrumentTrack;

	PatternTypes m_patternType;

	// data-stuff
	noteVector m_notes;
	int m_steps;

	// pattern freezing
	sampleBuffer * m_frozenPattern;
	bool m_freezing;
	volatile bool m_freezeAborted;


	friend class patternView;
	friend class patternFreezeThread;

} ;



class patternView : public trackContentObjectView
{
	Q_OBJECT
public:
	patternView( pattern * _pattern, trackView * _parent );
	virtual ~patternView();


public slots:
	virtual void update( void );


protected slots:
	void openInPianoRoll( void );

	void resetName( void );
	void changeName( void );

	void addSteps( QAction * _item );
	void removeSteps( QAction * _item );


protected:
	virtual void constructContextMenu( QMenu * );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re )
	{
		m_needsUpdate = TRUE;
		trackContentObjectView::resizeEvent( _re );
	}
	virtual void wheelEvent( QWheelEvent * _we );


private:
	static QPixmap * s_stepBtnOn;
	static QPixmap * s_stepBtnOverlay;
	static QPixmap * s_stepBtnOff;
	static QPixmap * s_stepBtnOffLight;
	static QPixmap * s_frozen;

	pattern * m_pat;
	QPixmap m_paintPixmap;
	bool m_needsUpdate;

} ;




// TODO: move to own header-files
//


class patternFreezeStatusDialog : public QDialog
{
	Q_OBJECT
public:
	patternFreezeStatusDialog( QThread * _thread );
	virtual ~patternFreezeStatusDialog();

	void setProgress( int _p );


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





class patternFreezeThread : public QThread
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
