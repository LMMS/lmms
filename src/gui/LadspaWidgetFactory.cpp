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
#include "TempoSyncKnob.h"

#include <QLabel>


namespace lmms::gui
{

QWidget * LadspaWidgetFactory::createWidget(LadspaControl * ladspaControl, QWidget * parent)
{
	Knob * knob = nullptr;

	auto const * port = ladspaControl->port();

	QString const name = port->name;

	switch (port->data_type)
	{
	case TOGGLED:
	{
		LedCheckBox * toggle = new LedCheckBox(
			name, parent, QString(), LedCheckBox::Green);
		toggle->setModel(ladspaControl->toggledModel());
		return toggle;
	}

	case INTEGER:
	case FLOATING:
		// TODO Remove when not needed anymore
		/*knob = new Knob(knobBright_26, parent, name);
		knob->setModel(ladspaControl->knobModel());
		knob->setLabel(name);
		break;*/
		return new BarModelEditor(name, ladspaControl->knobModel(), parent);

	case TIME:
		knob = new TempoSyncKnob(knobBright_26, parent, name);
		knob->setModel(ladspaControl->tempoSyncKnobModel());
		knob->setLabel(name);
		break;

	default:
		return new QLabel(QObject::tr("%1 (unsupported)").arg(name), parent);
	}

	if (knob != nullptr)
	{
		knob->setHintText(QObject::tr("Value:"), "");
		return knob;
	}

	return nullptr;
}

} // namespace lmms::gui
