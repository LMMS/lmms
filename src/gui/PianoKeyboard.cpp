/*
 * PianoKeyboard.cpp - Implementation of PianoKeyboard, used
 *			   to change keymapping of PC Keyboard for PianoRoll
 *
 * Copyright (c) 2015 - 2016 Özkan Afacan
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "PianoKeyboard.h"

int PianoKeyboard::m_octave = 2;
int PianoKeyboard::m_scaleNoteIndex = 0;
int PianoKeyboard::m_scaleType = 0;

const QString PianoKeyboard::scaleTypes[] =
{
	"No scale", "Major", "Harmonic minor", "Melodic minor",
	"Diminished", "Major bebop", "Dominant bebop", "Arabic",
	"Enigmatic", "Neopolitan", "Neopolitan minor", "Hungarian minor",
	"Dorian", "Phrygolydian", "Lydian", "Mixolydian", "Aeolian",
	"Locrian", "Minor", "Chromatic", "Half-Whole Diminished"
};

const int PianoKeyboard::majorNotes[] = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19};
const int PianoKeyboard::minorNotes[] = {0, 2, 3, 5, 7, 8, 10, 12, 14, 15, 17, 19};
const int PianoKeyboard::lydianNotes[] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19};

QString PianoKeyboard::m_notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

int PianoKeyboard::getLinuxKey( int key )
{
	if ( key < 38 && key >= 24 )
	{
		return getScaledNoteByIndex( key % 24 ) + 12 * m_octave + m_scaleNoteIndex;
	}
	else if ( key < 52 && key >= 38 )
	{
		return getScaledNoteByIndex( key % 38 ) + 12 * (m_octave - 1) + m_scaleNoteIndex;
	}
	else if ( key < 66 && key >= 52 )
	{
		return getScaledNoteByIndex( key % 52 ) + 12 * (m_octave - 2) + m_scaleNoteIndex;
	}
	else if ( key < 24 && key >= 10 )
	{
		return getScaledNoteByIndex( key - 10 ) + 12 * (m_octave + 1) + m_scaleNoteIndex;
	}
	else
	{
		return -100;
	}
}

int PianoKeyboard::getWindowsKey( int key )
{
	if ( key < 30 && key >= 16 )
	{
		return getScaledNoteByIndex( key % 16 ) + 12 * m_octave + m_scaleNoteIndex;
	}
	else if ( key < 44 && key >= 30 )
	{
		return getScaledNoteByIndex( key % 30 ) + 12 * (m_octave - 1) + m_scaleNoteIndex;
	}
	else if ( key < 58 && key >= 44 )
	{
		return getScaledNoteByIndex( key % 44 ) + 12 * (m_octave - 2) + m_scaleNoteIndex;
	}
	else if ( key < 16 && key >= 2 )
	{
		return getScaledNoteByIndex( key - 2 ) + 12 * (m_octave + 1) + m_scaleNoteIndex;
	}
	else
	{
		return -100;
	}
}

int PianoKeyboard::getAppleKey( int key )
{
	switch( key )
	{
		// Q Row
		case 12:	// Q
		case 13:	// W
		case 14:	// E
		case 15:	// R
			return getScaledNoteByIndex( key - 12 ) + 12 * m_octave + m_scaleNoteIndex;
		case 17:	// T
			return getScaledNoteByIndex( 4 ) + 12 * m_octave + m_scaleNoteIndex;
		case 16:	// Y
			return getScaledNoteByIndex( 5 ) + 12 * m_octave + m_scaleNoteIndex;
		case 32:	// U
			return getScaledNoteByIndex( 6 ) + 12 * m_octave + m_scaleNoteIndex;
		case 34:	// I
			return getScaledNoteByIndex( 7 ) + 12 * m_octave + m_scaleNoteIndex;
		case 31:	// O
			return getScaledNoteByIndex( 8 ) + 12 * m_octave + m_scaleNoteIndex;
		case 35:	// P
			return getScaledNoteByIndex( 9 ) + 12 * m_octave + m_scaleNoteIndex;
		case 33:	// [
			return getScaledNoteByIndex( 10 ) + 12 * m_octave + m_scaleNoteIndex;
		case 30:	// ]
			return getScaledNoteByIndex( 11 ) + 12 * m_octave + m_scaleNoteIndex;

		// A Row
		case 0:		// A
		case 1:		// S
		case 2:		// D
		case 3:		// F
			return getScaledNoteByIndex( key ) + 12 * (m_octave - 1) + m_scaleNoteIndex;
		case 5:		// G
			return getScaledNoteByIndex( 4 ) + 12 * (m_octave - 1) + m_scaleNoteIndex;
		case 4:		// H
			return getScaledNoteByIndex( 5 ) + 12 * (m_octave - 1) + m_scaleNoteIndex;
		case 38:	// J
			return getScaledNoteByIndex( 6 ) + 12 * (m_octave - 1) + m_scaleNoteIndex;
		case 40:	// K
			return getScaledNoteByIndex( 7 ) + 12 * (m_octave - 1) + m_scaleNoteIndex;
		case 37:	// L
			return getScaledNoteByIndex( 8 ) + 12 * (m_octave - 1) + m_scaleNoteIndex;
		case 41:	// ;
			return getScaledNoteByIndex( 9 ) + 12 * (m_octave - 1) + m_scaleNoteIndex;
		case 39:	// '
			return getScaledNoteByIndex( 10 ) + 12 * (m_octave - 1) + m_scaleNoteIndex;

		// Z Row
		case 6:		// Z
		case 7:		// X
		case 8:		// C
		case 9:		// V
			return getScaledNoteByIndex( key - 6 ) + 12 * (m_octave - 2) + m_scaleNoteIndex;
		case 11:	// B
			return getScaledNoteByIndex( 4 ) + 12 * (m_octave - 2) + m_scaleNoteIndex;
		case 45:	// N
			return getScaledNoteByIndex( 5 ) + 12 * (m_octave - 2) + m_scaleNoteIndex;
		case 46:	// M
			return getScaledNoteByIndex( 6 ) + 12 * (m_octave - 2) + m_scaleNoteIndex;
		case 43:	// ,
			return getScaledNoteByIndex( 7 ) + 12 * (m_octave - 2) + m_scaleNoteIndex;
		case 47:	// .
			return getScaledNoteByIndex( 8 ) + 12 * (m_octave - 2) + m_scaleNoteIndex;
		case 44:	// /
			return getScaledNoteByIndex( 9 ) + 12 * (m_octave - 2) + m_scaleNoteIndex;

		// Numbers Row
		case 18:	// 1
		case 19:	// 2
		case 20:	// 3
		case 21:	// 4
			return getScaledNoteByIndex( key - 18 ) + 12 * (m_octave + 1) + m_scaleNoteIndex;
		case 23:	// 5
			return getScaledNoteByIndex( 4 ) + 12 * (m_octave + 1) + m_scaleNoteIndex;
		case 22:	// 6
			return getScaledNoteByIndex( 5 ) + 12 * (m_octave + 1) + m_scaleNoteIndex;
		case 26:	// 7
			return getScaledNoteByIndex( 6 ) + 12 * (m_octave + 1) + m_scaleNoteIndex;
		case 28:	// 8
			return getScaledNoteByIndex( 7 ) + 12 * (m_octave + 1) + m_scaleNoteIndex;
		case 25:	// 9
			return getScaledNoteByIndex( 8 ) + 12 * (m_octave + 1) + m_scaleNoteIndex;
		case 29:	// 0
			return getScaledNoteByIndex( 9 ) + 12 * (m_octave + 1) + m_scaleNoteIndex;
		case 27:	// -
			return getScaledNoteByIndex( 10 ) + 12 * (m_octave + 1) + m_scaleNoteIndex;
		case 24:	// =
			return getScaledNoteByIndex( 11 ) + 12 * (m_octave + 1) + m_scaleNoteIndex;

		default: return -100;
	}
}

int PianoKeyboard::getScaledNoteByIndex( int index )
{
	if ( getScaleName() == "Major" ) 		{ return majorNotes[index]; }
	else if ( getScaleName() == "Minor" ) 	{ return minorNotes[index]; }
	else if ( getScaleName() == "Lydian" )	{ return lydianNotes[index]; }
	return 0;
}

void PianoKeyboard::setScaleNote( QString note )
{
	if ( note == "C" ) 			{ m_scaleNoteIndex = 0; }
	else if ( note == "C#" ) 	{ m_scaleNoteIndex = 1; }
	else if ( note == "D" )		{ m_scaleNoteIndex = 2; }
	else if ( note == "D#" )	{ m_scaleNoteIndex = 3; }
	else if ( note == "E" )		{ m_scaleNoteIndex = 4; }
	else if ( note == "F" )		{ m_scaleNoteIndex = 5; }
	else if ( note == "F#" )	{ m_scaleNoteIndex = 6; }
	else if ( note == "G" )		{ m_scaleNoteIndex = 7; }
	else if ( note == "G#" )	{ m_scaleNoteIndex = 8; }
	else if ( note == "A" )		{ m_scaleNoteIndex = 9; }
	else if ( note == "A#" )	{ m_scaleNoteIndex = 10; }
	else if ( note == "B" )		{ m_scaleNoteIndex = 11; }
}
