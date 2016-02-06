/*
 * PianoKeyboard.h - declaration of PianoKeyboard,
 *			 to change note keymapping according to music theory
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

#ifndef __PIANOKEYBOARD_H__
#define __PIANOKEYBOARD_H__

#include <QString>

class PianoKeyboard
{
public:

	static int getLinuxKey( int key );
	static int getWindowsKey( int key );
	static int getAppleKey( int key );

	static int getScaledNoteByIndex( int index );

	static QString	getNoteByNumber( int index )	{ return m_notes[index]; }

	static int	getOctaveNumber()					{ return m_octave; }
	static void	setOctaveNumber( int octave )		{ m_octave = octave - 3; }

	static QString	getScaleNote()					{ return m_notes[m_scaleNoteIndex]; }
	static int	getScaleNoteIndex()					{ return m_scaleNoteIndex; }
	static void	setScaleNote( QString note );

	static QString getScaleName()				{ return scaleTypes[m_scaleType]; }
	static void	setScaleType( int scale )		{ m_scaleType = scale; }

	static bool isThereScale()	{ return (m_scaleType == 0) ? false : true; }

private:
	static int	m_octave;
	static int	m_scaleNoteIndex;
	static int	m_scaleType;

	static const QString scaleTypes[];

	static const int majorNotes[];		// W W H W W W H
	static const int minorNotes[];		// W H W W H W W
	static const int lydianNotes[];		// W W W H W W H

	static QString m_notes[];
};

#endif
