/*
 * automatable_model_view.cpp - implementation of automatableModelView
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


#include <QtGui/QMenu>


#include "automatable_model_view.h"
#include "automation_pattern.h"
#include "embed.h"


void automatableModelView::addDefaultActions( QMenu * _menu )
{
	automatableModel * _model = modelUntyped();

	_menu->addAction( embed::getIconPixmap( "reload" ),
			automatableModel::tr( "&Reset (%1%2)" ).
				arg( _model->displayValue(
						_model->initValue<float>() ) ).
				arg( m_unit ),
					_model, SLOT( reset() ) );
	_menu->addSeparator();
	_menu->addAction( embed::getIconPixmap( "edit_copy" ),
			automatableModel::tr( "&Copy value (%1%2)" ).
				arg( _model->displayValue(
						_model->value<float>() ) ).
					arg( m_unit ),
						_model, SLOT( copyValue() ) );
	_menu->addAction( embed::getIconPixmap( "edit_paste" ),
			automatableModel::tr( "&Paste value (%1%2)").
				arg( _model->displayValue(
					automatableModel::copiedValue() ) ).
				arg( m_unit ),
						_model, SLOT( pasteValue() ) );

	_menu->addSeparator();

	if( !_model->nullTrack() )
	{
		_menu->addAction( embed::getIconPixmap( "automation" ),
			automatableModel::tr( "&Open in automation editor" ),
					_model->getAutomationPattern(),
					SLOT( openInAutomationEditor() ) );
		_menu->addSeparator();
	}

	_menu->addAction( embed::getIconPixmap( "controller" ),
			automatableModel::tr( "Connect to controller..." ),
				dynamic_cast<QObject *>( this ),
					SLOT( connectToController() ) );
}


