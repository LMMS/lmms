/*
 * nine_button_selector.cpp
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/yahoo/com>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "nine_button_selector.h"

#include <QtGui/QWhatsThis>

#include "caption_menu.h"
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
					int _default,
					int _x, int _y,
					QWidget * _parent ):
	QWidget( _parent ),
	IntModelView( new nineButtonSelectorModel(0, 8, _default, NULL, 
				QString::null, true ), this )
{
	setFixedSize( 50, 50 );
	move( _x, _y );

	m_button = new pixmapButton( this, NULL );
	m_button->move( 1, 1 );
	m_button->setActiveGraphic( _button0_on );
	m_button->setInactiveGraphic( _button0_off );
	m_button->setChecked( false );
	connect( m_button, SIGNAL ( clicked () ),
		 this, SLOT ( button0Clicked() ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, NULL );
	m_button->move( 18, 1 );
	m_button->setActiveGraphic( _button1_on );
	m_button->setInactiveGraphic( _button1_off );
	m_button->setChecked( false );
	connect( m_button, SIGNAL ( clicked () ),
		 this, SLOT ( button1Clicked() ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, NULL );
	m_button->move( 35, 1 );
	m_button->setActiveGraphic( _button2_on );
	m_button->setInactiveGraphic( _button2_off );
	m_button->setChecked( false );
	connect( m_button, SIGNAL ( clicked () ),
		 this, SLOT ( button2Clicked() ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, NULL );
	m_button->move( 1, 18 );
	m_button->setActiveGraphic( _button3_on );
	m_button->setInactiveGraphic( _button3_off );
	m_button->setChecked( false );
	connect( m_button, SIGNAL ( clicked () ),
		 this, SLOT ( button3Clicked() ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, NULL );
	m_button->move( 18, 18 );
	m_button->setActiveGraphic( _button4_on );
	m_button->setInactiveGraphic( _button4_off );
	m_button->setChecked( false );
	connect( m_button, SIGNAL ( clicked () ),
		 this, SLOT ( button4Clicked() ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, NULL );
	m_button->move( 35, 18 );
	m_button->setActiveGraphic( _button5_on );
	m_button->setInactiveGraphic( _button5_off );
	m_button->setChecked( false );
	connect( m_button, SIGNAL ( clicked () ),
		 this, SLOT ( button5Clicked() ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, NULL );
	m_button->move( 1, 35 );
	m_button->setActiveGraphic( _button6_on );
	m_button->setInactiveGraphic( _button6_off );
	m_button->setChecked( false );
	connect( m_button, SIGNAL ( clicked () ),
		 this, SLOT ( button6Clicked() ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, NULL );
	m_button->move( 18, 35 );
	m_button->setActiveGraphic( _button7_on );
	m_button->setInactiveGraphic( _button7_off );
	m_button->setChecked( false );
	connect( m_button, SIGNAL ( clicked () ),
		 this, SLOT ( button7Clicked() ) );
	m_buttons.append( m_button );
	
	m_button = new pixmapButton( this, NULL );
	m_button->move( 35, 35 );
	m_button->setActiveGraphic( _button8_on );
	m_button->setInactiveGraphic( _button8_off );
	m_button->setChecked( false );
	connect( m_button, SIGNAL ( clicked () ),
		 this, SLOT ( button8Clicked() ) );
	m_buttons.append( m_button );
	
	m_lastBtn = m_buttons[_default];
	m_lastBtn->setChecked( true );
}


nineButtonSelector::~ nineButtonSelector()
{
	for( int i = 0; i < 9; i++ )
	{
		delete m_buttons[i];
	}
}




void nineButtonSelector::button0Clicked()
{
	setSelected( 0 );
}




void nineButtonSelector::button1Clicked()
{
	setSelected( 1 );
}




void nineButtonSelector::button2Clicked()
{
	setSelected( 2 );
}




void nineButtonSelector::button3Clicked()
{
	setSelected( 3 );
}




void nineButtonSelector::button4Clicked()
{
	setSelected( 4 );
}




void nineButtonSelector::button5Clicked()
{
	setSelected( 5 );
}




void nineButtonSelector::button6Clicked()
{
	setSelected( 6 );
}




void nineButtonSelector::button7Clicked()
{
	setSelected( 7 );
}




void nineButtonSelector::button8Clicked()
{
	setSelected( 8 );
}

void nineButtonSelector::modelChanged()
{
	updateButton( model()->value() );
}

void nineButtonSelector::setSelected( int _new_button )
{
	model()->setValue(_new_button);
	updateButton( _new_button );
}

void nineButtonSelector::updateButton( int _new_button )
{
	m_lastBtn->setChecked( false );
	m_lastBtn->update();

	m_lastBtn = m_buttons[_new_button];
	m_lastBtn->setChecked( true );
	m_lastBtn->update();
	
	emit nineButtonSelection( _new_button );
}

void nineButtonSelector::contextMenuEvent( QContextMenuEvent * )
{
	captionMenu contextMenu( windowTitle(), this );
	contextMenu.addHelpAction();
	contextMenu.exec( QCursor::pos() );
}




void nineButtonSelector::displayHelp()
{
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ),
							      whatsThis() );
}




#include "moc_nine_button_selector.cxx"
