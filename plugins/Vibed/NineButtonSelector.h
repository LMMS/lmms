/*
 * NineButtonSelector.h
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/yahoo/com>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_NINE_BUTTON_SELECTOR_H
#define LMMS_GUI_NINE_BUTTON_SELECTOR_H

#include <array>
#include <QWidget>

#include "AutomatableModelView.h"

namespace lmms
{


namespace gui
{

class PixmapButton;


class NineButtonSelector : public QWidget, public IntModelView
{
	Q_OBJECT
public:
	NineButtonSelector(const std::array<QPixmap, 18>& onOffIcons, int defaultButton, int x, int y, QWidget* parent);
	~NineButtonSelector() override = default;

protected:
	void setSelected(int newButton);

public slots:
	void buttonClicked(int id);
	void contextMenuEvent(QContextMenuEvent*) override;

signals:
	void NineButtonSelection(int);

private:
	void modelChanged() override;
	void updateButton(int);

	std::array<PixmapButton*, 9> m_buttons;
	PixmapButton* m_lastBtn;
};


} // namespace gui

using NineButtonSelectorModel = IntModel;


} // namespace lmms

#endif // LMMS_GUI_NINE_BUTTON_SELECTOR_H
