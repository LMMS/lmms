#ifndef SINGLE_SOURCE_COMPILE

/*
 * controller_dialog.cpp - per-controller-specific view for changing a
 * controller's settings
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#include <QtGui/QCloseEvent>

#include "controller_dialog.h"
#include "controller.h"


controllerDialog::controllerDialog( controller * _controller,
							QWidget * _parent ) :
	QWidget( _parent ),
	modelView( _controller, this )
{
}



controllerDialog::~controllerDialog()
{
}



void controllerDialog::closeEvent( QCloseEvent * _ce )
{
	_ce->ignore();
	emit closed();
}


#include "controller_dialog.moc"

#endif
