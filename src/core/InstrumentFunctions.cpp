/*
 * InstrumentFunctions.cpp - models for instrument-function-tab
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

#include <QDomElement>

#include "Engine.h"
#include "InstrumentFunctions.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "NotePlayHandle.h"
#include "PresetPreviewPlayHandle.h"
#include "embed.h"

// Chord constructors
InstrumentFunctionNoteStacking::Chord::Chord(const char *n,
                                             const ChordSemiTones semi_tones) {
  m_name = QString(*n);
  m_semiTones = semi_tones;
}
InstrumentFunctionNoteStacking::Chord::Chord(QString n,
                                             const ChordSemiTones semi_tones) {
  m_name = n;
  m_semiTones = semi_tones;
}

InstrumentFunctionNoteStacking::Chord::Chord(const char *n, QString s) {
  m_name = QString(*n);
  m_semiTones = ChordSemiTones(s);
}

InstrumentFunctionNoteStacking::Chord::Chord(QString n, QString s) {
  m_name = n;
  m_semiTones = ChordSemiTones(s);
}

/**
 * Initializing the inbound static ChordTable vector
 *
 * @brief InstrumentFunctionNoteStacking::ChordTable::Init::Init
 */
InstrumentFunctionNoteStacking::ChordTable::Init::Init() {
  /*
   * Char delimiter for name: $
   * Chord delimiter : |
   * Semitone data:  a,b,c,d,e,f;
   *   a: key (distance in semitones from the original note, can be negative)
   *   b: volume - from 0 to 200 (the uint, but here in percentage??) - value
   * accepted from 0 to 200%; 100 same value
   *   c: panning - from -100 to 100 (or in percentage??)- from -100 to 100%; 0
   * centers
   *   d: active (true/false) - if the note is skipped (false)
   *   e: silenced (true/false) - if the note is played (false) or there is
   * silence (true)
   *   f: bare (true/false) - if the note only holds the key, all other data is
   * ignored
   */
  QString notes =
      "octave$ 0,100,0,1,0,0;|"
      "Major$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0;|"
      "Majb5$ 0,100,0,1,0,0; 4,100,0,1,0,0; 6,100,0,1,0,0;|"
      "minor$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0;|"
      "minb5$ 0,100,0,1,0,0; 3,100,0,1,0,0; 6,100,0,1,0,0;|"
      "sus2$ 0,100,0,1,0,0; 2,100,0,1,0,0; 7,100,0,1,0,0;|"
      "sus4$ 0,100,0,1,0,0; 5,100,0,1,0,0; 7,100,0,1,0,0;|"
      "aug$ 0,100,0,1,0,0; 4,100,0,1,0,0; 8,100,0,1,0,0;|"
      "augsus4$ 0,100,0,1,0,0; 5,100,0,1,0,0; 8,100,0,1,0,0;|"
      "tri$ 0,100,0,1,0,0; 3,100,0,1,0,0; 6,100,0,1,0,0; 9,100,0,1,0,0;|"

      "6$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 9,100,0,1,0,0;|"
      "6sus4$ 0,100,0,1,0,0; 5,100,0,1,0,0; 7,100,0,1,0,0; 9,100,0,1,0,0;|"
      "6add9$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 9,100,0,1,0,0; "
      "14,100,0,1,0,0;|"
      "m6$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; 9,100,0,1,0,0;|"
      "m6add9$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; 9,100,0,1,0,0; "
      "14,100,0,1,0,0;|"

      "7$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0;|"
      "7sus4$ 0,100,0,1,0,0; 5,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0;|"
      "7#5$ 0,100,0,1,0,0; 4,100,0,1,0,0; 8,100,0,1,0,0; 10,100,0,1,0,0;|"
      "7b5$ 0,100,0,1,0,0; 4,100,0,1,0,0; 6,100,0,1,0,0; 10,100,0,1,0,0;|"
      "7#9$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0;"
      "15,100,0,1,0,0;|"
      "7b9$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "13,100,0,1,0,0;|"
      "7#5#9$ 0,100,0,1,0,0; 4,100,0,1,0,0; 8,100,0,1,0,0; 10,100,0,1,0,0; "
      "15,100,0,1,0,0;|"
      "7#5b9$ 0,100,0,1,0,0; 4,100,0,1,0,0; 8,100,0,1,0,0; 10,100,0,1,0,0; "
      "13,100,0,1,0,0;|"
      "7b5b9$ 0,100,0,1,0,0; 4,100,0,1,0,0; 6,100,0,1,0,0; 10,100,0,1,0,0; "
      "13,100,0,1,0,0;|"
      "7add11$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "17,100,0,1,0,0;|"
      "7add13$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "21,100,0,1,0,0;|"
      "7#11$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "18,100,0,1,0,0;|"
      "Maj7$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 11,100,0,1,0,0;|"
      "Maj7b5$ 0,100,0,1,0,0; 4,100,0,1,0,0; 6,100,0,1,0,0; 11,100,0,1,0,0;|"
      "Maj7#5$ 0,100,0,1,0,0; 4,100,0,1,0,0; 8,100,0,1,0,0; 11,100,0,1,0,0;|"
      "Maj7#11$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 11,100,0,1,0,0; "
      "18,100,0,1,0,0;|"
      "Maj7add13$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 11,100,0,1,0,0; "
      "21,100,0,1,0,0;|"
      "m7$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0;|"
      "m7b5$ 0,100,0,1,0,0; 3,100,0,1,0,0; 6,100,0,1,0,0; 10,100,0,1,0,0;|"
      "m7b9$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "13,100,0,1,0,0;|"
      "m7add11$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "17,100,0,1,0,0;|"
      "m7add13$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "21,100,0,1,0,0;|"
      "m-Maj7$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; 11,100,0,1,0,0;|"
      "m-Maj7add11$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; "
      "11,100,0,1,0,0; 17,100,0,1,0,0;|"
      "m-Maj7add13$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; "
      "11,100,0,1,0,0; 21,100,0,1,0,0;|"

      "9$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "14,100,0,1,0,0;|"
      "9sus4$ 0,100,0,1,0,0; 5,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "14,100,0,1,0,0;|"
      "add9$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 14,100,0,1,0,0;|"
      "9#5$ 0,100,0,1,0,0; 4,100,0,1,0,0; 8,100,0,1,0,0; 10,100,0,1,0,0; "
      "14,100,0,1,0,0;|"
      "9b5$ 0,100,0,1,0,0; 4,100,0,1,0,0; 6,100,0,1,0,0; 10,100,0,1,0,0; "
      "14,100,0,1,0,0;|"
      "9#11$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "14,100,0,1,0,0; 18,100,0,1,0,0;|"
      "9b13$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "14,100,0,1,0,0; 20,100,0,1,0,0;|"
      "Maj9$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 11,100,0,1,0,0; "
      "14,100,0,1,0,0;|"
      "Maj9sus4$ 0,100,0,1,0,0; 5,100,0,1,0,0; 7,100,0,1,0,0; 11,100,0,1,0,0; "
      "15,100,0,1,0,0;|"
      "Maj9#5$ 0,100,0,1,0,0; 4,100,0,1,0,0; 8,100,0,1,0,0; 11,100,0,1,0,0; "
      "14,100,0,1,0,0;|"
      "Maj9#11$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 11,100,0,1,0,0; "
      "14,100,0,1,0,0; 18,100,0,1,0,0;|"
      "m9$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "14,100,0,1,0,0;|"
      "madd9$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; 14,100,0,1,0,0;|"
      "m9b5$ 0,100,0,1,0,0; 3,100,0,1,0,0; 6,100,0,1,0,0; 10,100,0,1,0,0; "
      "14,100,0,1,0,0;|"
      "m9-Maj7$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; 11,100,0,1,0,0; "
      "14,100,0,1,0,0;|"

      "11$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "14,100,0,1,0,0; 17,100,0,1,0,0;|"
      "11b9$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "13,100,0,1,0,0; 17,100,0,1,0,0;|"
      "Maj11$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 11,100,0,1,0,0; "
      "14,100,0,1,0,0; 17,100,0,1,0,0;|"
      "m11$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "14,100,0,1,0,0; 17,100,0,1,0,0;|"
      "m-Maj11$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; 11,100,0,1,0,0; "
      "14,100,0,1,0,0; 17,100,0,1,0,0;|"

      "13$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "14,100,0,1,0,0; 21,100,0,1,0,0;|"
      "13#9$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "15,100,0,1,0,0; 21,100,0,1,0,0;|"
      "13b9$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "13,100,0,1,0,0; 21,100,0,1,0,0;|"
      "13b5b9$ 0,100,0,1,0,0; 4,100,0,1,0,0; 6,100,0,1,0,0; 10,100,0,1,0,0; "
      "13,100,0,1,0,0; 21,100,0,1,0,0;|"
      "Maj13$ 0,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 11,100,0,1,0,0; "
      "14,100,0,1,0,0; 21,100,0,1,0,0;|"
      "m13$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; 10,100,0,1,0,0; "
      "14,100,0,1,0,0; 21,100,0,1,0,0;|"
      "m-Maj13$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0; 11,100,0,1,0,0; "
      "14,100,0,1,0,0; 21,100,0,1,0,0;|"

      "Major$ 0,100,0,1,0,0; 2,100,0,1,0,0; 4,100,0,1,0,0; 5,100,0,1,0,0; "
      "7,100,0,1,0,0; 9,100,0,1,0,0; 11,100,0,1,0,0;|"
      "Harmonic minor$ 0,100,0,1,0,0; 2,100,0,1,0,0; 3,100,0,1,0,0; "
      "5,100,0,1,0,0; 7,100,0,1,0,0; 8,100,0,1,0,0; 11,100,0,1,0,0;|"
      "Melodic minor$ 0,100,0,1,0,0; 2,100,0,1,0,0; 3,100,0,1,0,0; "
      "5,100,0,1,0,0; 7,100,0,1,0,0; 9,100,0,1,0,0; 11,100,0,1,0,0;|"
      "Whole tone$ 0,100,0,1,0,0; 2,100,0,1,0,0; 4,100,0,1,0,0; 6,100,0,1,0,0; "
      "8,100,0,1,0,0; 10,100,0,1,0,0;|"
      "Diminished$ 0,100,0,1,0,0; 2,100,0,1,0,0; 3,100,0,1,0,0; 5,100,0,1,0,0; "
      "6,100,0,1,0,0; 8,100,0,1,0,0; 9,100,0,1,0,0; 11,100,0,1,0,0;|"
      "Major pentatonic$ 0,100,0,1,0,0; 2,100,0,1,0,0; 4,100,0,1,0,0; "
      "7,100,0,1,0,0; 9,100,0,1,0,0;|"
      "Minor pentatonic$ 0,100,0,1,0,0; 3,100,0,1,0,0; 5,100,0,1,0,0; "
      "7,100,0,1,0,0; 10,100,0,1,0,0;|"
      "Jap in sen$ 0,100,0,1,0,0; 1,100,0,1,0,0; 5,100,0,1,0,0; 7,100,0,1,0,0; "
      "10,100,0,1,0,0;|"
      "Major bebop$ 0,100,0,1,0,0; 2,100,0,1,0,0; 4,100,0,1,0,0; "
      "5,100,0,1,0,0; 7,100,0,1,0,0; 8,100,0,1,0,0; 9,100,0,1,0,0; "
      "11,100,0,1,0,0;|"
      "Dominant bebop$ 0,100,0,1,0,0; 2,100,0,1,0,0; 4,100,0,1,0,0; "
      "5,100,0,1,0,0; 7,100,0,1,0,0; 9,100,0,1,0,0; 10,100,0,1,0,0; "
      "11,100,0,1,0,0;|"
      "Blues$ 0,100,0,1,0,0; 3,100,0,1,0,0; 5,100,0,1,0,0; 6,100,0,1,0,0; "
      "7,100,0,1,0,0; 10,100,0,1,0,0;|"
      "Arabic$ 0,100,0,1,0,0; 1,100,0,1,0,0; 4,100,0,1,0,0; 5,100,0,1,0,0; "
      "7,100,0,1,0,0; 8,100,0,1,0,0; 11,100,0,1,0,0;|"
      "Enigmatic$ 0,100,0,1,0,0; 1,100,0,1,0,0; 4,100,0,1,0,0; 6,100,0,1,0,0; "
      "8,100,0,1,0,0; 10,100,0,1,0,0; 11,100,0,1,0,0;|"
      "Neopolitan$ 0,100,0,1,0,0; 1,100,0,1,0,0; 3,100,0,1,0,0; 5,100,0,1,0,0; "
      "7,100,0,1,0,0; 9,100,0,1,0,0; 11,100,0,1,0,0;|"
      "Neopolitan minor$ 0,100,0,1,0,0; 1,100,0,1,0,0; 3,100,0,1,0,0; "
      "5,100,0,1,0,0; 7,100,0,1,0,0; 8,100,0,1,0,0; 11,100,0,1,0,0;|"
      "Hungarian minor$ 0,100,0,1,0,0; 2,100,0,1,0,0; 3,100,0,1,0,0; "
      "6,100,0,1,0,0; 7,100,0,1,0,0; 8,100,0,1,0,0; 11,100,0,1,0,0;|"
      "Dorian$ 0,100,0,1,0,0; 2,100,0,1,0,0; 3,100,0,1,0,0; 5,100,0,1,0,0; "
      "7,100,0,1,0,0; 9,100,0,1,0,0; 10,100,0,1,0,0;|"
      "Phrygolydian$ 0,100,0,1,0,0; 1,100,0,1,0,0; 3,100,0,1,0,0; "
      "5,100,0,1,0,0; 7,100,0,1,0,0; 8,100,0,1,0,0; 10,100,0,1,0,0;|"
      "Lydian$ 0,100,0,1,0,0; 2,100,0,1,0,0; 4,100,0,1,0,0; 6,100,0,1,0,0; "
      "7,100,0,1,0,0; 9,100,0,1,0,0; 11,100,0,1,0,0;|"
      "Mixolydian$ 0,100,0,1,0,0; 2,100,0,1,0,0; 4,100,0,1,0,0; 5,100,0,1,0,0; "
      "7,100,0,1,0,0; 9,100,0,1,0,0; 10,100,0,1,0,0;|"
      "Aeolian$ 0,100,0,1,0,0; 2,100,0,1,0,0; 3,100,0,1,0,0; 5,100,0,1,0,0; "
      "7,100,0,1,0,0; 8,100,0,1,0,0; 10,100,0,1,0,0;|"
      "Locrian$ 0,100,0,1,0,0; 1,100,0,1,0,0; 3,100,0,1,0,0; 5,100,0,1,0,0; "
      "6,100,0,1,0,0; 8,100,0,1,0,0; 10,100,0,1,0,0;|"
      "Minor$ 0,100,0,1,0,0; 2,100,0,1,0,0; 3,100,0,1,0,0; 5,100,0,1,0,0; "
      "7,100,0,1,0,0; 8,100,0,1,0,0; 10,100,0,1,0,0;|"
      "Chromatic$ 0,100,0,1,0,0; 1,100,0,1,0,0; 2,100,0,1,0,0; 3,100,0,1,0,0; "
      "4,100,0,1,0,0; 5,100,0,1,0,0; 6,100,0,1,0,0; 7,100,0,1,0,0; "
      "8,100,0,1,0,0; 9,100,0,1,0,0; 10,100,0,1,0,0; 11,100,0,1,0,0;|"
      "Half-Whole Diminished$ 0,100,0,1,0,0; 1,100,0,1,0,0; 3,100,0,1,0,0; "
      "4,100,0,1,0,0; 6,100,0,1,0,0; 7,100,0,1,0,0; 9,100,0,1,0,0; "
      "10,100,0,1,0,0;|"

      "5$ 0,100,0,1,0,0; 7,100,0,1,0,0;|"

      "Vera 477044$ 4,100,0,1,0,0; 7,100,0,1,0,0; 7,100,0,1,0,0; "
      "0,100,0,1,0,0; 4,100,0,1,0,0; 4,100,0,1,0,0;|"
      "Rik 00224422$ 0,100,0,1,0,0; 0,100,0,1,0,0; 2,100,0,1,0,0; "
      "2,100,0,1,0,0; 4,100,0,1,0,0; 4,100,0,1,0,0; 2,100,0,1,0,0; "
      "2,100,0,1,0,0;|"
      "Rik 00335522$ 0,100,0,1,0,0; 0,100,0,1,0,0; 3,100,0,1,0,0; "
      "3,100,0,1,0,0; 5,100,0,1,0,0; 5,100,0,1,0,0; 2,100,0,1,0,0; "
      "2,100,0,1,0,0;|"
      "Rik 0245$ 0,100,0,1,0,0; 2,100,0,1,0,0; 4,100,0,1,0,0; 5,100,0,1,0,0;|"
      "Rik 0-2-45$ 0,100,0,1,0,0; -2,100,0,1,0,0; -4,100,0,1,0,0; "
      "5,100,0,1,0,0;|"
      "Rik 07294_11_4$ 0,100,0,1,0,0; 7,100,0,1,0,0; 2,100,0,1,0,0; "
      "9,100,0,1,0,0; 4,100,0,1,0,0; 11,100,0,1,0,0; 4,100,0,1,0,0;|"
      "Rik 08$ 0,100,0,1,0,0; 8,100,0,1,0,0;|"
      "Rik 07$ 0,100,0,1,0,0; 7,100,0,1,0,0;|"
      "Rik 06$ 0,100,0,1,0,0; 6,100,0,1,0,0;|"
      "Rik 037$ 0,100,0,1,0,0; 3,100,0,1,0,0; 7,100,0,1,0,0;|"
      "Rik 058$ 0,100,0,1,0,0; 5,100,0,1,0,0; 8,100,0,1,0,0;|"
      "Rik 085$ 0,100,0,1,0,0; 8,100,0,1,0,0; 5,100,0,1,0,0;|"
      "Rik 04_10$ 0,100,0,1,0,0; 4,100,0,1,0,0; 10,100,0,1,0,0;|"
      "Rik 0_12_57$ 0,100,0,1,0,0; 12,100,0,1,0,0; 5,100,0,1,0,0; "
      "7,100,0,1,0,0;|"
      "Rik 0_12_75$ 0,100,0,1,0,0; 12,100,0,1,0,0; 7,100,0,1,0,0; "
      "5,100,0,1,0,0;|"
      "Rik 085_12$ 0,100,0,1,0,0; 8,100,0,1,0,0; 5,100,0,1,0,0; "
      "12,100,0,1,0,0;|"
      "Rik 0735$ 0,100,0,1,0,0; 7,100,0,1,0,0; 3,100,0,1,0,0; 5,100,0,1,0,0;|"
      "Rik 0.-2.0.2.1$ 0,100,0,1,0,0; -2,100,0,1,0,0; 0,100,0,1,0,0; "
      "2,100,0,1,0,0; 1,100,0,1,0,0;|"
      "Rik 0.-5.-3.-3.-3.-5$ 0,100,0,1,0,0; -5,100,0,1,0,0; -3,100,0,1,0,0; "
      "-3,100,0,1,0,0; -3,100,0,1,0,0; -5,100,0,1,0,0;|"
      "Rik 0.0.3.3.5.5.2.2$ 0,100,0,1,0,0; 0,100,0,1,0,0; 3,100,0,1,0,0; "
      "3,100,0,1,0,0; 5,100,0,1,0,0; 5,100,0,1,0,0; 2,100,0,1,0,0; "
      "2,100,0,1,0,0;|"
      "Rik 0.3.5.2$ 0,100,0,1,0,0; 3,100,0,1,0,0; 5,100,0,1,0,0; "
      "2,100,0,1,0,0;|"
      "Rik 0.16$ 0,100,0,1,0,0; 16,100,0,1,0,0;|"
      "Rik 0 10 7 10$ 0,100,0,1,0,0; 10,100,0,1,0,0; 7,100,0,1,0,0; "
      "10,100,0,1,0,0;|"
      "Rik 0 16 10 11$ 0,100,0,1,0,0; 16,100,0,1,0,0; 10,100,0,1,0,0; "
      "11,100,0,1,0,0;|"
      "Rik 0.7.-1.3$ 0,100,0,1,0,0; 7,100,0,1,0,0; -1,100,0,1,0,0; "
      "3,100,0,1,0,0;|"
      "Rik 0.7.-1.3.-3.1$ 0,100,0,1,0,0; 7,100,0,1,0,0; -1,100,0,1,0,0; "
      "3,100,0,1,0,0; -3,100,0,1,0,0; 1,100,0,1,0,0;|"
      "Rik 0.0.0.-3.4.4.4$ 1,100,0,1,0,0; 1,100,0,1,0,0; 1,100,0,1,0,0; "
      "-2,100,0,1,0,0; 5,100,0,1,0,0; 5,100,0,1,0,0; 5,100,0,1,0,0; "
      "0,100,0,1,0,0;|"
      "Rik 3.0.0.7.7.2.2.3$ 3,100,0,1,0,0; 0,100,0,1,0,0; 0,100,0,1,0,0; "
      "7,100,0,1,0,0; 7,100,0,1,0,0; 2,100,0,1,0,0; 2,100,0,1,0,0; "
      "3,100,0,1,0,0;|"
      "Rik 0.7.2.3$ 0,100,0,1,0,0; 7,100,0,1,0,0; 2,100,0,1,0,0; "
      "3,100,0,1,0,0;|"
      "Rik 0.7.2.5$ 0,100,0,1,0,0; 7,100,0,1,0,0; 2,100,0,1,0,0; "
      "5,100,0,1,0,0;|"
      "Rik 0.7.3.5$ 0,100,0,1,0,0; 7,100,0,1,0,0; 3,100,0,1,0,0; "
      "5,100,0,1,0,0;|"
      "Rik 3.3.3.0.0.7.7.2$ 3,100,0,1,0,0; 3,100,0,1,0,0; 3,100,0,1,0,0; "
      "0,100,0,1,0,0; 0,100,0,1,0,0; 7,100,0,1,0,0; 7,100,0,1,0,0; "
      "2,100,0,1,0,0;|"
      "Rik 0.-1.-1.3.3.-7$ 0,100,0,1,0,0; -1,100,0,1,0,0; -1,100,0,1,0,0; "
      "3,100,0,1,0,0; 3,100,0,1,0,0; -7,100,0,1,0,0;|"
      "Rik 0.-1.-1.4.4.7.7.6$ 0,100,0,1,0,0; -1,100,0,1,0,0; -1,100,0,1,0,0; "
      "4,100,0,1,0,0; 4,100,0,1,0,0; 7,100,0,1,0,0; 7,100,0,1,0,0; "
      "6,100,0,1,0,0;|"

      "Phrygian dominant$ 0,100,0,1,0,0; 1,100,0,1,0,0; 4,100,0,1,0,0; "
      "5,100,0,1,0,0; 7,100,0,1,0,0; 8,100,0,1,0,0; 10,100,0,1,0,0;|"

      "Persian$ 0,100,0,1,0,0; 1,100,0,1,0,0; 4,100,0,1,0,0; 5,100,0,1,0,0; "
      "6,100,0,1,0,0; 8,100,0,1,0,0; 11,100,0,1,0,0;|"

      "pino$ 0,100,0,1,0,0; 3,100,50,1,0,0; 5,100,-100,1,0,0; "
      "-1,100,100,1,0,0|"

      "RikPan 0.0.0.-3.4.4.4$ 0,100,0,1,0,0; 0,50,40,1,0,0; 0,80,-40,1,0,0; "
      "-3,120,70,1,0,0; 4,100,-70,1,0,0; 4,30,0,1,0,0; 4,60,90,1,0,0; "
      "0,100,-90,1,0,0;|"
      "RikPan 3.0.0.7.7.2.2.3$ 3,100,60,1,0,0; 0,100,-60,1,0,0; 0,60,0,1,0,0; "
      "7,60,0,1,0,0; 7,110,-80,1,0,0; 2,80,80,1,0,0; 2,90,70,1,0,0; "
      "3,90,-70,1,0,0;|";

  // Getting the list of chordes
  QStringList l0 = notes.remove(' ').split('|'); // Splitting each chord
  foreach (QString s, l0) {
    if (s.isEmpty()) { // eliminating the string originated by the last
                       // character
      break;
    }

    // l[0]: name l[1]:string of the list
    QStringList l1 = s.split('$');

    // getting semitones from string contructor
    ChordSemiTones cs = ChordSemiTones(l1[1]);
    // using "tr" to allow translation
    Chord chord = Chord(tr(l1[0].toUtf8().constData()), cs);

    // populating the inbound static structure
    push_back(chord);
  }
};

/**
 * Recalling the function Init which gets the static vector data by expliciting
 * the function variable (??)
 *
 * @brief InstrumentFunctionNoteStacking::ChordTable::s_initializer
 */
InstrumentFunctionNoteStacking::ChordTable::Init
    InstrumentFunctionNoteStacking::ChordTable::s_initializer;

/**
 * The constructor transfers the initialized inbound static data to the
 * vector itself
 *
 * @brief InstrumentFunctionNoteStacking::ChordTable::ChordTable
 */
InstrumentFunctionNoteStacking::ChordTable::ChordTable() : QVector<Chord>() {
  // Swaps the Chordable uninitialized vector with the static ChordTable one;
  swap(s_initializer);
}

/**
 * The original function maintained, it only checks for the same key, other
 * characteristics are neglected to maintain compatibility with other modules
 *
 * @brief InstrumentFunctionNoteStacking::Chord::hasSemiTone
 * @param semi_tone
 * @return
 */
bool InstrumentFunctionNoteStacking::Chord::hasSemiTone(
    int8_t semi_tone) const {
  for (int i = 0; i < size(); ++i) {
    // Using overloaded operator [] to return int8_t
    if (semi_tone == m_semiTones[i]) {
      return true;
    }
  }
  return false;
}

const InstrumentFunctionNoteStacking::Chord &
InstrumentFunctionNoteStacking::ChordTable::getByName(const QString &name,
                                                      bool is_scale) const {
  for (int i = 0; i < size(); i++) {
    if (at(i).getName() == name && is_scale == at(i).isScale())
      return at(i);
  }

  static Chord empty;
  return empty;
}

InstrumentFunctionNoteStacking::InstrumentFunctionNoteStacking(Model *_parent)
    : Model(_parent, tr("Chords")), m_chordsEnabledModel(false, this),
      m_chordsModel(this, tr("Chord type")),
      m_chordRangeModel(1.0f, 1.0f, 9.0f, 1.0f, this, tr("Chord range")) {
  const ChordTable &chord_table = ChordTable::getInstance();
  for (int i = 0; i < chord_table.size(); ++i) {
    m_chordsModel.addItem(chord_table[i].getName());
  }
}

InstrumentFunctionNoteStacking::~InstrumentFunctionNoteStacking() {}

void InstrumentFunctionNoteStacking::processNote(NotePlayHandle *_n) {
  // Getting base note key
  const int base_note_key = _n->key();
  // Getting base note volume and panning
  const volume_t base_note_vol = _n->getVolume();
  const panning_t base_note_pan = _n->getPanning();
  //
  const ChordTable &chord_table = ChordTable::getInstance();
  // we add chord-subnotes to note if either note is a base-note and
  // arpeggio is not used or note is part of an arpeggio
  // at the same time we only add sub-notes if nothing of the note was
  // played yet, because otherwise we would add chord-subnotes every
  // time an audio-buffer is rendered...
  if ((_n->origin() == NotePlayHandle::OriginArpeggio ||
       (_n->hasParent() == false &&
        _n->instrumentTrack()->isArpeggioEnabled() == false)) &&
      _n->totalFramesPlayed() == 0 && m_chordsEnabledModel.value() == true &&
      !_n->isReleased()) {

    //  the selected value!
    //  then insert sub-notes for chord
    const int selected_chord = m_chordsModel.value();

    for (int octave_cnt = 0; octave_cnt < m_chordRangeModel.value();
         ++octave_cnt) {
      const int sub_note_key_base = base_note_key + octave_cnt * KeysPerOctave;

      Chord c = chord_table[selected_chord];
      // process all notes in the chord
      foreach (ChordSemiTone cst, c.getChordSemiTones()) {
        if (cst.active) { // if the note is active process it, otherwise skip

          // getting the base note key
          const int sub_note_key = sub_note_key_base + cst.key;

          // the new volume and panning
          volume_t sub_note_vol;
          panning_t sub_note_pan;

          if (cst.bare) { // forget other settings but key, get the original
                          // data
            sub_note_vol = base_note_vol;
            sub_note_pan = base_note_pan;
          } else {
            // The note is silenced: playing it with no volume
            if (cst.silenced) {
              sub_note_vol = 0;
              sub_note_pan = 0;
            } else { // all modifications active, add interval to sub-note-key,
                     // volume, panning
              sub_note_vol = base_note_vol * ((float)cst.vol / (float)100);
              sub_note_pan = cst.pan;
            }
          }
          // maybe we're out of range -> let's get outta
          // here!
          if (sub_note_key > NumKeys || sub_note_key < 0 ||
              Engine::mixer()->criticalXRuns()) {
            break;
          }
          // create copy of base-note
          Note note_copy(_n->length(), 0, sub_note_key, sub_note_vol,
                         sub_note_pan, _n->detuning());

          // create sub-note-play-handle, only note is
          // different
          Engine::mixer()->addPlayHandle(NotePlayHandleManager::acquire(
              _n->instrumentTrack(), _n->offset(), _n->frames(), note_copy, _n,
              -1, NotePlayHandle::OriginNoteStacking));
        }
      }
    }
  }
}

void InstrumentFunctionNoteStacking::saveSettings(QDomDocument &_doc,
                                                  QDomElement &_this) {
  m_chordsEnabledModel.saveSettings(_doc, _this, "chord-enabled");
  m_chordsModel.saveSettings(_doc, _this, "chord");
  m_chordRangeModel.saveSettings(_doc, _this, "chordrange");
}

void InstrumentFunctionNoteStacking::loadSettings(const QDomElement &_this) {
  m_chordsEnabledModel.loadSettings(_this, "chord-enabled");
  m_chordsModel.loadSettings(_this, "chord");
  m_chordRangeModel.loadSettings(_this, "chordrange");
}

InstrumentFunctionArpeggio::InstrumentFunctionArpeggio(Model *_parent)
    : Model(_parent, tr("Arpeggio")), m_arpEnabledModel(false),
      m_arpModel(this, tr("Arpeggio type")),
      m_arpRangeModel(1.0f, 1.0f, 9.0f, 1.0f, this, tr("Arpeggio range")),
      m_arpCycleModel(0.0f, 0.0f, 6.0f, 1.0f, this, tr("Cycle steps")),
      m_arpSkipModel(0.0f, 0.0f, 100.0f, 1.0f, this, tr("Skip rate")),
      m_arpMissModel(0.0f, 0.0f, 100.0f, 1.0f, this, tr("Miss rate")),
      m_arpTimeModel(100.0f, 25.0f, 2000.0f, 1.0f, 2000, this,
                     tr("Arpeggio time")),
      m_arpGateModel(100.0f, 1.0f, 200.0f, 1.0f, this, tr("Arpeggio gate")),
      m_arpDirectionModel(this, tr("Arpeggio direction")),
      m_arpModeModel(this, tr("Arpeggio mode")) {
  const InstrumentFunctionNoteStacking::ChordTable &chord_table =
      InstrumentFunctionNoteStacking::ChordTable::getInstance();
  for (int i = 0; i < chord_table.size(); ++i) {
    m_arpModel.addItem(chord_table[i].getName());
  }

  m_arpDirectionModel.addItem(tr("Up"), new PixmapLoader("arp_up"));
  m_arpDirectionModel.addItem(tr("Down"), new PixmapLoader("arp_down"));
  m_arpDirectionModel.addItem(tr("Up and down"),
                              new PixmapLoader("arp_up_and_down"));
  m_arpDirectionModel.addItem(tr("Random"), new PixmapLoader("arp_random"));
  m_arpDirectionModel.addItem(tr("Down and up"),
                              new PixmapLoader("arp_up_and_down"));
  m_arpDirectionModel.setInitValue(ArpDirUp);

  m_arpModeModel.addItem(tr("Free"), new PixmapLoader("arp_free"));
  m_arpModeModel.addItem(tr("Sort"), new PixmapLoader("arp_sort"));
  m_arpModeModel.addItem(tr("Sync"), new PixmapLoader("arp_sync"));
}

InstrumentFunctionArpeggio::~InstrumentFunctionArpeggio() {}

void InstrumentFunctionArpeggio::processNote(NotePlayHandle *_n) {
  // getting base note key
  const int base_note_key = _n->key();
  // Getting volume and panning
  const volume_t base_note_vol = _n->getVolume();
  const panning_t base_note_pan = _n->getPanning();

  if (_n->origin() == NotePlayHandle::OriginArpeggio ||
      _n->origin() == NotePlayHandle::OriginNoteStacking ||
      !m_arpEnabledModel.value() ||
      (_n->isReleased() &&
       _n->releaseFramesDone() >= _n->actualReleaseFramesToDo())) {
    return;
  }

  const int selected_arp = m_arpModel.value();

  ConstNotePlayHandleList cnphv =
      NotePlayHandle::nphsOfInstrumentTrack(_n->instrumentTrack());

  if (m_arpModeModel.value() != FreeMode && cnphv.size() == 0) {
    // maybe we're playing only a preset-preview-note?
    cnphv =
        PresetPreviewPlayHandle::nphsOfInstrumentTrack(_n->instrumentTrack());
    if (cnphv.size() == 0) {
      // still nothing found here, so lets return
      // return;
      cnphv.push_back(_n);
    }
  }

  const InstrumentFunctionNoteStacking::ChordTable &chord_table =
      InstrumentFunctionNoteStacking::ChordTable::getInstance();
  const int cur_chord_size = chord_table[selected_arp].size();
  const int range = (int)(cur_chord_size * m_arpRangeModel.value());
  const int total_range = range * cnphv.size();

  // number of frames that every note should be played
  const f_cnt_t arp_frames = (f_cnt_t)(m_arpTimeModel.value() / 1000.0f *
                                       Engine::mixer()->processingSampleRate());
  const f_cnt_t gated_frames =
      (f_cnt_t)(m_arpGateModel.value() * arp_frames / 100.0f);

  // used for calculating remaining frames for arp-note, we have to add
  // arp_frames-1, otherwise the first arp-note will not be setup
  // correctly... -> arp_frames frames silence at the start of every note!
  int cur_frame =
      ((m_arpModeModel.value() != FreeMode) ? cnphv.first()->totalFramesPlayed()
                                            : _n->totalFramesPlayed()) +
      arp_frames - 1;
  // used for loop
  f_cnt_t frames_processed = (m_arpModeModel.value() != FreeMode)
                                 ? cnphv.first()->noteOffset()
                                 : _n->noteOffset();

  while (frames_processed < Engine::mixer()->framesPerPeriod()) {
    const f_cnt_t remaining_frames_for_cur_arp =
        arp_frames - (cur_frame % arp_frames);
    // does current arp-note fill whole audio-buffer?
    if (remaining_frames_for_cur_arp > Engine::mixer()->framesPerPeriod()) {
      // then we don't have to do something!
      break;
    }

    frames_processed += remaining_frames_for_cur_arp;

    // init with zero
    int cur_arp_idx = 0;

    // in sorted mode: is it our turn or do we have to be quiet for
    // now?
    if (m_arpModeModel.value() == SortMode &&
        ((cur_frame / arp_frames) % total_range) / range !=
            (f_cnt_t)_n->index()) {
      // update counters
      frames_processed += arp_frames;
      cur_frame += arp_frames;
      continue;
    }

    // Skip notes randomly
    if (m_arpSkipModel.value()) {

      if (100 * ((float)rand() / (float)(RAND_MAX + 1.0f)) <
          m_arpSkipModel.value()) {
        if (cur_arp_idx == 0) {
          _n->setMasterNote();
        }
        // update counters
        frames_processed += arp_frames;
        cur_frame += arp_frames;
        continue;
      }
    }

    int dir = m_arpDirectionModel.value();

    // Miss notes randomly. We intercept int dir and abuse it
    // after need.  :)

    if (m_arpMissModel.value()) {
      if (100 * ((float)rand() / (float)(RAND_MAX + 1.0f)) <
          m_arpMissModel.value()) {
        dir = ArpDirRandom;
      }
    }

    // process according to arpeggio-direction...
    if (dir == ArpDirUp) {
      cur_arp_idx = (cur_frame / arp_frames) % range;
    } else if (dir == ArpDirDown) {
      cur_arp_idx = range - (cur_frame / arp_frames) % range - 1;
    } else if (dir == ArpDirUpAndDown && range > 1) {
      // imagine, we had to play the arp once up and then
      // once down -> makes 2 * range possible notes...
      // because we don't play the lower and upper notes
      // twice, we have to subtract 2
      cur_arp_idx = (cur_frame / arp_frames) % (range * 2 - 2);
      // if greater than range, we have to play down...
      // looks like the code for arp_dir==DOWN... :)
      if (cur_arp_idx >= range) {
        cur_arp_idx = range - cur_arp_idx % (range - 1) - 1;
      }
    } else if (dir == ArpDirDownAndUp && range > 1) {
      // copied from ArpDirUpAndDown above
      cur_arp_idx = (cur_frame / arp_frames) % (range * 2 - 2);
      // if greater than range, we have to play down...
      // looks like the code for arp_dir==DOWN... :)
      if (cur_arp_idx >= range) {
        cur_arp_idx = range - cur_arp_idx % (range - 1) - 1;
      }
      // inverts direction
      cur_arp_idx = range - cur_arp_idx - 1;
    } else if (dir == ArpDirRandom) {
      // just pick a random chord-index
      cur_arp_idx = (int)(range * ((float)rand() / (float)RAND_MAX));
    }

    // Cycle notes
    if (m_arpCycleModel.value() && dir != ArpDirRandom) {
      cur_arp_idx *= m_arpCycleModel.value() + 1;
      cur_arp_idx %= range;
    }

    // cst: getting the processed semitone
    InstrumentFunctionNoteStacking::ChordSemiTone cst =
        chord_table[selected_arp].at(cur_arp_idx % cur_chord_size);

    if (cst.active) { // The note is active, process all

      // now calculate final key for our arp-note
      const int sub_note_key = base_note_key +
                               (cur_arp_idx / cur_chord_size) * KeysPerOctave +
                               cst.key;
      // if the note is bare we don't intervene into panning, volume etc...
      volume_t sub_note_vol;
      panning_t sub_note_pan;
      if (cst.bare) {
        sub_note_vol = base_note_vol;
        sub_note_pan = base_note_pan;
      } else {
        // The note is silenced: playing it with no volume
        if (cst.silenced) {
          sub_note_vol = 0;
          sub_note_pan = 0;
        } else { // all modifications active, add interval to sub-note-key,
                 // volume, panning
          sub_note_vol = base_note_vol * ((float)cst.vol / (float)100);
          sub_note_pan = cst.pan;
        }
      }

      // range-checking
      if (sub_note_key >= NumKeys || sub_note_key < 0 ||
          Engine::mixer()->criticalXRuns()) {
        continue;
      }

      float vol_level = 1.0f;
      if (_n->isReleased()) {
        vol_level = _n->volumeLevel(cur_frame + gated_frames);
      }

      // create new arp-note

      // create sub-note-play-handle, only ptr to note is different
      // and is_arp_note=true
      Engine::mixer()->addPlayHandle(NotePlayHandleManager::acquire(
          _n->instrumentTrack(), frames_processed, gated_frames,
          Note(MidiTime(0), MidiTime(0), sub_note_key,
               (volume_t)qRound(sub_note_vol * vol_level), sub_note_pan,
               _n->detuning()),
          _n, -1, NotePlayHandle::OriginArpeggio));

      // update counters
      frames_processed += arp_frames;
      cur_frame += arp_frames;

    } // end cst.active block
  }
}

void InstrumentFunctionArpeggio::saveSettings(QDomDocument &_doc,
                                              QDomElement &_this) {
  m_arpEnabledModel.saveSettings(_doc, _this, "arp-enabled");
  m_arpModel.saveSettings(_doc, _this, "arp");
  m_arpRangeModel.saveSettings(_doc, _this, "arprange");
  m_arpCycleModel.saveSettings(_doc, _this, "arpcycle");
  m_arpSkipModel.saveSettings(_doc, _this, "arpskip");
  m_arpMissModel.saveSettings(_doc, _this, "arpmiss");
  m_arpTimeModel.saveSettings(_doc, _this, "arptime");
  m_arpGateModel.saveSettings(_doc, _this, "arpgate");
  m_arpDirectionModel.saveSettings(_doc, _this, "arpdir");

  m_arpModeModel.saveSettings(_doc, _this, "arpmode");
}

void InstrumentFunctionArpeggio::loadSettings(const QDomElement &_this) {
  m_arpEnabledModel.loadSettings(_this, "arp-enabled");
  m_arpModel.loadSettings(_this, "arp");
  m_arpRangeModel.loadSettings(_this, "arprange");
  m_arpCycleModel.loadSettings(_this, "arpcycle");
  m_arpSkipModel.loadSettings(_this, "arpskip");
  m_arpMissModel.loadSettings(_this, "arpmiss");
  m_arpTimeModel.loadSettings(_this, "arptime");
  m_arpGateModel.loadSettings(_this, "arpgate");
  m_arpDirectionModel.loadSettings(_this, "arpdir");
  /*
     // Keep compatibility with version 0.2.1 file format
     if( _this.hasAttribute( "arpsyncmode" ) )
     {
      m_arpTimeKnob->setSyncMode(
      ( tempoSyncKnob::tempoSyncMode ) _this.attribute(
                       "arpsyncmode" ).toInt() );
     }*/

  m_arpModeModel.loadSettings(_this, "arpmode");
}
