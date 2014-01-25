/*
 * pattern.h - declaration of class pattern, which contains all informations
 *             about a pattern
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

class InstrumentTrack;
class patternFreezeThread;
class SampleBuffer;



class EXPORT pattern : public trackContentObject
{
	Q_OBJECT
public:
	enum PatternTypes
	{
		BeatPattern,
		MelodyPattern
	} ;

	pattern( InstrumentTrack * _instrument_track );
	pattern( const pattern & _pat_to_copy );
	virtual ~pattern();

	void init();


	virtual MidiTime length() const;
	MidiTime beatPatternLength() const;

	// note management
	note * addNote( const note & _new_note, const bool _quant_pos = true );

	void removeNote( const note * _note_to_del );

	note * rearrangeNote( const note * _note_to_proc,
						const bool _quant_pos = true );
	void rearrangeAllNotes();
	void clearNotes();

	inline const NoteVector & notes() const
	{
		return m_notes;
	}

	void setStep( int _step, bool _enabled );

	// pattern-type stuff
	inline PatternTypes type() const
	{
		return m_patternType;
	}
	void setType( PatternTypes _new_pattern_type );
	void checkType();


	// functions which are part of freezing-feature
	inline bool isFreezing() const
	{
		return m_freezing;
	}

	inline bool isFrozen() const
	{
		return m_frozenPattern != NULL;
	}

	SampleBuffer *frozenPattern()
	{
		return m_frozenPattern;
	}

	// settings-management
	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return "pattern";
	}

	inline InstrumentTrack * instrumentTrack() const
	{
		return m_instrumentTrack;
	}

	bool empty();


	virtual trackContentObjectView * createView( trackView * _tv );


	using Model::dataChanged;


protected:
	void ensureBeatNotes();
	void updateBBTrack();


protected slots:
	void addSteps();
	void removeSteps();
	void clear();
	void freeze();
	void unfreeze();
	void abortFreeze();
	void changeTimeSignature();


private:
	InstrumentTrack * m_instrumentTrack;

	PatternTypes m_patternType;

	// data-stuff
	NoteVector m_notes;
	int m_steps;

	// pattern freezing
	SampleBuffer* m_frozenPattern;
	bool m_freezing;
	volatile bool m_freezeAborted;


	friend class patternView;
	friend class patternFreezeThread;
	friend class bbEditor;

} ;



class patternView : public trackContentObjectView
{
	Q_OBJECT
public:
	patternView( pattern * _pattern, trackView * _parent );
	virtual ~patternView();


public slots:
	virtual void update();


protected slots:
	void openInPianoRoll();

	void resetName();
	void changeName();


protected:
	virtual void constructContextMenu( QMenu * );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re )
	{
		m_needsUpdate = true;
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
	void cancelBtnClicked();
	void updateProgress();


private:
	QProgressBar * m_progressBar;
	QPushButton * m_cancelBtn;

	QThread * m_freezeThread;

	int m_progress;


signals:
	void aborted();

} ;





class patternFreezeThread : public QThread
{
public:
	patternFreezeThread( pattern * _pattern );
	virtual ~patternFreezeThread();


protected:
	virtual void run();


private:
	pattern * m_pattern;
	patternFreezeStatusDialog * m_statusDlg;

} ;


#endif
