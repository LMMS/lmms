/*
 * InstrumentMiscView.h - widget in instrument-track-window for setting up
 *                        miscellaneous options not covered by other tabs
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2020 Martin Pavelek <he29.HS/at/gmail.com>
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

#ifndef INSTRUMENT_MISC_VIEW_H
#define INSTRUMENT_MISC_VIEW_H

#include <QWidget>

namespace lmms
{

class InstrumentTrack;

namespace gui
{

class ComboBox;
class GroupBox;
class LedCheckBox;


class InstrumentMiscView : public QWidget
{
	Q_OBJECT
public:
	InstrumentMiscView(InstrumentTrack *it, QWidget *parent);

	GroupBox *pitchGroupBox() {return m_pitchGroupBox;}
	GroupBox *microtunerGroupBox() {return m_microtunerGroupBox;}

	ComboBox *scaleCombo() {return m_scaleCombo;}
	ComboBox *keymapCombo() {return m_keymapCombo;}

	LedCheckBox *rangeImportCheckbox() {return m_rangeImportCheckbox;}

private:
	GroupBox *m_pitchGroupBox;
	GroupBox *m_microtunerGroupBox;

	ComboBox *m_scaleCombo;
	ComboBox *m_keymapCombo;

	LedCheckBox *m_rangeImportCheckbox;
};


} // namespace gui

} // namespace lmms

#endif
