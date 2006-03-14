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

#include <QWidget>

#else

#include <qwidget.h>

#endif


#include "automatable_object.h"


class automatableButtonGroup;


class automatableButton : public QWidget, public automatableObject<bool>
{
	Q_OBJECT
public:
	automatableButton( QWidget * _parent, engine * _engine );
	virtual ~automatableButton();


	inline virtual bool isChecked( void ) const
	{
		return( value() );
	}

	inline void setToggleButton( bool _on )
	{
		m_toggleButton = _on;
		setStepRecording( m_toggleButton );
	}


public slots:
	virtual void toggle( void );
	virtual void setChecked( bool _on );


protected:
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );


private:
	automatableButtonGroup * m_group;
	bool m_toggleButton;


	friend class automatableButtonGroup;


signals:
	void clicked( void );
	void toggled( bool );

} ;



class automatableButtonGroup : public QObject, public automatableObject<int>
{
	Q_OBJECT
public:
	automatableButtonGroup( QObject * _parent, engine * _engine );
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
