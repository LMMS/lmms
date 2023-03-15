/*
 * NineButtonSelector.cpp
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


#include "NineButtonSelector.h"

#include "CaptionMenu.h"

namespace lmms::gui
{


NineButtonSelector::NineButtonSelector(
		QPixmap button0On, QPixmap button0Off,
		QPixmap button1On, QPixmap button1Off,
		QPixmap button2On, QPixmap button2Off,
		QPixmap button3On, QPixmap button3Off,
		QPixmap button4On, QPixmap button4Off,
		QPixmap button5On, QPixmap button5Off,
		QPixmap button6On, QPixmap button6Off,
		QPixmap button7On, QPixmap button7Off,
		QPixmap button8On, QPixmap button8Off,
		int defaultButton, int x, int y, QWidget* parent) :
	QWidget{parent},
	IntModelView{new NineButtonSelectorModel{defaultButton, 0, 8, nullptr, QString{}, true}, this}
{
	setFixedSize(50, 50);
	move(x, y);

	m_buttons[0] = std::make_unique<PixmapButton>(this, nullptr);
	m_buttons[0]->move(1, 1);
	m_buttons[0]->setActiveGraphic(button0On);
	m_buttons[0]->setInactiveGraphic(button0Off);
	m_buttons[0]->setChecked(false);
	connect(m_buttons[0].get(), SIGNAL(clicked()), this, SLOT(button0Clicked()));

	m_buttons[1] = std::make_unique<PixmapButton>(this, nullptr);
	m_buttons[1]->move(18, 1);
	m_buttons[1]->setActiveGraphic(button1On);
	m_buttons[1]->setInactiveGraphic(button1Off);
	m_buttons[1]->setChecked(false);
	connect(m_buttons[1].get(), SIGNAL(clicked()), this, SLOT(button1Clicked()));

	m_buttons[2] = std::make_unique<PixmapButton>(this, nullptr);
	m_buttons[2]->move(35, 1);
	m_buttons[2]->setActiveGraphic(button2On);
	m_buttons[2]->setInactiveGraphic(button2Off);
	m_buttons[2]->setChecked(false);
	connect(m_buttons[2].get(), SIGNAL(clicked()), this, SLOT(button2Clicked()));

	m_buttons[3] = std::make_unique<PixmapButton>(this, nullptr);
	m_buttons[3]->move(1, 18);
	m_buttons[3]->setActiveGraphic(button3On);
	m_buttons[3]->setInactiveGraphic(button3Off);
	m_buttons[3]->setChecked(false);
	connect(m_buttons[3].get(), SIGNAL(clicked()), this, SLOT(button3Clicked()));

	m_buttons[4] = std::make_unique<PixmapButton>(this, nullptr);
	m_buttons[4]->move(18, 18);
	m_buttons[4]->setActiveGraphic(button4On);
	m_buttons[4]->setInactiveGraphic(button4Off);
	m_buttons[4]->setChecked(false);
	connect(m_buttons[4].get(), SIGNAL(clicked()), this, SLOT(button4Clicked()));

	m_buttons[5] = std::make_unique<PixmapButton>(this, nullptr);
	m_buttons[5]->move(35, 18);
	m_buttons[5]->setActiveGraphic(button5On);
	m_buttons[5]->setInactiveGraphic(button5Off);
	m_buttons[5]->setChecked(false);
	connect(m_buttons[5].get(), SIGNAL(clicked()), this, SLOT(button5Clicked()));

	m_buttons[6] = std::make_unique<PixmapButton>(this, nullptr);
	m_buttons[6]->move(1, 35);
	m_buttons[6]->setActiveGraphic(button6On);
	m_buttons[6]->setInactiveGraphic(button6Off);
	m_buttons[6]->setChecked(false);
	connect(m_buttons[6].get(), SIGNAL(clicked()), this, SLOT(button6Clicked()));

	m_buttons[7] = std::make_unique<PixmapButton>(this, nullptr);
	m_buttons[7]->move(18, 35);
	m_buttons[7]->setActiveGraphic(button7On);
	m_buttons[7]->setInactiveGraphic(button7Off);
	m_buttons[7]->setChecked(false);
	connect(m_buttons[7].get(), SIGNAL(clicked()), this, SLOT(button7Clicked()));

	m_buttons[8] = std::make_unique<PixmapButton>(this, nullptr);
	m_buttons[8]->move(35, 35);
	m_buttons[8]->setActiveGraphic(button8On);
	m_buttons[8]->setInactiveGraphic(button8Off);
	m_buttons[8]->setChecked(false);
	connect(m_buttons[8].get(), SIGNAL(clicked()), this, SLOT(button8Clicked()));

	m_lastBtn = m_buttons[defaultButton].get();
	m_lastBtn->setChecked(true);
}

void NineButtonSelector::button0Clicked()
{
	setSelected(0);
}

void NineButtonSelector::button1Clicked()
{
	setSelected(1);
}

void NineButtonSelector::button2Clicked()
{
	setSelected(2);
}

void NineButtonSelector::button3Clicked()
{
	setSelected(3);
}

void NineButtonSelector::button4Clicked()
{
	setSelected(4);
}

void NineButtonSelector::button5Clicked()
{
	setSelected(5);
}

void NineButtonSelector::button6Clicked()
{
	setSelected(6);
}

void NineButtonSelector::button7Clicked()
{
	setSelected(7);
}

void NineButtonSelector::button8Clicked()
{
	setSelected(8);
}

void NineButtonSelector::modelChanged()
{
	updateButton(model()->value());
}

void NineButtonSelector::setSelected(int newButton)
{
	model()->setValue(newButton);
	updateButton(newButton);
}

void NineButtonSelector::updateButton(int newButton)
{
	m_lastBtn->setChecked(false);
	m_lastBtn->update();

	m_lastBtn = m_buttons[newButton].get();
	m_lastBtn->setChecked(true);
	m_lastBtn->update();

	emit NineButtonSelection(newButton);
}

void NineButtonSelector::contextMenuEvent(QContextMenuEvent*)
{
	CaptionMenu contextMenu{windowTitle(), this};
	contextMenu.exec(QCursor::pos());
}


} // namespace lmms::gui
