/*
 * EffectSelectDialog.cpp - dialog to choose effect plugin
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QGroupBox>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QListView>
#include <QtGui/QPushButton>
#include <QtGui/QScrollArea>

#include "EffectSelectDialog.h"

#include "gui_templates.h"
#include "embed.h"


EffectSelectDialog::EffectSelectDialog( QWidget * _parent ) :
	QDialog( _parent )
{
	setWindowIcon( embed::getIconPixmap( "setup_audio" ) );
	setWindowTitle( tr( "Effects Selector" ) );
	setModal( true );

	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setSpacing( 10 );
	vlayout->setMargin( 10 );

	EffectListWidget * elist = new EffectListWidget( this );
	elist->setMinimumSize( 540, 400 );
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
	
	QPushButton * cancel_btn = new QPushButton( 
					embed::getIconPixmap( "cancel" ),
					tr( "Cancel" ), buttons );
	connect( cancel_btn, SIGNAL( clicked() ),
				this, SLOT( reject() ) );
	
	btn_layout->addStretch();
	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( select_btn );
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




EffectSelectDialog::~EffectSelectDialog()
{
}




Effect * EffectSelectDialog::instantiateSelectedPlugin( EffectChain * _parent )
{
	if( !m_currentSelection.name.isEmpty() && m_currentSelection.desc )
	{
		return Effect::instantiate( m_currentSelection.desc->name,
							_parent,
							&m_currentSelection );
	}
	return( NULL );
}




void EffectSelectDialog::setSelection( const EffectKey & _selection )
{
	m_currentSelection = _selection;
}




void EffectSelectDialog::selectPlugin()
{
	if( m_currentSelection.isValid() )
	{
		accept();
	}
}







EffectListWidget::EffectListWidget( QWidget * _parent ) :
	QWidget( _parent ),
	m_sourceModel(),
	m_model(),
	m_descriptionWidget( NULL )
{
	Plugin::getDescriptorsOfAvailPlugins( m_pluginDescriptors );

	for( QVector<Plugin::Descriptor>::iterator it =
						m_pluginDescriptors.begin();
					it != m_pluginDescriptors.end(); ++it )
	{
		if( it->type != Plugin::Effect )
		{
			continue;
		}
		if( it->sub_plugin_features )
		{
			it->sub_plugin_features->listSubPluginKeys(
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
			m_effectKeys << EffectKey( &( *it ), it->name );

		}
	}

	QStringList plugin_names;
	for( EffectKeyList::ConstIterator it = m_effectKeys.begin();
						it != m_effectKeys.end(); ++it )
	{
		plugin_names += QString( ( *it ).desc->displayName ) +
			( ( ( *it ).desc->sub_plugin_features != NULL ) ?
							": " + ( *it ).name
						:
							"" );
	}


	int row = 0;
	for( QStringList::iterator it = plugin_names.begin();
					it != plugin_names.end(); ++it )
	{
		m_sourceModel.setItem( row, 0, new QStandardItem( *it ) );
		++row;
	}

	m_model.setSourceModel( &m_sourceModel );
	m_model.setFilterCaseSensitivity( Qt::CaseInsensitive );

	m_filterEdit = new QLineEdit( this );
	connect( m_filterEdit, SIGNAL( textChanged( const QString & ) ),
		&m_model, SLOT( setFilterRegExp( const QString & ) ) );
	connect( m_filterEdit, SIGNAL( textChanged( const QString & ) ),
					this, SLOT( updateSelection() ) );

	m_pluginList = new QListView( this );
	m_pluginList->setModel( &m_model );
	QItemSelectionModel * sm = new QItemSelectionModel( &m_model );
	m_pluginList->setSelectionModel( sm );
	m_pluginList->setSelectionBehavior( QAbstractItemView::SelectRows );
	m_pluginList->setSelectionMode( QAbstractItemView::SingleSelection );
	m_pluginList->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	connect( sm, SIGNAL( currentRowChanged( const QModelIndex &,
						const QModelIndex & ) ),
			SLOT( rowChanged( const QModelIndex &,
						const QModelIndex & ) ) );	
	connect( m_pluginList, SIGNAL( doubleClicked( const QModelIndex & ) ),
			SLOT( onDoubleClicked( const QModelIndex & ) ) );

	QGroupBox * groupbox = new QGroupBox( tr( "Description" ), this );
	groupbox->setFixedHeight( 200 );

	QVBoxLayout * gbl = new QVBoxLayout( groupbox );
	gbl->setMargin( 0 );
	gbl->setSpacing( 10 );

	m_scrollArea = new QScrollArea( groupbox );
	m_scrollArea->setFrameStyle( 0 );

	gbl->addWidget( m_scrollArea );

	QVBoxLayout * vboxl = new QVBoxLayout( this );
	vboxl->setMargin( 0 );
	vboxl->setSpacing( 10 );
	vboxl->addWidget( m_filterEdit );
	vboxl->addWidget( m_pluginList );
	vboxl->addWidget( groupbox );

	updateSelection();
}




EffectListWidget::~EffectListWidget()
{
}




void EffectListWidget::rowChanged( const QModelIndex & _idx, const QModelIndex & )
{
	delete m_descriptionWidget;
	m_descriptionWidget = NULL;

	if( m_model.mapToSource( _idx ).row() < 0 )
	{
		// invalidate current selection
		m_currentSelection =
				Plugin::Descriptor::SubPluginFeatures::Key();
	}
	else
	{
		m_currentSelection =
				m_effectKeys[m_model.mapToSource( _idx ).row()];
	}
	if( m_currentSelection.desc &&
				m_currentSelection.desc->sub_plugin_features )
	{
		m_descriptionWidget = new QWidget;
		QVBoxLayout * l = new QVBoxLayout( m_descriptionWidget );
		l->setMargin( 4 );
		l->setSpacing( 0 );

		m_scrollArea->setWidget( m_descriptionWidget );

		m_currentSelection.desc->sub_plugin_features->
			fillDescriptionWidget( m_descriptionWidget,
							&m_currentSelection );
		foreach( QWidget * w,
				m_descriptionWidget->findChildren<QWidget *>() )
		{
			if( w->parent() == m_descriptionWidget )
			{
				l->addWidget( w );
			}
		}
		l->setSizeConstraint( QLayout::SetFixedSize );
		m_descriptionWidget->show();
	}
	emit highlighted( m_currentSelection );
}




void EffectListWidget::onDoubleClicked( const QModelIndex & )
{
	emit doubleClicked( m_currentSelection );
}




void EffectListWidget::updateSelection()
{
	// no valid selection anymore due to changed filter?
	if( m_pluginList->selectionModel()->selection().size() <= 0 )
	{
		// then select our first item
		m_pluginList->selectionModel()->select( m_model.index( 0, 0 ),
					QItemSelectionModel::ClearAndSelect );
		rowChanged( m_model.index( 0, 0 ), QModelIndex() );
	}
}


#include "moc_EffectSelectDialog.cxx"

