/*
 * MidiPortMenu.h - a menu for subscribing a MidiPort to several external
 *                  MIDI ports
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef _MIDI_PORT_MENU_H
#define _MIDI_PORT_MENU_H

#include <QtGui/QMenu>

#include "ModelView.h"
#include "MidiPort.h"

class QAction;


class MidiPortMenu : public QMenu, public ModelView
{
	Q_OBJECT
public:
	MidiPortMenu( MidiPort::Modes _mode );
	virtual ~MidiPortMenu();


public slots:
	void updateMenu();


protected slots:
	void activatedPort( QAction * _item );


private:
	virtual void modelChanged();

	MidiPort::Modes m_mode;

} ;


#endif
