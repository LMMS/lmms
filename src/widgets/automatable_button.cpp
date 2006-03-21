#ifndef SINGLE_SOURCE_COMPILE

/*
 * automatable_button.cpp - implementation of class automatableButton and
 *                          automatableButtonGroup
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


#include "automatable_button.h"


automatableButton::automatableButton( QWidget * _parent, engine * _engine ) :
	QWidget( _parent ),
	autoObj( _engine, FALSE, TRUE, FALSE ),
	m_group( NULL ),
	m_checkable( FALSE )
{
}




automatableButton::~automatableButton()
{
	if( m_group != NULL )
	{
		m_group->removeButton( this );
	}
}




void automatableButton::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		if( m_checkable == FALSE )
		{
			setChecked( TRUE );
		}
		else
		{
			toggle();
		}
		_me->accept();
	}
	else
	{
		QWidget::mousePressEvent( _me );
	}
}




void automatableButton::mouseReleaseEvent( QMouseEvent * _me )
{
	if( m_checkable == FALSE )
	{
		setChecked( FALSE );
	}
	emit clicked();
}




void automatableButton::toggle( void )
{
	if( m_checkable == TRUE && m_group != NULL )
	{
		if( value() == FALSE )
		{
			m_group->activateButton( this );
		}
	}
	else
	{
		setValue( !value() );
		update();
		emit( toggled( value() ) );
	}
}




void automatableButton::setValue( const bool _on )
{
	if( _on != value() )
	{
		autoObj::setValue( _on );
		update();
		emit( toggled( value() ) );
	}
}








automatableButtonGroup::automatableButtonGroup( QObject * _parent,
							engine * _engine ) :
	QObject( _parent ),
	automatableObject<int>( _engine )
{
}




automatableButtonGroup::~automatableButtonGroup()
{
	while( m_buttons.empty() == FALSE )
	{
		removeButton( m_buttons.front() );
	}
}




void automatableButtonGroup::addButton( automatableButton * _btn )
{
	_btn->m_group = this;
	_btn->setCheckable( TRUE );
	_btn->setChecked( FALSE );
	// disable step-recording as we're recording changes of states of 
	// button-group members on our own
	_btn->setJournalling( FALSE );

	m_buttons.push_back( _btn );
	setRange( 0, m_buttons.size() - 1 );
}




void automatableButtonGroup::removeButton( automatableButton * _btn )
{
	m_buttons.erase( qFind( m_buttons.begin(), m_buttons.end(), _btn ) );
	_btn->m_group = NULL;

	setRange( 0, m_buttons.size() - 1 );
}




void automatableButtonGroup::activateButton( automatableButton * _btn )
{
	if( _btn != m_buttons[value()] && m_buttons.findIndex( _btn ) != -1 )
	{
		setValue( m_buttons.findIndex( _btn ) );
	}
}




void automatableButtonGroup::setValue( const int _value )
{
	if( m_buttons.empty() == FALSE )
	{
		// range not updated yet?
		if( value() == fittedValue( value() ) )
		{
			m_buttons[value()]->setChecked( FALSE );
		}
		automatableObject<int>::setValue( _value );
		m_buttons[value()]->setChecked( TRUE );
	}

	emit valueChanged( value() );
}



#include "automatable_button.moc"

#endif
