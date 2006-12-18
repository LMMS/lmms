#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect_select_dialog.cpp - dialog to choose effect plugin
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QLayout>
#include <QtGui/QPushButton>

#else

#include <qlayout.h>
#include <qpushbutton.h>

#endif

#include "effect_select_dialog.h"

#include "gui_templates.h"
#include "embed.h"


effectSelectDialog::effectSelectDialog( QWidget * _parent, engine * _engine ) :
	QDialog( _parent ),
	engineObject( _engine )
{
	setWindowIcon( embed::getIconPixmap( "setup_audio" ) );
	setWindowTitle( tr( "Effects Selector" ) );
	setModal( TRUE );

	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setSpacing( 10 );
	vlayout->setMargin( 10 );

	effectList * elist = new effectList( this, eng() );
	elist->setMinimumSize( 500, 400 );
	connect( elist, SIGNAL( doubleClicked( const effectKey & ) ),
				this, SLOT( selectPlugin() ) );
	connect( elist, SIGNAL( highlighted( const effectKey & ) ),
			this, SLOT( setSelection( const effectKey & ) ) );
	m_currentSelection = elist->getSelected();

	QWidget * buttons = new QWidget( this );
	QHBoxLayout * btn_layout = new QHBoxLayout( buttons );
	btn_layout->setSpacing( 0 );
	btn_layout->setMargin( 0 );

	
	QPushButton * select_btn = new QPushButton( 
					embed::getIconPixmap( "add" ),
					tr( "Add" ), buttons );
	connect( select_btn, SIGNAL( clicked() ), 
				this, SLOT( selectPlugin() ) );
	
/*	QPushButton * ports_btn = new QPushButton( 
					embed::getIconPixmap("ports" ), 
					tr( "Ports" ), buttons );
	connect( ports_btn, SIGNAL( clicked() ),
				this, SLOT( showPorts() ) );
*/	
	QPushButton * cancel_btn = new QPushButton( 
					embed::getIconPixmap( "cancel" ),
					tr( "Cancel" ), buttons );
	connect( cancel_btn, SIGNAL( clicked() ),
				this, SLOT( reject() ) );
	
	btn_layout->addStretch();
	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( select_btn );
/*	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( ports_btn );*/
	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( cancel_btn );
	btn_layout->addSpacing( 10 );

	vlayout->addWidget( elist );
	vlayout->addSpacing( 10 );
	vlayout->addWidget( buttons );
	vlayout->addSpacing( 10 );
	//vlayout->addStretch();

	show();
}




effectSelectDialog::~effectSelectDialog()
{
}




effect * effectSelectDialog::instantiateSelectedPlugin( void )
{
	if( !m_currentSelection.name.isEmpty() && m_currentSelection.desc )
	{
		effect::constructionData cd =
		{
			eng(),
			m_currentSelection
		} ;
		return( effect::instantiate( m_currentSelection.desc->name,
									cd ) );
	}
	return( NULL );
}




void effectSelectDialog::showPorts( void )
{
/*	ladspaPortDialog ports( m_currentSelection, eng() );
	ports.exec();*/
}




void effectSelectDialog::setSelection( const effectKey & _selection )
{
	m_currentSelection = _selection;
}




void effectSelectDialog::selectPlugin( void )
{
	accept();
}







effectList::effectList( QWidget * _parent, engine * _engine ) :
	QWidget( _parent ),
	engineObject( _engine ),
	m_descriptionWidgetParent( new QWidget( this ) ),
	m_descriptionWidget( NULL )
{
	plugin::getDescriptorsOfAvailPlugins( m_pluginDescriptors );

	for( vvector<plugin::descriptor>::iterator it =
						m_pluginDescriptors.begin();
					it != m_pluginDescriptors.end(); ++it )
	{
		if( it->type != plugin::Effect )
		{
			continue;
		}
		if( it->sub_plugin_features )
		{
			it->sub_plugin_features->listSubPluginKeys( eng(),
				// as iterators are always stated to be not
				// equal with pointers, we dereference the
				// iterator and take the address of the item,
				// so we're on the safe side and the compiler
				// likely will reduce that to just "it"
							&( *it ),
							m_effectKeys );
		}
		else
		{
			m_effectKeys << effectKey( &( *it ), it->name );

		}
	}

	QStringList plugin_names;
	for( effectKeyList::const_iterator it = m_effectKeys.begin();
						it != m_effectKeys.end(); ++it )
	{
		plugin_names += QString( ( *it ).desc->public_name ) +
			( ( ( *it ).desc->sub_plugin_features != NULL ) ?
							": " + ( *it ).name
						:
							"" );
	}

	m_pluginList = new Q3ListBox( this );
	m_pluginList->insertStringList( plugin_names );
	connect( m_pluginList, SIGNAL( highlighted( int ) ),
				SLOT( onHighlighted( int ) ) );	
	connect( m_pluginList, SIGNAL( doubleClicked( QListBoxItem * ) ),
				SLOT( onDoubleClicked( QListBoxItem * ) ) );
	QVBoxLayout * vboxl = new QVBoxLayout( this );
	vboxl->setMargin( 0 );
	vboxl->setSpacing( 10 );
	vboxl->addWidget( m_pluginList );
	vboxl->addWidget( m_descriptionWidgetParent );

	new QVBoxLayout( m_descriptionWidgetParent );

	if( m_pluginList->numRows() > 0 )
	{
		m_pluginList->setSelected( 0, true );
		onHighlighted( 0 );
	}
}




effectList::~effectList()
{
}




void effectList::onHighlighted( int _pluginIndex )
{
	m_currentSelection = m_effectKeys[_pluginIndex];
	delete m_descriptionWidget;
	m_descriptionWidget = NULL;
	if( m_currentSelection.desc &&
				m_currentSelection.desc->sub_plugin_features )
	{
		m_descriptionWidget = m_currentSelection.desc->
						sub_plugin_features->
			createDescriptionWidget( m_descriptionWidgetParent,
						eng(), m_currentSelection );
	}
	if( m_descriptionWidget != NULL )
	{
		dynamic_cast<QVBoxLayout *>(
				m_descriptionWidgetParent->layout() )->
					addWidget( m_descriptionWidget );
		m_descriptionWidget->show();
	}
	emit( highlighted( m_currentSelection ) );
}




void effectList::onDoubleClicked( QListBoxItem * _item )
{
	emit( doubleClicked( m_currentSelection ) );
}




void effectList::onAddButtonReleased()
{
	emit( addPlugin( m_currentSelection ) );
}



#include "effect_select_dialog.moc"

#endif
