/*
 * MidiClip.h - declaration of class MidiClip, which contains all information
 *              about a clip
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

#ifndef MIDI_CLIP_H
#define MIDI_CLIP_H


#include "Clip.h"
#include "Note.h"


class InstrumentTrack;


class LMMS_EXPORT MidiClip : public Clip
{
	Q_OBJECT
public:
	enum MidiClipTypes
	{
		BeatClip,
		MelodyClip
	} ;

	MidiClip( InstrumentTrack* instrumentTrack );
	MidiClip( const MidiClip& other );
	virtual ~MidiClip();

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

	// Split the list of notes on the given position
	void splitNotes(NoteVector notes, TimePos pos);

	// clip-type stuff
	inline MidiClipTypes type() const
	{
		return m_clipType;
	}


	// next/previous track based on position in the containing track
	MidiClip * previousMidiClip() const;
	MidiClip * nextMidiClip() const;

	// settings-management
	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return "midiclip";
	}

	inline InstrumentTrack * instrumentTrack() const
	{
		return m_instrumentTrack;
	}

	bool empty();


	ClipView * createView( TrackView * _tv ) override;


	using Model::dataChanged;

public slots:
	void addSteps();
	void cloneSteps();
	void removeSteps();
	void clear();

protected:
	void updatePatternTrack();

protected slots:
	void changeTimeSignature();


private:
	TimePos beatClipLength() const;

	void setType( MidiClipTypes _new_clip_type );
	void checkType();

	void resizeToFirstTrack();

	InstrumentTrack * m_instrumentTrack;

	MidiClipTypes m_clipType;

	// data-stuff
	NoteVector m_notes;
	int m_steps;

	MidiClip * adjacentMidiClipByOffset(int offset) const;

	friend class MidiClipView;


signals:
	void destroyedMidiClip( MidiClip* );
} ;



#endif
