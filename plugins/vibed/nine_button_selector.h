/*
 * nine_button_selector.h
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
#ifndef _NINE_BUTTON_SELECTOR_H
#define _NINE_BUTTON_SELECTOR_H

#include "pixmap_button.h"


class nineButtonSelector: public QWidget , public IntModelView
{
	Q_OBJECT
			
public:
	nineButtonSelector( 	QPixmap _button0_on,
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
				QWidget * _parent);
	virtual ~nineButtonSelector();
	
//	inline int getSelected() { 
//		return( castModel<nineButtonSelectorModel>()->value() );
//	};

protected:
	void setSelected( int _new_button );
	
public slots:
	void button0Clicked();
	void button1Clicked();
	void button2Clicked();
	void button3Clicked();
	void button4Clicked();
	void button5Clicked();
	void button6Clicked();
	void button7Clicked();
	void button8Clicked();
	void contextMenuEvent( QContextMenuEvent * );
	void displayHelp();
	
signals:
	void nineButtonSelection( int );
	
private:
	virtual void modelChanged();
	void updateButton( int );

	QList<pixmapButton *> m_buttons;
	pixmapButton * m_button;
	pixmapButton * m_lastBtn;

} ;

typedef IntModel nineButtonSelectorModel;

#endif
