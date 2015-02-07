/*
 * AutomatableModelView.cpp - implementation of AutomatableModelView
 *
 * Copyright (c) 2011-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QMenu>
#include <QMouseEvent>

#include "AutomatableModelView.h"
#include "AutomationPattern.h"
#include "ControllerConnectionDialog.h"
#include "ControllerConnection.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "StringPairDrag.h"

#include "AutomationEditor.h"



AutomatableModelView::AutomatableModelView( ::Model* model, QWidget* _this ) :
	ModelView( model, _this ),
	m_description( QString::null ),
	m_unit( QString::null )
{
	widget()->setAcceptDrops( true );
	widget()->setCursor( QCursor( QPixmap( "icons:hand.png" ), 3, 3 ) );
}




AutomatableModelView::~AutomatableModelView()
{
}




void AutomatableModelView::addDefaultActions( QMenu* menu )
{
	AutomatableModel* model = modelUntyped();

	AutomatableModelViewSlots* amvSlots = new AutomatableModelViewSlots( this, menu );

	menu->addAction( QPixmap( "icons:reload.png" ),
						AutomatableModel::tr( "&Reset (%1%2)" ).
							arg( model->displayValue( model->initValue<float>() ) ).
							arg( m_unit ),
						model, SLOT( reset() ) );

	menu->addSeparator();
	menu->addAction( QPixmap( "icons:edit_copy.png" ),
						AutomatableModel::tr( "&Copy value (%1%2)" ).
							arg( model->displayValue( model->value<float>() ) ).
							arg( m_unit ),
						model, SLOT( copyValue() ) );

	menu->addAction( QPixmap( "icons:edit_paste.png" ),
						AutomatableModel::tr( "&Paste value (%1%2)").
							arg( model->displayValue( AutomatableModel::copiedValue() ) ).
							arg( m_unit ),
						model, SLOT( pasteValue() ) );

	menu->addSeparator();

	menu->addAction( QPixmap( "icons:automation.png" ),
						AutomatableModel::tr( "Edit song-global automation" ),
							amvSlots,
							SLOT( editSongGlobalAutomation() ) );

	menu->addAction( QPixmap(),
						AutomatableModel::tr( "Remove song-global automation" ),
						amvSlots,
						SLOT( removeSongGlobalAutomation() ) );

	menu->addSeparator();

	if( model->hasLinkedModels() )
	{
		menu->addAction( QPixmap( "icons:edit-delete.png" ),
							AutomatableModel::tr( "Remove all linked controls" ),
							amvSlots, SLOT( unlinkAllModels() ) );
		menu->addSeparator();
	}

	QString controllerTxt;
	if( model->controllerConnection() )
	{
		Controller* cont = model->controllerConnection()->getController();
		if( cont )
		{
			controllerTxt = AutomatableModel::tr( "Connected to %1" ).arg( cont->name() );
		}
		else
		{
			controllerTxt = AutomatableModel::tr( "Connected to controller" );
		}

		QMenu* contMenu = menu->addMenu( QPixmap( "icons:controller.png" ), controllerTxt );

		contMenu->addAction( QPixmap( "icons:controller.png" ),
								AutomatableModel::tr("Edit connection..."),
								amvSlots, SLOT( execConnectionDialog() ) );
		contMenu->addAction( QPixmap( "icons:cancel.png" ),
								AutomatableModel::tr("Remove connection"),
								amvSlots, SLOT( removeConnection() ) );
	}
	else
	{
		menu->addAction( QPixmap( "icons:controller.png" ),
							AutomatableModel::tr("Connect to controller..."),
							amvSlots, SLOT( execConnectionDialog() ) );
	}
}




void AutomatableModelView::setModel( Model* model, bool isOldModelValid )
{
	ModelView::setModel( model, isOldModelValid );
}




void AutomatableModelView::mousePressEvent( QMouseEvent* event )
{
	if( event->button() == Qt::LeftButton && event->modifiers() & Qt::ControlModifier )
	{
		new StringPairDrag( "automatable_model", QString::number( modelUntyped()->id() ), QPixmap(), widget() );
		event->accept();
	}
	else if( event->button() == Qt::MidButton )
	{
		modelUntyped()->reset();
	}
}






AutomatableModelViewSlots::AutomatableModelViewSlots( AutomatableModelView* amv, QObject* parent ) :
	QObject(),
	m_amv( amv )
{
	connect( parent, SIGNAL( destroyed() ), this, SLOT( deleteLater() ), Qt::QueuedConnection );
}




void AutomatableModelViewSlots::execConnectionDialog()
{
	// TODO[pg]: Display a dialog with list of controllers currently in the song
	// in addition to any system MIDI controllers
	AutomatableModel* m = m_amv->modelUntyped();

	m->displayName();
	ControllerConnectionDialog d( gui->mainWindow(), m );

	if( d.exec() == 1 )
	{
		// Actually chose something
		if( d.chosenController() )
		{
			// Update
			if( m->controllerConnection() )
			{
				m->controllerConnection()->setController( d.chosenController() );
			}
			// New
			else
			{
				ControllerConnection* cc = new ControllerConnection( d.chosenController() );
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
}




void AutomatableModelViewSlots::removeConnection()
{
	AutomatableModel* m = m_amv->modelUntyped();

	if( m->controllerConnection() )
	{
		delete m->controllerConnection();
		m->setControllerConnection( NULL );
	}
}




void AutomatableModelViewSlots::editSongGlobalAutomation()
{
	gui->automationEditor()->open(
				AutomationPattern::globalAutomationPattern(m_amv->modelUntyped())
	);
}



void AutomatableModelViewSlots::removeSongGlobalAutomation()
{
	delete AutomationPattern::globalAutomationPattern( m_amv->modelUntyped() );
}


void AutomatableModelViewSlots::unlinkAllModels()
{
	m_amv->modelUntyped()->unlinkAllModels();
}



