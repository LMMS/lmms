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

#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <QScrollArea>
#include <QStyleOption>

#include "embed.h"
#include "Engine.h"
#include "templates.h"
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

	QScrollArea* scrollarea = new QScrollArea( m_view );
	PluginDescList* descList = new PluginDescList( m_view );
	scrollarea->setWidget(descList);
	scrollarea->setWidgetResizable(true);

	view_layout->addWidget(hint);
	view_layout->addWidget(scrollarea);
}




PluginDescList::PluginDescList(QWidget *parent) :
	QWidget(parent)
{
	QVBoxLayout* layout = new QVBoxLayout(this);

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
		if( desc->subPluginFeatures )
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
		PluginDescWidget* p = new PluginDescWidget( key, this );
		p->show();
		layout->addWidget(p);
	}

	setLayout(layout);
	layout->addStretch();
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




void PluginDescWidget::paintEvent( QPaintEvent * )
{

	QPainter p( this );

	// Paint everything according to the style sheet
	QStyleOption o;
	o.initFrom( this );
	style()->drawPrimitive( QStyle::PE_Widget, &o, &p, this );

	// Draw the rest
	const int s = 16 + ( 32 * ( tLimit( height(), 24, 60 ) - 24 ) ) /
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
	if( _me->button() == Qt::LeftButton )
	{
		Engine::setDndPluginKey(&m_pluginKey);
		new StringPairDrag("instrument",
			QString::fromUtf8(m_pluginKey.desc->name), m_logo, this);
		leaveEvent( _me );
	}
}








