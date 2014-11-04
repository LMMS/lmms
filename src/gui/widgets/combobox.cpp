/*
 * combobox.cpp - implementation of LMMS combobox
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008-2009 Paul Giblock <pgib/at/users.sourceforge.net>
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


#include "combobox.h"

#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QDesktopWidget>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QStyleOptionFrame>

#include "caption_menu.h"
#include "engine.h"
#include "embed.h"
#include "gui_templates.h"
#include "MainWindow.h"


QPixmap * comboBox::s_background = NULL;
QPixmap * comboBox::s_arrow = NULL;
QPixmap * comboBox::s_arrowSelected = NULL;

const int CB_ARROW_BTN_WIDTH = 20;


comboBox::comboBox( QWidget * _parent, const QString & _name ) :
	QWidget( _parent ),
	IntModelView( new ComboBoxModel( NULL, QString::null, true ), this ),
	m_menu( this ),
	m_pressed( false )
{
	if( s_background == NULL )
	{
		s_background = new QPixmap( embed::getIconPixmap( "combobox_bg" ) );
	}

	if( s_arrow == NULL )
	{
		s_arrow = new QPixmap( embed::getIconPixmap( "combobox_arrow" ) );
	}

	if( s_arrowSelected == NULL )
	{
		s_arrowSelected = new QPixmap( embed::getIconPixmap( "combobox_arrow_selected" ) );
	}

	setFont( pointSize<9>( font() ) );
	m_menu.setFont( pointSize<8>( m_menu.font() ) );

	connect( &m_menu, SIGNAL( triggered( QAction * ) ),
				this, SLOT( setItem( QAction * ) ) );

	setWindowTitle( _name );
	doConnections();
}




comboBox::~comboBox()
{
}



QSize comboBox::sizeHint() const
{
	int maxTextWidth = 0;
	for( int i = 0; model() && i < model()->size(); ++i )
	{
		int w = fontMetrics().width( model()->itemText( i ) );
		if( w > maxTextWidth )
		{
			maxTextWidth = w;
		}
	}

	return QSize( 32 + maxTextWidth, 22 );
}



void comboBox::contextMenuEvent( QContextMenuEvent * event )
{
	if( model() == NULL || event->x() <= width() - CB_ARROW_BTN_WIDTH )
	{
		QWidget::contextMenuEvent( event );
		return;
	}

	captionMenu contextMenu( model()->displayName() );
	addDefaultActions( &contextMenu );
	contextMenu.exec( QCursor::pos() );
}




void comboBox::mousePressEvent( QMouseEvent* event )
{
	if( model() == NULL )
	{
		return;
	}

	if( event->button() == Qt::LeftButton && ! ( event->modifiers() & Qt::ControlModifier ) )
	{
		if( event->x() > width() - CB_ARROW_BTN_WIDTH )
		{
			m_pressed = true;
			update();

			m_menu.clear();
			for( int i = 0; i < model()->size(); ++i )
			{
				QAction * a = m_menu.addAction( model()->itemPixmap( i ) ? model()->itemPixmap( i )->pixmap() : QPixmap(),
													model()->itemText( i ) );
				a->setData( i );
			}

			QPoint gpos = mapToGlobal( QPoint( 0, height() ) );
			if( gpos.y() + m_menu.sizeHint().height() < qApp->desktop()->height() )
			{
				m_menu.exec( gpos );
			}
			else
			{
				m_menu.exec( mapToGlobal( QPoint( width(), 0 ) ) );
			}
			m_pressed = false;
			update();
		}
		else if( event->button() == Qt::LeftButton )
		{
			model()->setInitValue( model()->value() + 1 );
			update();
		}
	}
	else if( event->button() == Qt::RightButton )
	{
		model()->setInitValue( model()->value() - 1 );
		update();
	}
	else
	{
		IntModelView::mousePressEvent( event );
	}
}




void comboBox::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );

	p.fillRect( 2, 2, width()-2, height()-4, *s_background );

	QColor shadow = palette().shadow().color();
	QColor highlight = palette().highlight().color();

	shadow.setAlpha( 124 );
	highlight.setAlpha( 124 );

	// button-separator
	p.setPen( shadow );
	p.drawLine( width() - CB_ARROW_BTN_WIDTH - 1, 1, width() - CB_ARROW_BTN_WIDTH - 1, height() - 3 );

	p.setPen( highlight );
	p.drawLine( width() - CB_ARROW_BTN_WIDTH, 1, width() - CB_ARROW_BTN_WIDTH, height() - 3 );

	// Border
	QStyleOptionFrame opt;
	opt.initFrom( this );
	opt.state = 0;

	style()->drawPrimitive( QStyle::PE_Frame, &opt, &p, this );

	QPixmap * arrow = m_pressed ? s_arrowSelected : s_arrow;

	p.drawPixmap( width() - CB_ARROW_BTN_WIDTH + 5, 4, *arrow );

	if( model() && model()->size() > 0 )
	{
		p.setFont( font() );
		p.setClipRect( QRect( 4, 2, width() - CB_ARROW_BTN_WIDTH - 8, height() - 2 ) );
		QPixmap pm = model()->currentData() ?  model()->currentData()->pixmap() : QPixmap();
		int tx = 5;
		if( !pm.isNull() )
		{
			if( pm.height() > 16 )
			{
				pm = pm.scaledToHeight( 16, Qt::SmoothTransformation );
			}
			p.drawPixmap( tx, 3, pm );
			tx += pm.width() + 3;
		}
		const int y = ( height()+p.fontMetrics().height() ) /2;
		p.setPen( QColor( 64, 64, 64 ) );
		p.drawText( tx+1, y-3, model()->currentText() );
		p.setPen( QColor( 224, 224, 224 ) );
		p.drawText( tx, y-4, model()->currentText() );
	}
}




void comboBox::wheelEvent( QWheelEvent* event )
{
	if( model() )
	{
		model()->setInitValue( model()->value() + ( ( event->delta() < 0 ) ? 1 : -1 ) );
		update();
		event->accept();
	}
}




void comboBox::setItem( QAction* item )
{
	if( model() )
	{
		model()->setInitValue( item->data().toInt() );
	}
}



#include "moc_combobox.cxx"


