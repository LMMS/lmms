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
#include "FontHelper.h"

namespace lmms::gui
{


CustomTextKnob::CustomTextKnob( KnobType _knob_num, const QString& label, QWidget * _parent, const QString & _name, const QString & _value_text ) :
	Knob( _knob_num, _parent, _name ),
	m_value_text( _value_text )
{
	setFont(adjustedToPixelSize(font(), SMALL_FONT_SIZE));
	setLabel(label);
}

QString CustomTextKnob::displayValue() const
{
	return m_description.trimmed() + m_value_text;
}


} // namespace lmms::gui
