//
// Created by seledreams on 02/03/2021.
//

#include "VocalNote.h"

VocalNote::VocalNote() : Note()
{
	ResetVocalNote();
}
VocalNote::VocalNote(const Note &note)
	: Note(note)
{
	ResetVocalNote();
}
void VocalNote::ResetVocalNote()
{
	m_lyric = m_defaultlyric;
	m_modulation = 0;
	m_preutterance = 0;
	m_velocity = 0;
	m_overlap = 0;
	m_vibrato.fill(0);
	m_envelopeHeight.fill(0);
	m_envelopeWidth.fill(0);
	createDetuning();
	detuning()->automationPattern()->putValue(0,0);
	detuning()->automationPattern()->putValue(length(),0);
	detuning()->automationPattern()->setAutoResize(true);
}


void VocalNote::setLength(const TimePos &newLength)
{
	Note::setLength(newLength);
}
