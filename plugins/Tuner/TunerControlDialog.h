/*
 * TunerControlDialog.h
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
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

#ifndef TUNER_CONTROL_DIALOG_H
#define TUNER_CONTROL_DIALOG_H

#include <QLabel>

#include "EffectControlDialog.h"
#include "LcdWidget.h"

class TunerControls;
class TunerControlDialog : public EffectControlDialog
{
public:
	enum class NoteName 
	{
		A,
		ASharp,
		B,
		C,
		CSharp,
		D,
		DSharp,
		E,
		F,
		FSharp,
		G,
		GSharp
	};

	TunerControlDialog(TunerControls* controls);
	void frequencyCalculated(float frequency);
	std::string noteToString(NoteName name);
private:
	LcdWidget* m_centsWidget;
	LcdWidget* m_freqWidget;
	QLabel* m_noteLabel;
	QLabel* m_octaveLabel;
	QLabel* m_centsLabel;
	QLabel* m_frequencyLabel;
	friend class TunerControls;
};

#endif