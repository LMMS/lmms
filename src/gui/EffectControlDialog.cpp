/*
 * EffectControlDialog.cpp - base-class for effect-dialogs for displaying
 *                           and editing control port values
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QMessageBox>
#include <QtGui/QCloseEvent>

#include "EffectControlDialog.h"
#include "EffectControls.h"
#include "Effect.h"


EffectControlDialog::EffectControlDialog( EffectControls * _controls ) :
	QWidget( NULL ),
	ModelView( _controls, this ),
	m_effectControls( _controls )
{
	setWindowTitle( m_effectControls->effect()->displayName() );
}




EffectControlDialog::~EffectControlDialog()
{
}




void EffectControlDialog::closeEvent( QCloseEvent * _ce )
{
	_ce->ignore();
	emit closed();
}



#include "moc_EffectControlDialog.cxx"

