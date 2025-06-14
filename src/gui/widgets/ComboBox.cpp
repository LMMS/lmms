/*
 * ComboBox.cpp - implementation of LMMS combobox
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008-2009 Paul Giblock <pgib/at/users.sourceforge.net>
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


#include "ComboBox.h"

#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionFrame>
#include <QScreen>

#include "CaptionMenu.h"
#include "FontHelper.h"

#define QT_SUPPORTS_WIDGET_SCREEN (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
#if !QT_SUPPORTS_WIDGET_SCREEN
#include <QApplication>
#include <QDesktopWidget>
#endif

namespace lmms::gui
{
const int CB_ARROW_BTN_WIDTH = 18;


ComboBox::ComboBox( QWidget * _parent, const QString & _name ) :
	QWidget( _parent ),
	IntModelView( new ComboBoxModel( nullptr, QString(), true ), this ),
	m_menu( this ),
	m_pressed( false )
{
	setFixedHeight( ComboBox::DEFAULT_HEIGHT );

	setFont(adjustedToPixelSize(font(), DEFAULT_FONT_SIZE));

	connect( &m_menu, SIGNAL(triggered(QAction*)),
				this, SLOT(setItem(QAction*)));

	setWindowTitle( _name );
	doConnections();
}






void ComboBox::selectNext()
{
	model()->setValue(model()->value() + 1);
}




void ComboBox::selectPrevious()
{
	model()->setValue(model()->value() - 1);
}



void ComboBox::contextMenuEvent( QContextMenuEvent * event )
{
	if( model() == nullptr || event->x() <= width() - CB_ARROW_BTN_WIDTH )
	{
		QWidget::contextMenuEvent( event );
		return;
	}

	CaptionMenu contextMenu( model()->displayName() );
	addDefaultActions( &contextMenu );
	contextMenu.exec( QCursor::pos() );
}




void ComboBox::mousePressEvent( QMouseEvent* event )
{
	if( model() == nullptr )
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

			QPoint gpos = mapToGlobal(QPoint(0, height()));

			#if (QT_SUPPORTS_WIDGET_SCREEN)
			bool const menuCanBeFullyShown = screen()->geometry().contains(QRect(gpos, m_menu.sizeHint()));
			#else
			bool const menuCanBeFullyShown = gpos.y() + m_menu.sizeHint().height() < qApp->desktop()->height();
			#endif

			if (menuCanBeFullyShown)
			{
				m_menu.exec(gpos);
			}
			else
			{
				m_menu.exec(mapToGlobal(QPoint(width(), 0)));
			}

			m_pressed = false;
			update();
		}
		else if( event->button() == Qt::LeftButton )
		{
			selectNext();
			update();
		}
	}
	else if( event->button() == Qt::RightButton )
	{
		selectPrevious();
		update();
	}
	else
	{
		IntModelView::mousePressEvent( event );
	}
}




void ComboBox::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );

	p.fillRect(2, 2, width() - 2, height() - 4, m_background);

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
	opt.state = QStyle::StateFlag::State_None;

	style()->drawPrimitive( QStyle::PE_Frame, &opt, &p, this );

	auto arrow = m_pressed ? m_arrowSelected : m_arrow;

	p.drawPixmap(width() - CB_ARROW_BTN_WIDTH + 3, 4, arrow);

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




void ComboBox::wheelEvent( QWheelEvent* event )
{
	if( model() )
	{
		const int direction = (event->angleDelta().y() < 0 ? 1 : -1) * (event->inverted() ? -1 : 1);
		model()->setValue(model()->value() + direction);
		update();
		event->accept();
	}
}




void ComboBox::setItem( QAction* item )
{
	if( model() )
	{
		model()->setValue(item->data().toInt());
	}
}


} // namespace lmms::gui


