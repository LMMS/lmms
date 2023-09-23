/*
 * LadspaWidgetFactory.cpp - Factory that creates widgets for LADSPA ports
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2015-2023 Michael Gregorius
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


#include "LadspaWidgetFactory.h"

#include "LadspaControl.h"
#include "LadspaBase.h"

#include "BarModelEditor.h"
#include "LedCheckBox.h"
#include "TempoSyncBarModelEditor.h"

#include <QLabel>


namespace lmms::gui
{

QWidget * LadspaWidgetFactory::createWidget(LadspaControl * ladspaControl, QWidget * parent)
{
	auto const * port = ladspaControl->port();

	QString const name = port->name;

	switch (port->data_type)
	{
	case BufferDataType::Toggled:
	{
		LedCheckBox * toggle = new LedCheckBox(
			name, parent, QString(), LedCheckBox::LedColor::Green, false);
		toggle->setModel(ladspaControl->toggledModel());
		return toggle;
	}

	case BufferDataType::Integer:
	case BufferDataType::Enum:
	case BufferDataType::Floating:
		return new BarModelEditor(name, ladspaControl->knobModel(), parent);

	case BufferDataType::Time:
		return new TempoSyncBarModelEditor(name, ladspaControl->tempoSyncKnobModel(), parent);

	default:
		return new QLabel(QObject::tr("%1 (unsupported)").arg(name), parent);
	}

	return nullptr;
}

} // namespace lmms::gui
