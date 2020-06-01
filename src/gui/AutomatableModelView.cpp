/*
 * AutomatableModelView.cpp - implementation of AutomatableModelView
 *
 * Copyright (c) 2011-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QMouseEvent>

#include "AutomatableModelView.h"
#include "AutomationPattern.h"
#include "ControllerConnectionDialog.h"
#include "ControllerConnection.h"
#include "embed.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "StringPairDrag.h"

#include "AutomationEditor.h"


static float floatFromClipboard(bool* ok=nullptr);

AutomatableModelView::AutomatableModelView( ::Model* model, QWidget* _this ) :
	ModelView( model, _this )
{
	widget()->setAcceptDrops( true );
	widget()->setCursor( QCursor( embed::getIconPixmap( "hand" ), 3, 3 ) );
}

void AutomatableModelView::addDefaultActions( QMenu* menu )
{
	AutomatableModel* model = modelUntyped();

	AutomatableModelViewSlots* amvSlots = new AutomatableModelViewSlots( this, menu );

	menu->addAction( embed::getIconPixmap( "reload" ),
						AutomatableModel::tr( "&Reset (%1%2)" ).
							arg( model->displayValue( model->initValue<float>() ) ).
							arg( m_unit ),
						model, SLOT( reset() ) );

	menu->addSeparator();
	menu->addAction( embed::getIconPixmap( "edit_copy" ),
						AutomatableModel::tr( "&Copy value (%1%2)" ).
							arg( model->displayValue( model->value<float>() ) ).
							arg( m_unit ),
						amvSlots, SLOT( copyToClipboard() ) );

	bool canPaste = true;
	const float valueToPaste = floatFromClipboard(&canPaste);
	const QString pasteDesc = canPaste ?
					AutomatableModel::tr( "&Paste value (%1%2)").
						arg( model->displayValue( valueToPaste ) ).
						arg( m_unit )
					: AutomatableModel::tr( "&Paste value");
	QAction* pasteAction = menu->addAction( embed::getIconPixmap( "edit_paste" ),
						pasteDesc, amvSlots, SLOT( pasteFromClipboard() ) );
	pasteAction->setEnabled(canPaste);

	menu->addSeparator();

	menu->addAction( embed::getIconPixmap( "automation" ),
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
		menu->addAction( embed::getIconPixmap( "edit-delete" ),
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

		QMenu* contMenu = menu->addMenu( embed::getIconPixmap( "controller" ), controllerTxt );

		contMenu->addAction( embed::getIconPixmap( "controller" ),
								AutomatableModel::tr("Edit connection..."),
								amvSlots, SLOT( execConnectionDialog() ) );
		contMenu->addAction( embed::getIconPixmap( "cancel" ),
								AutomatableModel::tr("Remove connection"),
								amvSlots, SLOT( removeConnection() ) );
	}
	else
	{
		menu->addAction( embed::getIconPixmap( "controller" ),
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

void AutomatableModelViewSlots::copyToClipboard()
{
	QClipboard* clipboard = QApplication::clipboard();
	clipboard->setText(QString::number(m_amv->value<float>()));
}

void AutomatableModelViewSlots::pasteFromClipboard()
{
	bool isNumber = false;
	const float number = floatFromClipboard(&isNumber);
	if (isNumber) {
		m_amv->modelUntyped()->setValue(number);
	}
}


/// Attempt to parse a float from the clipboard
static float floatFromClipboard(bool* ok)
{
	const QClipboard* clipboard = QApplication::clipboard();
	return clipboard->text().toFloat(ok);
}

