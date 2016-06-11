/*
 * SubWindow.cpp - Implementation of QMdiSubWindow that correctly tracks
 *   the geometry that windows should be restored to.
 *   Workaround for https://bugreports.qt.io/browse/QTBUG-256
 *
 * Copyright (c) 2015 Colin Wallace <wallace.colin.a@gmail.com>
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

#include "SubWindow.h"

#include <QMdiArea>
#include <QMoveEvent>
#include <QResizeEvent>

#include "embed.h"


SubWindow::SubWindow( QWidget *parent, Qt::WindowFlags windowFlags ) :
	QMdiSubWindow( parent, windowFlags ),
	m_buttonSize( 17, 17 ),
	m_titleBarHeight( 24 )
{
	// initialize the tracked geometry to whatever Qt thinks the normal geometry currently is.
	// this should always work, since QMdiSubWindows will not start as maximized
	m_trackedNormalGeom = normalGeometry();

	// inits the colors
	m_activeColor = Qt::SolidPattern;
	m_textShadowColor = Qt::black;
	m_borderColor = Qt::black;

	// close, minimize, maximize and restore (after minimizing) buttons
	m_closeBtn = new QPushButton( embed::getIconPixmap( "close" ), QString::null, this );
	m_closeBtn->resize( m_buttonSize );
	m_closeBtn->setFocusPolicy( Qt::NoFocus );
	m_closeBtn->setCursor( Qt::ArrowCursor );
	m_closeBtn->setAttribute( Qt::WA_NoMousePropagation );
	m_closeBtn->setToolTip( tr( "Close" ) );
	connect( m_closeBtn, SIGNAL( clicked( bool ) ), this, SLOT( close() ) );

	m_maximizeBtn = new QPushButton( embed::getIconPixmap( "maximize" ), QString::null, this );
	m_maximizeBtn->resize( m_buttonSize );
	m_maximizeBtn->setFocusPolicy( Qt::NoFocus );
	m_maximizeBtn->setCursor( Qt::ArrowCursor );
	m_maximizeBtn->setAttribute( Qt::WA_NoMousePropagation );
	m_maximizeBtn->setToolTip( tr( "Maximize" ) );
	connect( m_maximizeBtn, SIGNAL( clicked( bool ) ), this, SLOT( showMaximized() ) );

	m_minimizeBtn = new QPushButton( embed::getIconPixmap( "minimize" ), QString::null, this );
	m_minimizeBtn->resize( m_buttonSize );
	m_minimizeBtn->setFocusPolicy( Qt::NoFocus );
	m_minimizeBtn->setCursor( Qt::ArrowCursor );
	m_minimizeBtn->setAttribute( Qt::WA_NoMousePropagation );
	m_minimizeBtn->setToolTip( tr( "Minimize" ) );
	connect( m_minimizeBtn, SIGNAL( clicked( bool ) ), this, SLOT( showMinimized() ) );

	m_restoreBtn = new QPushButton( embed::getIconPixmap( "restore" ), QString::null, this );
	m_restoreBtn->resize( m_buttonSize );
	m_restoreBtn->setFocusPolicy( Qt::NoFocus );
	m_restoreBtn->setCursor( Qt::ArrowCursor );
	m_restoreBtn->setAttribute( Qt::WA_NoMousePropagation );
	m_restoreBtn->setToolTip( tr( "Restore" ) );
	connect( m_restoreBtn, SIGNAL( clicked( bool ) ), this, SLOT( showNormal() ) );

	// QLabel for the window title and the shadow effect
	m_shadow = new QGraphicsDropShadowEffect();
	m_shadow->setColor( m_textShadowColor );
	m_shadow->setXOffset( 1 );
	m_shadow->setYOffset( 1 );

	m_windowTitle = new QLabel( this );
	m_windowTitle->setFocusPolicy( Qt::NoFocus );
	m_windowTitle->setAttribute( Qt::WA_TransparentForMouseEvents, true );
	m_windowTitle->setGraphicsEffect( m_shadow );
}




void SubWindow::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	QRect rect( 0, 0, width(), m_titleBarHeight );
	bool isActive = SubWindow::mdiArea()->activeSubWindow() == this;

	p.fillRect( rect, isActive ? activeColor() : p.pen().brush() );

	// window border
	p.setPen( borderColor() );

	// bottom, left, and right lines
	p.drawLine( 0, height() - 1, width(), height() - 1 );
	p.drawLine( 0, m_titleBarHeight, 0, height() - 1 );
	p.drawLine( width() - 1, m_titleBarHeight, width() - 1, height() - 1 );

	// window icon
	QPixmap winicon( widget()->windowIcon().pixmap( m_buttonSize ) );
	p.drawPixmap( 3, 3, m_buttonSize.width(), m_buttonSize.height(), winicon );
}




void SubWindow::elideText( QLabel *label, QString text )
{
	QFontMetrics metrix( label->font() );
	int width = label->width() - 2;
	QString clippedText = metrix.elidedText( text, Qt::ElideRight, width );
	label->setText( clippedText );
}




QRect SubWindow::getTrueNormalGeometry() const
{
	return m_trackedNormalGeom;
}




QBrush SubWindow::activeColor() const
{
	return m_activeColor;
}




QColor SubWindow::textShadowColor() const
{
	return m_textShadowColor;
}




QColor SubWindow::borderColor() const
{
	return m_borderColor;
}




void SubWindow::setActiveColor( const QBrush & b )
{
	m_activeColor = b;
}




void SubWindow::setTextShadowColor( const QColor & c )
{
	m_textShadowColor = c;
}




void SubWindow::setBorderColor( const QColor &c )
{
	m_borderColor = c;
}




void SubWindow::moveEvent( QMoveEvent * event )
{
	QMdiSubWindow::moveEvent( event );
	// if the window was moved and ISN'T minimized/maximized/fullscreen,
	// then save the current position
	if( !isMaximized() && !isMinimized() && !isFullScreen() )
	{
		m_trackedNormalGeom.moveTopLeft( event->pos() );
	}
}




void SubWindow::resizeEvent( QResizeEvent * event )
{
	// button adjustments
	m_minimizeBtn->hide();
	m_maximizeBtn->hide();
	m_restoreBtn->hide();

	const int rightSpace = 3;
	const int buttonGap = 1;
	const int menuButtonSpace = 24;

	QPoint rightButtonPos( width() - rightSpace - m_buttonSize.width() , 3 );
	QPoint middleButtonPos( width() - rightSpace - ( 2 * m_buttonSize.width() ) - buttonGap, 3 );
	QPoint leftButtonPos( width() - rightSpace - ( 3 * m_buttonSize.width() ) - ( 2 * buttonGap ), 3 );

	// the buttonBarWidth depends on the number of buttons.
	// we need it to calculate the width of window title label
	int buttonBarWidth = rightSpace + m_buttonSize.width();

	// set the buttons on their positions.
	// the close button is always needed and on the rightButtonPos
	m_closeBtn->move( rightButtonPos );

	// here we ask: is the Subwindow maximizable and/or minimizable
	// then we set the buttons and show them if needed
	if( windowFlags() & Qt::WindowMaximizeButtonHint )
	{
		buttonBarWidth = buttonBarWidth + m_buttonSize.width() + buttonGap;
		m_maximizeBtn->move( middleButtonPos );
		m_maximizeBtn->show();
	}

	if( windowFlags() & Qt::WindowMinimizeButtonHint )
	{
		buttonBarWidth = buttonBarWidth + m_buttonSize.width() + buttonGap;
		if( m_maximizeBtn->isHidden() )
		{
			m_minimizeBtn->move( middleButtonPos );
		}
		else
		{
			m_minimizeBtn->move( leftButtonPos );
		}
		m_minimizeBtn->show();
		m_restoreBtn->hide();
		if( isMinimized() )
		{
			if( m_maximizeBtn->isHidden() )
			{
				m_restoreBtn->move( middleButtonPos );
			}
			else
			{
				m_restoreBtn->move( leftButtonPos );
			}
			m_restoreBtn->show();
			m_minimizeBtn->hide();
		}
	}

	// title QLabel adjustments
	m_windowTitle->setAlignment( Qt::AlignHCenter );
	m_windowTitle->setFixedWidth( widget()->width() - ( menuButtonSpace + buttonBarWidth ) );
	m_windowTitle->move( menuButtonSpace,
		( m_titleBarHeight / 2 ) - ( m_windowTitle->sizeHint().height() / 2 ) - 1 );

	// if minimized we can't use widget()->width(). We have to hard code the width,
	// as the width of all minimized windows is the same.
	if( isMinimized() )
	{
		m_windowTitle->setFixedWidth( 120 );
	}

	// truncate the label string if the window is to small. Adds "..."
	elideText( m_windowTitle, widget()->windowTitle() );
	m_windowTitle->setTextInteractionFlags( Qt::NoTextInteraction );
	m_windowTitle->adjustSize();

	QMdiSubWindow::resizeEvent( event );

	// if the window was resized and ISN'T minimized/maximized/fullscreen,
	// then save the current size
	if( !isMaximized() && !isMinimized() && !isFullScreen() )
	{
		m_trackedNormalGeom.setSize( event->size() );
	}
}
