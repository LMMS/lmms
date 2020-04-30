/*
 * MidiSetupWidget - class for configuring midi sources in the settings window
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "MidiSetupWidget.h"

#include <QLineEdit>

#include "ConfigManager.h"
#include "gui_templates.h"

MidiSetupWidget::MidiSetupWidget(const QString & caption, const QString & configSection,
	const QString & devName, QWidget * parent) :
	TabWidget(TabWidget::tr("Settings for %1").arg(tr(caption.toLatin1())), parent),
	m_configSection(configSection),
	m_device(nullptr)
{
	// supply devName=QString() (distinct from QString(""))
	// to indicate that there is no editable device field
	if (!devName.isNull())
	{
		m_device = new QLineEdit(devName, this);
		m_device->setGeometry(10, 20, 160, 20);

		QLabel * dev_lbl = new QLabel(tr("Device"), this);
		dev_lbl->setFont(pointSize<7>(dev_lbl->font()));
		dev_lbl->setGeometry(10, 40, 160, 10);
	}
}

void MidiSetupWidget::saveSettings()
{
	if (!m_configSection.isEmpty() && m_device)
	{
		ConfigManager::inst()->setValue(m_configSection, "device",
				m_device->text());
	}
}

void MidiSetupWidget::show()
{
	// the setup widget should only be visible if the device has some configurable attributes
	bool visible = !m_configSection.isEmpty();
	parentWidget()->setVisible(visible);
	QWidget::setVisible(visible);
}
