#ifndef SINGLE_SOURCE_COMPILE

/*
 * automatable_button.cpp - implementation of class automatableButton and
 *                          automatableButtonGroup
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


#include "automatable_button.h"

#ifndef QT3

#include <QtGui/QCursor>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMouseEvent>

#else

#include <qcursor.h>
#include <qlabel.h>
#include <qpopupmenu.h>

#define indexOf findIndex

#endif

#include "embed.h"



automatableButton::automatableButton( QWidget * _parent, const QString & _name,
							track * _track ) :
	QWidget( _parent
#ifndef QT4
			, _name.ascii()
#endif
		),
	autoObj( _track, FALSE, FALSE, TRUE ),
	m_group( NULL ),
	m_checkable( FALSE )
{
	if( _track != NULL )
	{
		getAutomationPattern();
	}
	setInitValue( FALSE );
#ifdef QT4
	setAccessibleName( _name );
#endif
}




automatableButton::~automatableButton()
{
	if( m_group != NULL )
	{
		m_group->removeButton( this );
	}
}




void automatableButton::contextMenuEvent( QContextMenuEvent * _me )
{
	if( nullTrack() && ( m_group == NULL || m_group->nullTrack() ) )
	{
		QWidget::contextMenuEvent( _me );
		return;
	}

	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent( NULL );

	QWidget * target;
	automationPattern * pattern;
	if ( m_group != NULL )
	{
		target = m_group;
		pattern = m_group->getAutomationPattern();
	}
	else
	{
		target = this;
		pattern = getAutomationPattern();
	}

	QMenu contextMenu( target );
#ifdef QT4
	contextMenu.setTitle( target->accessibleName() );
#else
	QLabel * caption = new QLabel( "<font color=white><b>" +
			QString( target->accessibleName() ) + "</b></font>",
									this );
	caption->setPaletteBackgroundColor( QColor( 0, 0, 192 ) );
	caption->setAlignment( Qt::AlignCenter );
	contextMenu.addAction( caption );
#endif
	contextMenu.addAction( embed::getIconPixmap( "automation" ),
					tr( "&Open in automation editor" ),
					pattern,
					SLOT( openInAutomationEditor() ) );
	contextMenu.exec( QCursor::pos() );
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
	}
}




void automatableButton::setValue( const bool _on )
{
	if( _on != value() )
	{
		autoObj::setValue( _on );
		setFirstValue();
		update();
		emit( toggled( value() ) );
	}
}








automatableButtonGroup::automatableButtonGroup( QWidget * _parent,
							const QString & _name,
							track * _track ) :
	QWidget( _parent
#ifndef QT4
			, _name.ascii()
#endif
		),
	automatableObject<int>( _track )
{
	hide();
	if( _track != NULL )
	{
		getAutomationPattern();
	}
	setInitValue( 0 );
#ifdef QT4
	setAccessibleName( _name );
#endif
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
	if( _btn != m_buttons[value()] && m_buttons.indexOf( _btn ) != -1 )
	{
		setValue( m_buttons.indexOf( _btn ) );
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
		setFirstValue();
		m_buttons[value()]->setChecked( TRUE );
	}

	emit valueChanged( value() );
}


#ifdef QT3
#undef findIndex
#endif


#include "automatable_button.moc"

#endif
