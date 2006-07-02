/*
 * automatable_button.h - class automatableButton, the base for all buttons
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUTOMATABLE_BUTTON_H
#define _AUTOMATABLE_BUTTON_H

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QWidget>

#else

#include <qwidget.h>

#endif


#include "automatable_object.h"


class automatableButtonGroup;


class automatableButton : public QWidget, public automatableObject<bool,
			  					signed char>
{
	Q_OBJECT
public:
	automatableButton( QWidget * _parent, const QString & _name,
					engine * _engine, track * _track );
	virtual ~automatableButton();


	inline virtual bool isChecked( void ) const
	{
		return( value() );
	}

	virtual void setValue( const bool _on );

	inline void setCheckable( bool _on )
	{
		m_checkable = _on;
		setJournalling( m_checkable );
	}

	inline bool isCheckable( void ) const
	{
		return( m_checkable );
	}


public slots:
	virtual void toggle( void );
	virtual void setChecked( bool _on )
	{
		setValue( _on );
	}


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );


private:
	automatableButtonGroup * m_group;
	bool m_checkable;


	friend class automatableButtonGroup;


signals:
	void clicked( void );
	void toggled( bool );

} ;



class automatableButtonGroup : public QWidget, public automatableObject<int>
{
	Q_OBJECT
public:
	automatableButtonGroup( QWidget * _parent, const QString & _name,
					engine * _engine, track * _track );
	virtual ~automatableButtonGroup();

	void addButton( automatableButton * _btn );
	void removeButton( automatableButton * _btn );

	void activateButton( automatableButton * _btn );

	virtual void setValue( const int _value );


private:
	vlist<automatableButton *> m_buttons;


signals:
	void valueChanged( int );

} ;



#endif
