/*
 * Pattern.h - declaration of class Pattern, which contains all information
 *             about a pattern
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

#ifndef PATTERN_H
#define PATTERN_H

#include <QtCore/QVector>
#include <QWidget>
#include <QDialog>
#include <QPixmap>
#include <QStaticText>


#include "Note.h"
#include "Track.h"


class QAction;
class QProgressBar;
class QPushButton;

class InstrumentTrack;
class SampleBuffer;



class LMMS_EXPORT Pattern : public TrackContentObject
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

	void updateLength();

	// note management
	Note * addNote( const Note & _new_note, const bool _quant_pos = true );

	void removeNote( Note * _note_to_del );

	Note * noteAtStep( int _step );

	void rearrangeAllNotes();
	void clearNotes();

	inline const NoteVector & notes() const
	{
		return m_notes;
	}

	Note * addStepNote( int step );
	void setStep( int step, bool enabled );

	// pattern-type stuff
	inline PatternTypes type() const
	{
		return m_patternType;
	}


	// next/previous track based on position in the containing track
	Pattern * previousPattern() const;
	Pattern * nextPattern() const;

	// settings-management
	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return "pattern";
	}

	inline InstrumentTrack * instrumentTrack() const
	{
		return m_instrumentTrack;
	}

	bool empty();


	TrackContentObjectView * createView( TrackView * _tv ) override;


	using Model::dataChanged;


protected:
	void updateBBTrack();


protected slots:
	void addSteps();
	void cloneSteps();
	void removeSteps();
	void clear();
	void changeTimeSignature();


private:
	MidiTime beatPatternLength() const;

	void setType( PatternTypes _new_pattern_type );
	void checkType();

	void resizeToFirstTrack();

	InstrumentTrack * m_instrumentTrack;

	PatternTypes m_patternType;

	// data-stuff
	NoteVector m_notes;
	int m_steps;

	Pattern * adjacentPatternByOffset(int offset) const;

	friend class PatternView;
	friend class BBTrackContainerView;


signals:
	void destroyedPattern( Pattern* );

} ;



class PatternView : public TrackContentObjectView
{
	Q_OBJECT

public:
	PatternView( Pattern* pattern, TrackView* parent );
	virtual ~PatternView() = default;

	Q_PROPERTY(QColor noteFillColor READ getNoteFillColor WRITE setNoteFillColor)
	Q_PROPERTY(QColor noteBorderColor READ getNoteBorderColor WRITE setNoteBorderColor)
	Q_PROPERTY(QColor mutedNoteFillColor READ getMutedNoteFillColor WRITE setMutedNoteFillColor)
	Q_PROPERTY(QColor mutedNoteBorderColor READ getMutedNoteBorderColor WRITE setMutedNoteBorderColor)

	QColor const & getNoteFillColor() const { return m_noteFillColor; }
	void setNoteFillColor(QColor const & color) { m_noteFillColor = color; }

	QColor const & getNoteBorderColor() const { return m_noteBorderColor; }
	void setNoteBorderColor(QColor const & color) { m_noteBorderColor = color; }

	QColor const & getMutedNoteFillColor() const { return m_mutedNoteFillColor; }
	void setMutedNoteFillColor(QColor const & color) { m_mutedNoteFillColor = color; }

	QColor const & getMutedNoteBorderColor() const { return m_mutedNoteBorderColor; }
	void setMutedNoteBorderColor(QColor const & color) { m_mutedNoteBorderColor = color; }

public slots:
	void update() override;


protected slots:
	void openInPianoRoll();
	void setGhostInPianoRoll();

	void resetName();
	void changeName();


protected:
	void constructContextMenu( QMenu * ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void mouseDoubleClickEvent( QMouseEvent * _me ) override;
	void paintEvent( QPaintEvent * pe ) override;
	void wheelEvent( QWheelEvent * _we ) override;


private:
	static QPixmap * s_stepBtnOn0;
	static QPixmap * s_stepBtnOn200;
	static QPixmap * s_stepBtnOff;
	static QPixmap * s_stepBtnOffLight;

	Pattern* m_pat;
	QPixmap m_paintPixmap;

	QColor m_noteFillColor;
	QColor m_noteBorderColor;
	QColor m_mutedNoteFillColor;
	QColor m_mutedNoteBorderColor;

	QStaticText m_staticTextName;
} ;



#endif
