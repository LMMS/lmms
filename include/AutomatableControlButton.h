/*
 * AutomatableButton.h - class automatableControlButton,
 *  A button with a model that accepts values from 0 - 127 for midi
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef AUTOMATABLE_CONTROL_BUTTON_H
#define AUTOMATABLE_CONTROL_BUTTON_H

#include <QPushButton>

#include "AutomatableModelView.h"


class automatableButtonGroup;


class EXPORT AutomatableControlButton : public QPushButton, public FloatModelView
{
	Q_OBJECT
public:
	AutomatableControlButton( QWidget * _parent, const QString & _name = QString::null );
	virtual ~AutomatableControlButton();

	virtual void modelChanged();


public slots:
	virtual void update();

protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );


private:

} ;


#endif
