/*
 * automatable_button.h - class automatableButton, the base for all buttons
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef _AUTOMATABLE_BUTTON_H
#define _AUTOMATABLE_BUTTON_H

#include <QtGui/QPushButton>

#include "automatable_object.h"


class automatableButtonGroup;


class automatableButton : public QPushButton, public automatableObject<bool,
			  					signed char>
{
	Q_OBJECT
public:
	automatableButton( QWidget * _parent, const QString & _name,
							track * _track );
	virtual ~automatableButton();


	virtual void setValue( const bool _on );

	inline void setCheckable( bool _on )
	{
		QPushButton::setCheckable( _on );
		setJournalling( _on );
	}


public slots:
	virtual void toggle( void );
	virtual void setChecked( bool _on )
	{
		// QPushButton::setChecked is called in setValue()
		setValue( _on );
	}


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );


private:
	automatableButtonGroup * m_group;


	friend class automatableButtonGroup;

} ;



class automatableButtonGroup : public QWidget, public automatableObject<int>
{
	Q_OBJECT
public:
	automatableButtonGroup( QWidget * _parent, const QString & _name,
							track * _track );
	virtual ~automatableButtonGroup();

	void addButton( automatableButton * _btn );
	void removeButton( automatableButton * _btn );

	void activateButton( automatableButton * _btn );

	virtual void setValue( const int _value );


private:
	QList<automatableButton *> m_buttons;


signals:
	void valueChanged( int );

} ;



#endif
