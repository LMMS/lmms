/*
 * tab_button.h - declaration of class tabButton
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _TAB_BUTTON_H
#define _TAB_BUTTON_H

#include "qt3support.h"

#ifdef QT4

#include <QPushButton>

#else

#include <qpushbutton.h>

#endif



class tabButton : public QPushButton
{
	Q_OBJECT
public:
	tabButton( const QString & _text, int _id, QWidget * _parent ) :
		QPushButton( _text, _parent ),
		m_id( _id )
	{
		setCheckable( TRUE );
		connect( this, SIGNAL( clicked() ), this,
						SLOT( slotClicked() ) );
	}
	virtual ~tabButton()
	{
	}


signals:
	void clicked( int );


protected slots:
	void slotClicked( void )
	{
		emit clicked( m_id );
	}


private:
	int m_id;

} ;

#endif
