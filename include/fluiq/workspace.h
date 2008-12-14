/*
 * workspace.h - header file for FLUIQ::Workspace
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#ifndef _FLUIQ_WORKSPACE_H
#define _FLUIQ_WORKSPACE_H

#include "fluiq/widget.h"
#include "fluiq/splitter.h"

class QVBoxLayout;


namespace FLUIQ
{


class Workspace : public Widget
{
	Q_OBJECT
public:
	Workspace( QWidget * _parent = NULL );
	virtual ~Workspace();

	void addWidget( QWidget * _w );


private:
	Splitter * m_currentSplitter;
	QVBoxLayout * m_masterLayout;

} ;


}

#endif
