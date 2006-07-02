/*
 * nine_button_selector.h
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
#ifndef _NINE_BUTTON_SELECTOR_H
#define _NINE_BUTTON_SELECTOR_H

#include "qt3support.h"
#include "config.h"
#include "types.h"
#include "pixmap_button.h"


class nineButtonSelector: public QWidget, public engineObject
{
	Q_OBJECT
			
public:
	nineButtonSelector( 	QPixmap _button1_on,
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
				QPixmap _button9_on,
				QPixmap _button9_off,
				Uint8 _default,
				Uint32 _x, Uint32 _y,
				QWidget * _parent,
				engine * _engine,
				track * _track );
	~nineButtonSelector();
	
	inline Uint8 getSelected() { return( m_selected ); };
	void FASTCALL setSelected( Uint8 _new_button );
	
public slots:
	void button0Clicked( void );
	void button1Clicked( void );
	void button2Clicked( void );
	void button3Clicked( void );
	void button4Clicked( void );
	void button5Clicked( void );
	void button6Clicked( void );
	void button7Clicked( void );
	void button8Clicked( void );
	void contextMenuEvent( QContextMenuEvent * );
	void displayHelp( void );
	
signals:
	void nineButtonSelection( Uint8 );
	
private:
	vlist<pixmapButton *> m_buttons;
	pixmapButton * m_button;
	pixmapButton * m_lastBtn;
	QPixmap m_base;
	
	Uint8 m_selected;
};
#endif
