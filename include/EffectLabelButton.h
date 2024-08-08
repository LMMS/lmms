/*
 * EffectLabelButton.h - class trackLabelButton
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_EFFECT_LABEL_BUTTON_H
#define LMMS_GUI_EFFECT_LABEL_BUTTON_H

#include <QPushButton>

namespace lmms::gui
{

class EffectView;

class EffectLabelButton : public QPushButton
{
	Q_OBJECT
public:
	EffectLabelButton(EffectView* _tv, QWidget* _parent);
	~EffectLabelButton() override = default;

protected:
	void paintEvent(QPaintEvent* pe) override;

private:
	EffectView* m_effectView;
	QString m_iconName;
	QRect m_buttonRect;
	QString elideName(const QString &name);
};


} // namespace lmms::gui

#endif // LMMS_GUI_EFFECT_LABEL_BUTTON_H