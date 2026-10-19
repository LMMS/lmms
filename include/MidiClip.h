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

#ifndef LMMS_MIDI_CLIP_H
#define LMMS_MIDI_CLIP_H

#include <memory>
#include <vector>

#include "Clip.h"
#include "Note.h"


namespace lmms
{


class InstrumentTrack;

namespace gui
{
class MidiClipView;
}


class LMMS_EXPORT MidiClip : public Clip
{
	Q_OBJECT
public:
	enum class Type
	{
		BeatClip,
		MelodyClip
	} ;

	MidiClip( InstrumentTrack* instrumentTrack );
	~MidiClip() override;

	void init();

	void updateLength() override;

	bool supportsLooping() const override { return !isInPattern(); }

	// note management
	Note * addNote( const Note & _new_note, const bool _quant_pos = true );

	NoteVector::const_iterator removeNote(NoteVector::const_iterator it);
	NoteVector::const_iterator removeNote(Note* note);

	Note * noteAtStep( int _step );

	void rearrangeAllNotes();
	void clearNotes();

	const NoteVector& notes() const;

	Note * addStepNote( int step );
	void setStep( int step, bool enabled );

	//! Horizontally flip the positions of the given notes.
	void reverseNotes(const NoteVector& notes);

	// Split the list of notes on the given position
	void splitNotes(const NoteVector& notes, TimePos pos);

	// Split the list of notes along a line
	void splitNotesAlongLine(const NoteVector notes, TimePos pos1, int key1, TimePos pos2, int key2, bool deleteShortEnds);

	// clip-type stuff
	inline Type type() const
	{
		return m_clipType;
	}


	// next/previous track based on position in the containing track
	MidiClip * previousMidiClip() const;
	MidiClip * nextMidiClip() const;

	// settings-management
	void exportToXML(QDomDocument& doc, QDomElement& midiClipElement, bool onlySelectedNotes = false);
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


	gui::ClipView * createView( gui::TrackView * _tv ) override;

	//! @brief Creates a copy of this clip that does not update with the original
	MidiClip* clone() override
	{
		return new MidiClip(*this);
	}

	// Linked clips share contents, so editing one of them changes all of them

	//! @brief Whether this clip can share its notes with other clips
	bool supportsLinking() const { return !isInPattern(); }

	//! @brief Whether this clip shares its notes with at least one other clip
	bool isLinked() const;

	//! @brief All clips sharing notes with this clip, including this clip
	const std::vector<MidiClip*>& linkedClips() const;

	//! @brief Identifies the notes shared by linked clips. Stored as the "link" attribute in project files.
	const QString& linkId() const;

	//! @brief The label that tells linked clips of the same group apart from other groups (A, B, ..., Z, A1, B1, ...)
	//! @return An empty string if the clip isn't linked
	QString linkLabel() const;
	static QString linkLabel(int index);

	//! @brief Creates a copy of this clip that updates with the original
	MidiClip* linkedClone();

	//! @brief Shares the notes of @p other, replacing the notes of this clip
	void linkTo(MidiClip* other);

	//! @brief Stops sharing notes with other clips while keeping the notes
	void unlink();

	//! @brief Gives the MIDI clips found in @p element new link IDs, so that clips created from it are linked
	//! with each other like before, but not with existing clips
	static void renewLinkIds(QDomElement& element);


	using Model::dataChanged;

public slots:
	void addSteps();
	void cloneSteps();
	void removeSteps();
	void clear();

protected:
	MidiClip( const MidiClip& other );
	void updatePatternTrack();

protected slots:
	void changeTimeSignature();


private:
	TimePos beatClipLength() const;

	void setType( Type _new_clip_type );
	void checkType();

	void resizeToFirstTrack();

	InstrumentTrack * m_instrumentTrack;

	Type m_clipType;

	struct LinkGroup;
	using LinkGroupPtr = std::shared_ptr<LinkGroup>;

	void setLinkGroup(LinkGroupPtr group);
	void setLinkId(const QString& id);
	//! Updates the other clips sharing the notes after the notes changed
	void updateLinkedClips();
	//! Locks the tracks of all clips sharing the notes, so that none of them plays while the notes change
	void lockLinkedTracks();
	void unlockLinkedTracks();

	// data-stuff
	LinkGroupPtr m_links;
	int m_steps;

	MidiClip * adjacentMidiClipByOffset(int offset) const;

	friend class gui::MidiClipView;


signals:
	void destroyedMidiClip( lmms::MidiClip* );
} ;


} // namespace lmms

#endif // LMMS_MIDI_CLIP_H
