#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect_control_dialog.cpp - base-class for effect-dialogs for displaying
 *                            and editing control port values
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QMessageBox>
#include <QtGui/QCloseEvent>

#include "effect_control_dialog.h"
#include "effect_controls.h"
#include "effect.h"


effectControlDialog::effectControlDialog( effectControls * _controls ) :
	QWidget( NULL ),
	modelView( _controls, this ),
	m_effectControls( _controls )
{
	setWindowTitle( m_effectControls->getEffect()->publicName() );

}




effectControlDialog::~effectControlDialog()
{
}




void effectControlDialog::closeEvent( QCloseEvent * _ce )
{
	_ce->ignore();
	emit( closed() );
}




#include "effect_control_dialog.moc"

#endif

