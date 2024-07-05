/*
 * InstrumentFunctionViews.h - views for instrument-functions-tab
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

#ifndef LMMS_INSTRUMENT_FUNCTION_VIEWS_H
#define LMMS_INSTRUMENT_FUNCTION_VIEWS_H

#include "ModelView.h"

#include <QWidget>

class QLabel;

namespace lmms
{

class InstrumentFunctionArpeggio;
class InstrumentFunctionNoteStacking;

namespace gui
{

class ComboBox;
class GroupBox;
class Knob;
class TempoSyncKnob;

class InstrumentFunctionNoteStackingView : public QWidget, public ModelView
{
	Q_OBJECT
public:
	InstrumentFunctionNoteStackingView( InstrumentFunctionNoteStacking* cc, QWidget* parent = nullptr );
	~InstrumentFunctionNoteStackingView() override;


private:
	void modelChanged() override;

	InstrumentFunctionNoteStacking * m_cc;

	GroupBox * m_chordsGroupBox;
	ComboBox * m_chordsComboBox;
	Knob * m_chordRangeKnob;

} ;





class InstrumentFunctionArpeggioView : public QWidget, public ModelView
{
	Q_OBJECT
public:
	InstrumentFunctionArpeggioView( InstrumentFunctionArpeggio* arp, QWidget* parent = nullptr );
	~InstrumentFunctionArpeggioView() override;


private:
	void modelChanged() override;

	InstrumentFunctionArpeggio * m_a;
	GroupBox * m_arpGroupBox;
	ComboBox * m_arpComboBox;
	Knob * m_arpRangeKnob;
	Knob * m_arpRepeatsKnob;
	Knob * m_arpCycleKnob;
	Knob * m_arpSkipKnob;
	Knob * m_arpMissKnob;
	TempoSyncKnob * m_arpTimeKnob;
	Knob * m_arpGateKnob;

	ComboBox * m_arpDirectionComboBox;
	ComboBox * m_arpModeComboBox;

} ;

} // namespace gui

} // namespace lmms

#endif // LMMS_INSTRUMENT_FUNCTION_VIEWS_H
