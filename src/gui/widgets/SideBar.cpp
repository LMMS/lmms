/*
 * SideBar.cpp - side-bar in LMMS' MainWindow
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QStyleOptionToolButton>
#include <QtGui/QStylePainter>
#include <QtGui/QToolButton>

#include "SideBar.h"
#include "SideBarWidget.h"
#include "tooltip.h"


// internal helper class allowing to create QToolButtons with
// vertical orientation
class SideBarButton : public QToolButton
{
public:
	SideBarButton( Qt::Orientation _orientation, QWidget * _parent ) :
		QToolButton( _parent ),
		m_orientation( _orientation )
	{
	}
	virtual ~SideBarButton()
	{
	}

	Qt::Orientation orientation() const
	{
		return m_orientation;
	}

	virtual QSize sizeHint() const
	{
		QSize s = QToolButton::sizeHint();
		s.setWidth( s.width() + 8 );
		if( orientation() == Qt::Horizontal )
		{
			return s;
		}
		return QSize( s.height(), s.width() );
	}


protected:
	virtual void paintEvent( QPaintEvent * )
	{
		QStylePainter p( this );
		QStyleOptionToolButton opt;
		initStyleOption( &opt );
		if( orientation() == Qt::Vertical )
		{
			const QSize s = sizeHint();
			p.rotate( 270 );
			p.translate( -s.height(), 0 );
			opt.rect = QRect( 0, 0, s.height(), s.width() );
		}
		p.drawComplexControl( QStyle::CC_ToolButton, opt );
	}


private:
	Qt::Orientation m_orientation;

} ;


SideBar::SideBar( Qt::Orientation _orientation, QWidget * _parent ) :
	QToolBar( _parent ),
	m_btnGroup( this )
{
	setOrientation( _orientation );
	setIconSize( QSize( 16, 16 ) );

	m_btnGroup.setExclusive( false );
	connect( &m_btnGroup, SIGNAL( buttonClicked( QAbstractButton * ) ),
				this, SLOT( toggleButton( QAbstractButton * ) ) );
}




SideBar::~SideBar()
{
}




void SideBar::appendTab( SideBarWidget * _sbw )
{
	SideBarButton * btn = new SideBarButton( orientation(), this );
	btn->setText( _sbw->title() );
	btn->setIcon( _sbw->icon() );
	btn->setCheckable( true );
	m_widgets[btn] = _sbw;
	m_btnGroup.addButton( btn );
	addWidget( btn );

	_sbw->hide();
	_sbw->setMinimumWidth( 200 );

	toolTip::add( btn, _sbw->title() );
}




void SideBar::toggleButton( QAbstractButton * _btn )
{
	QToolButton * toolButton = NULL;
	QWidget * activeWidget = NULL;
	for( ButtonMap::Iterator it = m_widgets.begin();
							it != m_widgets.end(); ++it )
	{
		QToolButton * curBtn = it.key();
		if( curBtn != _btn )
		{
			curBtn->setChecked( false );
			curBtn->setToolButtonStyle( Qt::ToolButtonIconOnly );
		}
		else
		{
			toolButton = it.key();
			activeWidget = it.value();
		}
		if( it.value() )
		{
			it.value()->hide();
		}
	}

	if( toolButton && activeWidget )
	{
		activeWidget->setVisible( _btn->isChecked() );
		toolButton->setToolButtonStyle( _btn->isChecked() ?
				Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly );
	}
}


#include "moc_SideBar.cxx"

