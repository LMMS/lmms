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

#ifndef MIDISETUPWIDGET_H
#define MIDISETUPWIDGET_H

#include <QLabel>

#include "TabWidget.h"

class QLineEdit;

class MidiSetupWidget : public TabWidget
{
	Q_OBJECT
	MidiSetupWidget( const QString & caption, const QString & configSection,
		const QString & devName, QWidget * parent );
public:
	// create a widget with editors for all of @MidiClientType's fields
	template <typename MidiClientType> static MidiSetupWidget* create( QWidget * parent )
	{
		QString configSection = MidiClientType::configSection();
		QString dev = MidiClientType::probeDevice();
		return new MidiSetupWidget(MidiClientType::name(), configSection, dev, parent);
	}

	void saveSettings();

	void show();
private:
	QString m_configSection;
	QLineEdit *m_device;

};

#endif
