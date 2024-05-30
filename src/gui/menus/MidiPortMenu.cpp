/*
 * MidiPortMenu.cpp - a menu for subscribing a MidiPort to several external
 *                      MIDI ports
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "MidiPortMenu.h"

namespace lmms::gui
{


MidiPortMenu::MidiPortMenu( MidiPort::Mode _mode ) :
	ModelView( nullptr, this ),
	m_mode( _mode )
{
	connect( this, SIGNAL(triggered(QAction*)),
			this, SLOT(activatedPort(QAction*)));
}





void MidiPortMenu::modelChanged()
{
	auto mp = castModel<MidiPort>();
	if( m_mode == MidiPort::Mode::Input )
	{
		connect( mp, SIGNAL(readablePortsChanged()),
				this, SLOT(updateMenu()));
	}
	else if( m_mode == MidiPort::Mode::Output )
	{
		connect( mp, SIGNAL(writablePortsChanged()),
				this, SLOT(updateMenu()));
	}
	updateMenu();
}




void MidiPortMenu::activatedPort( QAction * _item )
{
	if( m_mode == MidiPort::Mode::Input )
	{
		castModel<MidiPort>()->subscribeReadablePort( _item->text(),
							_item->isChecked() );
	}
	else if( m_mode == MidiPort::Mode::Output )
	{
		castModel<MidiPort>()->subscribeWritablePort( _item->text(),
							_item->isChecked() );
	}
}




void MidiPortMenu::updateMenu()
{
	auto mp = castModel<MidiPort>();
	const MidiPort::Map & map = ( m_mode == MidiPort::Mode::Input ) ?
				mp->readablePorts() : mp->writablePorts();
	clear();
	for( MidiPort::Map::ConstIterator it = map.begin();
							it != map.end(); ++it )
	{
		QAction * a = addAction( it.key() );
		a->setCheckable( true );
		a->setChecked( it.value() );
	}
}


} // namespace lmms::gui


