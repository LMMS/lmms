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

#include <functional>

namespace lmms::gui
{


class LMMS_EXPORT CustomTextKnob : public Knob
{
protected:
	inline void setHintText(const QString& txtBefore, const QString& txtAfter) {} // inaccessible
public:
	CustomTextKnob(
		KnobType knobNum,
		QWidget* parent = nullptr,
		const QString& name = QString{},
		const QString& valueText = QString{});

	//! default ctor
	CustomTextKnob(
		QWidget* parent = nullptr,
		const QString& name = QString{});

	CustomTextKnob(const Knob& other) = delete;

	inline void setValueText(const QString& valueText)
	{
		m_valueText = valueText;
		m_valueTextType = ValueTextType::Static;
	}

	inline void setValueText(const std::function<QString()>& valueTextFunc)
	{
		m_valueTextFunc = valueTextFunc;
		m_valueTextType = ValueTextType::Dynamic;
	}

private:
	QString displayValue() const override;

protected:

	enum class ValueTextType
	{
		Static,
		Dynamic
	} m_valueTextType = ValueTextType::Static;

	QString m_valueText;
	std::function<QString()> m_valueTextFunc;
};


} // namespace lmms::gui

#endif // LMMS_GUI_CUSTOM_TEXT_KNOB_H
