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
#include <QtGui/QMouseEvent>

#include "automatable_model_view.h"
#include "automation_pattern.h"
#include "ControllerConnectionDialog.h"
#include "ControllerConnection.h"
#include "embed.h"
#include "MainWindow.h"
#include "string_pair_drag.h"



automatableModelView::automatableModelView( ::model * _model,
							QWidget * _this ) :
	modelView( _model, _this ),
	m_description( QString::null ),
	m_unit( QString::null )
{
	widget()->setAcceptDrops( TRUE );
	widget()->setCursor( QCursor( embed::getIconPixmap( "hand" ), 0, 0 ) );
}




automatableModelView::~automatableModelView()
{
}




void automatableModelView::addDefaultActions( QMenu * _menu )
{
	automatableModel * _model = modelUntyped();

	automatableModelViewSlots * amvSlots = 
		new automatableModelViewSlots( this, _menu );
	
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

	_menu->addAction( embed::getIconPixmap( "automation" ),
			automatableModel::tr( "Edit song-global automation" ),
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
				automatableModel::tr( "Connected to %1" ).
							arg( cont->name() );
		}
		else
		{
			controllerTxt = automatableModel::tr(
						"Connected to controller" );
		}

		QMenu * contMenu = _menu->addMenu(
					embed::getIconPixmap( "controller" ),
								controllerTxt );

		contMenu->addAction( embed::getIconPixmap( "controller" ),
				automatableModel::tr("Edit connection..."),
				amvSlots,
				SLOT( execConnectionDialog() ) );
		contMenu->addAction( embed::getIconPixmap( "cancel" ),
				automatableModel::tr("Remove connection"),
				amvSlots,
				SLOT( removeConnection() ) );
	}
	else
	{
		_menu->addAction( embed::getIconPixmap( "controller" ),
			automatableModel::tr("Connect to controller..."),
				amvSlots,
				SLOT( execConnectionDialog() ) );
	}

	if( _model->armed() )
	{
		_menu->addAction( //embed::getIconPixmap( "controller" ), 
			automatableModel::tr( "de-arm" ),
			amvSlots,
			SLOT( deArm() ) );
	}
	else
	{
		_menu->addAction( //embed::getIconPixmap( "controller" ), 
			automatableModel::tr( "Arm for recording" ),
			amvSlots,
			SLOT( arm() ) );
	}
}




void automatableModelView::setModel( model * _model, bool _old_model_valid )
{
	modelView::setModel( _model, _old_model_valid );
	//setAccessibleName( _model->displayName();
}




void automatableModelView::mousePressEvent( QMouseEvent * _me )
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






automatableModelViewSlots::automatableModelViewSlots( 
						automatableModelView * _amv,
							QObject * _parent ) :
	QObject(),
	amv( _amv )
{
	QObject::connect( _parent, SIGNAL( destroyed() ),
			this, SLOT( deleteLater() ),
			Qt::QueuedConnection );
}




void automatableModelViewSlots::execConnectionDialog( void )
{
	// TODO[pg]: Display a dialog with list of controllers currently in the song
	// in addition to any system MIDI controllers
	automatableModel * m = amv->modelUntyped();	

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




void automatableModelViewSlots::removeConnection( void )
{
	automatableModel * m = amv->modelUntyped();

	if( m->getControllerConnection() )
	{
		delete m->getControllerConnection();
		m->setControllerConnection( NULL );
	}
}




void automatableModelViewSlots::editSongGlobalAutomation( void )
{
	automationPattern::globalAutomationPattern( amv->modelUntyped() )->
						openInAutomationEditor();
}


void automatableModelViewSlots::arm( void )
{
	amv->modelUntyped()->setArmed( true );
}


void automatableModelViewSlots::deArm( void )
{
	amv->modelUntyped()->setArmed( false );
}


#include "moc_automatable_model_view.cxx"
