/*
 * Pattern.h - declaration of class Pattern, which contains all information
 *             about a pattern
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

#ifndef PATTERN_H
#define PATTERN_H

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
class SampleBuffer;



class EXPORT Pattern : public trackContentObject
{
	Q_OBJECT
public:
	enum PatternTypes
	{
		BeatPattern,
		MelodyPattern
	} ;

	Pattern( InstrumentTrack* instrumentTrack );
	Pattern( const Pattern& other );
	virtual ~Pattern();

	void init();


	virtual MidiTime length() const;
	MidiTime beatPatternLength() const;

	// note management
	note * addNote( const note & _new_note, const bool _quant_pos = true );

	void removeNote( const note * _note_to_del );

	note * noteAtStep( int _step );

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
	void changeTimeSignature();


private:
	InstrumentTrack * m_instrumentTrack;

	PatternTypes m_patternType;

	// data-stuff
	NoteVector m_notes;
	int m_steps;

	friend class PatternView;
	friend class bbEditor;


signals:
	void destroyedPattern( Pattern* );

} ;



class PatternView : public trackContentObjectView
{
	Q_OBJECT

// theming qproperties
	Q_PROPERTY( QColor fgColor READ fgColor WRITE setFgColor )
	Q_PROPERTY( QColor textColor READ textColor WRITE setTextColor )
public:
	PatternView( Pattern* pattern, trackView* parent );
	virtual ~PatternView();


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

	Pattern* m_pat;
	QPixmap m_paintPixmap;
	bool m_needsUpdate;
} ;



#endif
