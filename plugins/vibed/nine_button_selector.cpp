/*
 * nine_button_selector.cpp
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/yahoo/com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef QT4

#include <QLabel>
#include <QMenu>

#else

#include <qwhatsthis.h>
#include <qlabel.h>
#include <qpopupmenu.h>
#include <qcursor.h>

#endif


#include "nine_button_selector.h"
#include "embed.h"

nineButtonSelector::nineButtonSelector(	QPixmap _button0_on,
					QPixmap _button0_off,
					QPixmap _button1_on,
					QPixmap _button1_off,
					QPixmap _button2_on,
					QPixmap _button2_off,
					QPixmap _button3_on,
					QPixmap _button3_off,
					QPixmap _button4_on,
					QPixmap _button4_off,
					QPixmap _button5_on,
					QPixmap _button5_off,
					QPixmap _button6_on,
					QPixmap _button6_off,
					QPixmap _button7_on,
					QPixmap _button7_off,
					QPixmap _button8_on,
					QPixmap _button8_off,
					Uint8 _default,
					Uint32 _x, Uint32 _y,
					QWidget * _parent,
					engine * _engine ):
	QWidget( _parent, "nineButtonSelector" ),
	engineObject( _engine ),
	m_selected( _default )
{
	setFixedSize( 50, 50 );
	m_base = QPixmap::grabWidget( _parent, _x, _y );
	move( _x, _y );
	setPaletteBackgroundPixmap( m_base );
	
	m_button = new pixmapButton( this, eng() );
	m_button->move( 1, 1 );
	m_button->setActiveGraphic( _button0_on );
	m_button->setInactiveGraphic( _button0_off );
	m_button->setChecked( FALSE );
	connect( m_button, SIGNAL ( clicked ( void ) ),
		 this, SLOT ( button0Clicked( void ) ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, eng() );
	m_button->move( 18, 1 );
	m_button->setActiveGraphic( _button1_on );
	m_button->setInactiveGraphic( _button1_off );
	m_button->setChecked( FALSE );
	connect( m_button, SIGNAL ( clicked ( void ) ),
		 this, SLOT ( button1Clicked( void ) ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, eng() );
	m_button->move( 35, 1 );
	m_button->setActiveGraphic( _button2_on );
	m_button->setInactiveGraphic( _button2_off );
	m_button->setChecked( FALSE );
	connect( m_button, SIGNAL ( clicked ( void ) ),
		 this, SLOT ( button2Clicked( void ) ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, eng() );
	m_button->move( 1, 18 );
	m_button->setActiveGraphic( _button3_on );
	m_button->setInactiveGraphic( _button3_off );
	m_button->setChecked( FALSE );
	connect( m_button, SIGNAL ( clicked ( void ) ),
		 this, SLOT ( button3Clicked( void ) ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, eng() );
	m_button->move( 18, 18 );
	m_button->setActiveGraphic( _button4_on );
	m_button->setInactiveGraphic( _button4_off );
	m_button->setChecked( FALSE );
	connect( m_button, SIGNAL ( clicked ( void ) ),
		 this, SLOT ( button4Clicked( void ) ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, eng() );
	m_button->move( 35, 18 );
	m_button->setActiveGraphic( _button5_on );
	m_button->setInactiveGraphic( _button5_off );
	m_button->setChecked( FALSE );
	connect( m_button, SIGNAL ( clicked ( void ) ),
		 this, SLOT ( button5Clicked( void ) ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, eng() );
	m_button->move( 1, 35 );
	m_button->setActiveGraphic( _button6_on );
	m_button->setInactiveGraphic( _button6_off );
	m_button->setChecked( FALSE );
	connect( m_button, SIGNAL ( clicked ( void ) ),
		 this, SLOT ( button6Clicked( void ) ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, eng() );
	m_button->move( 18, 35 );
	m_button->setActiveGraphic( _button7_on );
	m_button->setInactiveGraphic( _button7_off );
	m_button->setChecked( FALSE );
	connect( m_button, SIGNAL ( clicked ( void ) ),
		 this, SLOT ( button7Clicked( void ) ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, eng() );
	m_button->move( 35, 35 );
	m_button->setActiveGraphic( _button8_on );
	m_button->setInactiveGraphic( _button8_off );
	m_button->setChecked( FALSE );
	connect( m_button, SIGNAL ( clicked ( void ) ),
		 this, SLOT ( button8Clicked( void ) ) );
	m_buttons.append( m_button );
	
	m_lastBtn = m_buttons[_default];
	m_lastBtn->setChecked( TRUE );
}


nineButtonSelector::~ nineButtonSelector()
{
	for( Uint8 i = 0; i < 9; i++ )
	{
		delete m_buttons[i];
	}
}




void nineButtonSelector::button0Clicked( void )
{
	m_lastBtn->setChecked( FALSE);
	m_lastBtn = m_buttons[0];
	m_lastBtn->setChecked( TRUE );
	
	m_selected = 0;
	emit nineButtonSelection( m_selected );
}




void nineButtonSelector::button1Clicked( void )
{
	m_lastBtn->setChecked( FALSE);
	m_lastBtn = m_buttons[1];
	m_lastBtn->setChecked( TRUE );
	
	m_selected = 1;
	emit nineButtonSelection( m_selected );
}




void nineButtonSelector::button2Clicked( void )
{
	m_lastBtn->setChecked( FALSE);
	m_lastBtn = m_buttons[2];
	m_lastBtn->setChecked( TRUE );
	
	m_selected = 2;
	emit nineButtonSelection( m_selected );
}




void nineButtonSelector::button3Clicked( void )
{
	m_lastBtn->setChecked( FALSE);
	m_lastBtn = m_buttons[3];
	m_lastBtn->setChecked( TRUE );
	
	m_selected = 3;
	emit nineButtonSelection( m_selected );
}




void nineButtonSelector::button4Clicked( void )
{
	m_lastBtn->setChecked( FALSE);
	m_lastBtn = m_buttons[4];
	m_lastBtn->setChecked( TRUE );
	
	m_selected = 4;
	emit nineButtonSelection( m_selected );
}




void nineButtonSelector::button5Clicked( void )
{
	m_lastBtn->setChecked( FALSE);
	m_lastBtn = m_buttons[5];
	m_lastBtn->setChecked( TRUE );
	
	m_selected = 5;
	emit nineButtonSelection( m_selected );
}




void nineButtonSelector::button6Clicked( void )
{
	m_lastBtn->setChecked( FALSE);
	m_lastBtn = m_buttons[6];
	m_lastBtn->setChecked( TRUE );
	
	m_selected = 6;
	emit nineButtonSelection( m_selected );
}




void nineButtonSelector::button7Clicked( void )
{
	m_lastBtn->setChecked( FALSE);
	m_lastBtn = m_buttons[7];
	m_lastBtn->setChecked( TRUE );
	
	m_selected = 7;
	emit nineButtonSelection( m_selected );
}




void nineButtonSelector::button8Clicked( void )
{
	m_lastBtn->setChecked( FALSE);
	m_lastBtn = m_buttons[8];
	m_lastBtn->setChecked( TRUE );
	
	m_selected = 8;
	emit nineButtonSelection( m_selected );
}




void nineButtonSelector::contextMenuEvent( QContextMenuEvent * )
{
	QMenu contextMenu( this );
#ifdef QT4
	contextMenu.setTitle( accessibleName() );
#else
	QLabel * caption = new QLabel( "<font color=white><b>" +
			QString( "Selector" ) + "</b></font>", this );
	caption->setPaletteBackgroundColor( QColor( 0, 0, 192 ) );
	caption->setAlignment( Qt::AlignCenter );
	contextMenu.addAction( caption );
#endif
	contextMenu.addAction( embed::getIconPixmap( "help" ), tr( "&Help" ),
			       this, SLOT( displayHelp() ) );
	contextMenu.exec( QCursor::pos() );
}




void nineButtonSelector::displayHelp( void )
{
#ifdef QT4
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ),
			      whatsThis() );
#else
	QWhatsThis::display( QWhatsThis::textFor( this ), mapToGlobal(
						rect().bottomRight() ) );
#endif
}




#include "nine_button_selector.moc"
