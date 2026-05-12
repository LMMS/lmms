/*
 * PianoRollConstants.h - shared constants for PianoRoll rendering and layout
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 */

#ifndef LMMS_GUI_PIANO_ROLL_CONSTANTS_H
#define LMMS_GUI_PIANO_ROLL_CONSTANTS_H

namespace lmms::gui
{

inline constexpr int INITIAL_PIANOROLL_WIDTH = 970;
inline constexpr int INITIAL_PIANOROLL_HEIGHT = 485;

inline constexpr int SCROLLBAR_SIZE = 12;
inline constexpr int PIANO_X = 0;

inline constexpr int WHITE_KEY_WIDTH = 64;
inline constexpr int BLACK_KEY_WIDTH = 41;

inline constexpr int DEFAULT_KEY_LINE_HEIGHT = 12;
inline constexpr int DEFAULT_CELL_WIDTH = 12;

inline constexpr int NOTE_EDIT_RESIZE_BAR = 6;
inline constexpr int NOTE_EDIT_MIN_HEIGHT = 50;
inline constexpr int KEY_AREA_MIN_HEIGHT = DEFAULT_KEY_LINE_HEIGHT * 10;
inline constexpr int PR_BOTTOM_MARGIN = SCROLLBAR_SIZE;
inline constexpr int PR_TOP_MARGIN = 18;
inline constexpr int PR_RIGHT_MARGIN = SCROLLBAR_SIZE;

inline constexpr int RESIZE_AREA_WIDTH = 9;
inline constexpr int NOTE_EDIT_LINE_WIDTH = 3;

inline constexpr int NUM_EVEN_LENGTHS = 6;
inline constexpr int NUM_TRIPLET_LENGTHS = 5;
inline constexpr int DETUNING_HANDLE_RADIUS = 3;

} // namespace lmms::gui

#endif // LMMS_GUI_PIANO_ROLL_CONSTANTS_H
