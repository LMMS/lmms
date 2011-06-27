/*
 * AutomatableModelView.cpp - implementation of AutomatableModelView
 *
 * Copyright (c) 2011 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtGui/QMouseEvent>

#include "AutomatableModelView.h"
#include "AutomationPattern.h"
#include "ControllerConnectionDialog.h"
#include "ControllerConnection.h"
#include "embed.h"
#include "MainWindow.h"
#include "string_pair_drag.h"



AutomatableModelView::AutomatableModelView( ::Model * _model,
							QWidget * _this ) :
	ModelView( _model, _this ),
	m_description( QString::null ),
	m_unit( QString::null )
{
	widget()->setAcceptDrops( true );
	widget()->setCursor( QCursor( embed::getIconPixmap( "hand" ), 0, 0 ) );
}




AutomatableModelView::~AutomatableModelView()
{
}




void AutomatableModelView::addDefaultActions( QMenu * _menu )
{
	AutomatableModel * _model = modelUntyped();

	AutomatableModelViewSlots * amvSlots = 
		new AutomatableModelViewSlots( this, _menu );
	
	_menu->addAction( embed::getIconPixmap( "reload" ),
			AutomatableModel::tr( "&Reset (%1%2)" ).
				arg( _model->displayValue(
						_model->initValue<float>() ) ).
				arg( m_unit ),
					_model, SLOT( reset() ) );
	_menu->addSeparator();
	_menu->addAction( embed::getIconPixmap( "edit_copy" ),
			AutomatableModel::tr( "&Copy value (%1%2)" ).
				arg( _model->displayValue(
						_model->value<float>() ) ).
					arg( m_unit ),
						_model, SLOT( copyValue() ) );
	_menu->addAction( embed::getIconPixmap( "edit_paste" ),
			AutomatableModel::tr( "&Paste value (%1%2)").
				arg( _model->displayValue(
					AutomatableModel::copiedValue() ) ).
				arg( m_unit ),
						_model, SLOT( pasteValue() ) );

	_menu->addSeparator();

	_menu->addAction( embed::getIconPixmap( "automation" ),
			AutomatableModel::tr( "Edit song-global automation" ),
					amvSlots,
					SLOT( editSongGlobalAutomation() ) );
	_menu->addSeparator();

	QString controllerTxt;
	if( _model->getControllerConnection() )
	{
		Controller * cont = _model->getControllerConnection()->
								getController();
		if( cont )
		{
			controllerTxt =
				AutomatableModel::tr( "Connected to %1" ).
							arg( cont->name() );
		}
		else
		{
			controllerTxt = AutomatableModel::tr(
						"Connected to controller" );
		}

		QMenu * contMenu = _menu->addMenu(
					embed::getIconPixmap( "controller" ),
								controllerTxt );

		contMenu->addAction( embed::getIconPixmap( "controller" ),
				AutomatableModel::tr("Edit connection..."),
				amvSlots,
				SLOT( execConnectionDialog() ) );
		contMenu->addAction( embed::getIconPixmap( "cancel" ),
				AutomatableModel::tr("Remove connection"),
				amvSlots,
				SLOT( removeConnection() ) );
	}
	else
	{
		_menu->addAction( embed::getIconPixmap( "controller" ),
			AutomatableModel::tr("Connect to controller..."),
				amvSlots,
				SLOT( execConnectionDialog() ) );
	}
}




void AutomatableModelView::setModel( Model * _model, bool _old_model_valid )
{
	ModelView::setModel( _model, _old_model_valid );
}




void AutomatableModelView::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
			_me->modifiers() & Qt::ControlModifier )
	{
		new stringPairDrag( "automatable_model",
					QString::number( modelUntyped()->id() ),
							QPixmap(), widget() );
		_me->accept();
	}
	else if( _me->button() == Qt::MidButton )
	{
		modelUntyped()->reset();
	}
}






AutomatableModelViewSlots::AutomatableModelViewSlots( 
						AutomatableModelView * _amv,
							QObject * _parent ) :
	QObject(),
	amv( _amv )
{
	QObject::connect( _parent, SIGNAL( destroyed() ),
			this, SLOT( deleteLater() ),
			Qt::QueuedConnection );
}




void AutomatableModelViewSlots::execConnectionDialog()
{
	// TODO[pg]: Display a dialog with list of controllers currently in the song
	// in addition to any system MIDI controllers
	AutomatableModel * m = amv->modelUntyped();	

	m->displayName();
	ControllerConnectionDialog * d = new ControllerConnectionDialog( 
			(QWidget*)engine::mainWindow(), m );

	if( d->exec() == 1) 
	{
		// Actually chose something
		if (d->chosenController() != NULL )
		{
			// Update
			if( m->getControllerConnection() )
			{
				m->getControllerConnection()->
					setController( d->chosenController() );
			}
			// New
			else
			{
				ControllerConnection * cc =
						new ControllerConnection( d->chosenController() );
				m->setControllerConnection( cc );
				//cc->setTargetName( m->displayName() );
				
			}
		}
		// no controller, so delete existing connection
		else
		{
			removeConnection();
		}
	}

	delete d;
}




void AutomatableModelViewSlots::removeConnection()
{
	AutomatableModel * m = amv->modelUntyped();

	if( m->getControllerConnection() )
	{
		delete m->getControllerConnection();
		m->setControllerConnection( NULL );
	}
}




void AutomatableModelViewSlots::editSongGlobalAutomation()
{
	AutomationPattern::globalAutomationPattern( amv->modelUntyped() )->
						openInAutomationEditor();
}



#include "moc_AutomatableModelView.cxx"
