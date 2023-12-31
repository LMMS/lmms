/*
 * CustomTextKnob.cpp
 *
 * Copyright (c) 2020 Ibuki Sugiyama <main/at/fuwa.dev>
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

#include "CustomTextKnob.h"

namespace lmms::gui
{


CustomTextKnob::CustomTextKnob(KnobType knobNum, QWidget* parent, const QString& name)
	: Knob(knobNum, parent, name) {}

CustomTextKnob::CustomTextKnob(QWidget* parent, const QString& name)
	: Knob(parent, name) {}

QString CustomTextKnob::displayValue() const
{
	if (m_valueTextType == ValueTextType::Static)
	{
		return m_description.trimmed() + m_valueText;
	}
	else
	{
		return m_valueTextFunc();
	}
}


} // namespace lmms::gui
