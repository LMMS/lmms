/*
 * tab_button.h - declaration of class tabButton
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _TAB_BUTTON_H
#define _TAB_BUTTON_H

#include <QtGui/QPushButton>


class tabButton : public QPushButton
{
	Q_OBJECT
public:
	tabButton( const QString & _text, int _id, QWidget * _parent ) :
		QPushButton( _text, _parent ),
		m_id( _id )
	{
		setCheckable( true );
		connect( this, SIGNAL( clicked() ), this,
						SLOT( slotClicked() ) );
	}

	~tabButton()
	{
	}


signals:
	void clicked( int );


protected slots:
	void slotClicked()
	{
		emit clicked( m_id );
	}


private:
	int m_id;

} ;

#endif
