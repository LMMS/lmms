/*
 * PianoRollPainter.h - rendering helper for PianoRoll
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 */

#ifndef LMMS_GUI_PIANO_ROLL_PAINTER_H
#define LMMS_GUI_PIANO_ROLL_PAINTER_H

class QPaintEvent;

namespace lmms::gui
{

class PianoRoll;

class PianoRollPainter
{
public:
	explicit PianoRollPainter(PianoRoll& pianoRoll);

	void paint(QPaintEvent* event);

private:
	PianoRoll& m_pianoRoll;
};

} // namespace lmms::gui

#endif // LMMS_GUI_PIANO_ROLL_PAINTER_H
