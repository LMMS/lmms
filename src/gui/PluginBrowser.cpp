/*
 * PluginBrowser.cpp - implementation of the plugin-browser
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "PluginBrowser.h"

#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <QTreeWidget>

#include "embed.h"
#include "Engine.h"
#include "gui_templates.h"
#include "StringPairDrag.h"
#include "PluginFactory.h"


PluginBrowser::PluginBrowser( QWidget * _parent ) :
	SideBarWidget( tr( "Instrument Plugins" ),
				embed::getIconPixmap( "plugins" ).transformed( QTransform().rotate( 90 ) ), _parent )
{
	setWindowTitle( tr( "Instrument browser" ) );
	m_view = new QWidget( contentParent() );
	//m_view->setFrameShape( QFrame::NoFrame );

	addContentWidget( m_view );

	QVBoxLayout * view_layout = new QVBoxLayout( m_view );
	view_layout->setMargin( 5 );
	view_layout->setSpacing( 5 );


	auto hint = new QLabel( tr( "Drag an instrument "
					"into either the Song-Editor, the "
					"Beat+Bassline Editor or into an "
					"existing instrument track." ),
								m_view );
	hint->setWordWrap( true );

	QLineEdit * searchBar = new QLineEdit( m_view );
	searchBar->setPlaceholderText( "Search" );
	searchBar->setMaxLength( 64 );
	searchBar->setClearButtonEnabled( true );

	m_descTree = new QTreeWidget( m_view );
	m_descTree->setColumnCount( 1 );
	m_descTree->header()->setVisible( false );
	m_descTree->setIndentation( 10 );
	m_descTree->setSelectionMode( QAbstractItemView::NoSelection );

	connect( searchBar, SIGNAL( textEdited( const QString & ) ),
			this, SLOT( onFilterChanged( const QString & ) ) );

	view_layout->addWidget( hint );
	view_layout->addWidget( searchBar );
	view_layout->addWidget( m_descTree );

	// Add LMMS root to the tree
	m_lmmsRoot = new QTreeWidgetItem();
	m_lmmsRoot->setText( 0, "LMMS" );
	m_descTree->insertTopLevelItem( 0, m_lmmsRoot );
	m_lmmsRoot->setExpanded( true );

	// Add LV2 root to the tree
	m_lv2Root = new QTreeWidgetItem();
	m_lv2Root->setText( 0, "LV2" );
	m_descTree->insertTopLevelItem( 1, m_lv2Root );

	// Add plugins to the tree roots
	addPlugins();

	// Resize
	m_descTree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

	// Hide empty roots
	updateRootVisibilities();
}


void PluginBrowser::updateRootVisibility( int rootIndex )
{
	QTreeWidgetItem * root = m_descTree->topLevelItem( rootIndex );
	root->setHidden( !root->childCount() );
}


void PluginBrowser::updateRootVisibilities()
{
	int rootCount = m_descTree->topLevelItemCount();
	for (int rootIndex = 0; rootIndex < rootCount; ++rootIndex)
	{
		updateRootVisibility( rootIndex );
	}
}


void PluginBrowser::onFilterChanged( const QString & filter )
{
	int rootCount = m_descTree->topLevelItemCount();
	for (int rootIndex = 0; rootIndex < rootCount; ++rootIndex)
	{
		QTreeWidgetItem * root = m_descTree->topLevelItem( rootIndex );

		int itemCount = root->childCount();
		for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex)
		{
			QTreeWidgetItem * item = root->child( itemIndex );
			PluginDescWidget * descWidget = static_cast<PluginDescWidget *>
							(m_descTree->itemWidget( item, 0));
			if (descWidget->name().contains(filter, Qt::CaseInsensitive))
			{
				item->setHidden( false );
			}
			else
			{
				item->setHidden( true );
			}
		}
	}
}


void PluginBrowser::addPlugins()
{
	QList<Plugin::Descriptor*> descs = pluginFactory->descriptors(Plugin::Instrument);
	std::sort(
			descs.begin(),
			descs.end(),
			[]( const Plugin::Descriptor* d1, const Plugin::Descriptor* d2 ) -> bool
			{
				return qstricmp( d1->displayName, d2->displayName ) < 0 ? true : false;
			}
	);

	typedef Plugin::Descriptor::SubPluginFeatures::KeyList PluginKeyList;
	typedef Plugin::Descriptor::SubPluginFeatures::Key PluginKey;
	PluginKeyList subPluginKeys, pluginKeys;

	for (const Plugin::Descriptor* desc: descs)
	{
		if ( desc->subPluginFeatures )
		{
			desc->subPluginFeatures->listSubPluginKeys(
							desc,
							subPluginKeys );
		}
		else
		{
			pluginKeys << PluginKey( desc, desc->name );
		}
	}

	pluginKeys += subPluginKeys;

	for (const PluginKey& key : pluginKeys)
	{
		QTreeWidgetItem * item = new QTreeWidgetItem();
		if ( key.desc->name == QStringLiteral("lv2instrument") )
		{
			m_lv2Root->addChild( item );
		}
		else
		{
			m_lmmsRoot->addChild( item );
		}
		PluginDescWidget* p = new PluginDescWidget( key, m_descTree );
		m_descTree->setItemWidget( item, 0, p );
	}
}




PluginDescWidget::PluginDescWidget(const PluginKey &_pk,
							QWidget * _parent ) :
	QWidget( _parent ),
	m_pluginKey( _pk ),
	m_logo( _pk.logo()->pixmap() ),
	m_mouseOver( false )
{
	setFixedHeight( DEFAULT_HEIGHT );
	setMouseTracking( true );
	setCursor( Qt::PointingHandCursor );
	setToolTip(_pk.description());
}




QString PluginDescWidget::name() const
{
	return m_pluginKey.displayName();
}




void PluginDescWidget::paintEvent( QPaintEvent * )
{

	QPainter p( this );

	// Paint everything according to the style sheet
	QStyleOption o;
	o.initFrom( this );
	style()->drawPrimitive( QStyle::PE_Widget, &o, &p, this );

	// Draw the rest
	const int s = 16 + ( 32 * ( qBound( 24, height(), 60 ) - 24 ) ) /
								( 60 - 24 );
	const QSize logo_size( s, s );
	QPixmap logo = m_logo.scaled( logo_size, Qt::KeepAspectRatio,
						Qt::SmoothTransformation );
	p.drawPixmap( 4, 4, logo );

	QFont f = p.font();
	if ( m_mouseOver )
	{
		f.setBold( true );
	}

	p.setFont( f );
	p.drawText( 10 + logo_size.width(), 15, m_pluginKey.displayName());
}




void PluginDescWidget::enterEvent( QEvent * _e )
{
	m_mouseOver = true;

	QWidget::enterEvent( _e );
}




void PluginDescWidget::leaveEvent( QEvent * _e )
{
	m_mouseOver = false;

	QWidget::leaveEvent( _e );
}




void PluginDescWidget::mousePressEvent( QMouseEvent * _me )
{
	if ( _me->button() == Qt::LeftButton )
	{
		Engine::setDndPluginKey(&m_pluginKey);
		new StringPairDrag("instrument",
			QString::fromUtf8(m_pluginKey.desc->name), m_logo, this);
		leaveEvent( _me );
	}
}








