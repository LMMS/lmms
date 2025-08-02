/*
 * CustomTextKnob.h
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

#ifndef LMMS_GUI_CUSTOM_TEXT_KNOB_H
#define LMMS_GUI_CUSTOM_TEXT_KNOB_H

#include "Knob.h"

namespace lmms::gui
{


class LMMS_EXPORT CustomTextKnob : public Knob
{
protected:
	inline void setHintText( const QString & _txt_before, const QString & _txt_after ) {} // inaccessible
public:
	CustomTextKnob( KnobType _knob_num, const QString& label, QWidget * _parent = nullptr, const QString & _name = QString(), const QString & _value_text = QString() );

	CustomTextKnob( const Knob& other ) = delete;

	inline void setValueText(const QString & _value_text)
	{
		m_value_text = _value_text;
	}

private:
	QString displayValue() const override;

protected:
	QString m_value_text;
} ;


} // namespace lmms::gui

#endif // LMMS_GUI_CUSTOM_TEXT_KNOB_H
