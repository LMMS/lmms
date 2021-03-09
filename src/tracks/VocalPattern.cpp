//
// Created by seledreams on 02/03/2021.
//

#include "VocalPattern.h"
#include "VocalNote.h"

VocalPattern::VocalPattern(InstrumentTrack *track)
	: Pattern(track)
{
	m_patternType = PatternTypes::MelodyPattern;
}

VocalPattern::VocalPattern(Pattern &pattern) : Pattern(pattern)
{
	for (int i = 0; i < m_notes.length();i++){
		Note *currentNote = m_notes[i];
		if (!dynamic_cast<VocalNote*>(currentNote)){
			VocalNote *vocalNote = new VocalNote(*currentNote);

			m_notes[i] = vocalNote;
			delete currentNote;
		}
	}
}

void VocalPattern::ResetAllLyrics()
{
	for (auto note : notes()){
		VocalNote *vocalNote = dynamic_cast<VocalNote*>(note);
		if (vocalNote){
			vocalNote->setLyric("la");
		}
	}
}
void VocalPattern::ResetAllNotes()
{
	for (auto note : notes()){
		VocalNote *vocalNote = dynamic_cast<VocalNote*>(note);
		if (vocalNote){
			vocalNote->ResetVocalNote();
		}
	}
}
Note *VocalPattern::addNote(const Note &_new_note, const bool _quant_pos)
{
	VocalNote *vocalNote = new VocalNote(_new_note);
	return Pattern::addNote(vocalNote, _quant_pos);
}
void VocalPattern::setLyric(VocalNote *note, const std::string &lyric)
{
	note->setLyric(lyric);
	emit movedNote();
}
