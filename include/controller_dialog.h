/*
 * controller_dialog.h - per-controller-specific view for changing a
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

#ifndef _CONTROLLER_DIALOG_H
#define _CONTROLLER_DIALOG_H

#include <QtGui/QWidget>

#include "mv_base.h"

class controller;


class controllerDialog : public QWidget, public modelView
{
    Q_OBJECT
public:
	controllerDialog( controller * _controller, QWidget * _parent );

	virtual ~controllerDialog();

signals:
	void closed();


protected:
/*	virtual void contextMenuEvent( QContextMenuEvent * _me ) {};
	virtual void paintEvent( QPaintEvent * _pe ) {};
	virtual void modelChanged( void ) {};*/
	virtual void closeEvent( QCloseEvent * _ce );

} ;

#endif
