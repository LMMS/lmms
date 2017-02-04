/*
 * AutomatableButton.cpp - implementation of class automatableButton and
 *                          automatableButtonGroup
 *
 * Copyright (c) 2006-2011 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AutomatableControlButton.h"

#include <QCursor>
#include <QMouseEvent>
#include <QPushButton>

#include "CaptionMenu.h"
#include "Engine.h"
#include "embed.h"
#include "MainWindow.h"
#include "StringPairDrag.h"


AutomatableControlButton::AutomatableControlButton( QWidget * _parent, const QString & _name ) :
	QPushButton( _parent ),
	FloatModelView( new FloatModel( 0.0, 0.0, 127.0, 1.0 ), this )
{
	setWindowTitle( _name );
	doConnections();
	setFocusPolicy( Qt::NoFocus );
}


AutomatableControlButton::~AutomatableControlButton()
{

}


void AutomatableControlButton::modelChanged()
{

}


void AutomatableControlButton::update()
{

}


void AutomatableControlButton::contextMenuEvent( QContextMenuEvent * _me )
{
	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent( NULL );

	CaptionMenu contextMenu( model()->displayName() );
	addDefaultActions( &contextMenu );
	contextMenu.exec( QCursor::pos() );

}


void AutomatableControlButton::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
			! ( _me->modifiers() & Qt::ControlModifier ) )
	{
        // User simply clicked
		_me->accept();
	}
	else
	{
		// Otherwise, drag the standalone button
		AutomatableModelView::mousePressEvent( _me );
		QPushButton::mousePressEvent( _me );
	}
}

void AutomatableControlButton::mouseReleaseEvent( QMouseEvent * _me )
{
	if( _me && _me->button() == Qt::LeftButton )
	{
		emit clicked();
	}
}



