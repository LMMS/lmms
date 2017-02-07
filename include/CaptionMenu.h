/*
 * CaptionMenu.h - context menu with a caption
 *
 * Copyright (c) 2007-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#ifndef CAPTION_MENU_H
#define CAPTION_MENU_H

#include <QMenu>

#include "export.h"

///
/// \brief A context menu with a caption
///
class EXPORT CaptionMenu : public QMenu
{
	Q_OBJECT
public:
	CaptionMenu( const QString & _title, QWidget * _parent = 0 );
	virtual ~CaptionMenu();

	///
	/// \brief Adds a "Help" action displaying the Menu's parent's WhatsThis
	///		   text when selected.
	///
	void addHelpAction();

} ;




#endif
