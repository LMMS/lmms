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

#include <QtGui/QCursor>
#include <QtGui/QMouseEvent>

#include "automatable_model_templates.h"
#include "caption_menu.h"
#include "embed.h"




automatableButton::automatableButton( QWidget * _parent,
						const QString & _name ) :
	QPushButton( _parent ),
	autoModelView(),
	m_group( NULL )
{
	setModel( new autoModel( FALSE, FALSE, TRUE, 1, NULL, TRUE ) );
	setAccessibleName( _name );
}




automatableButton::~automatableButton()
{
	if( m_group != NULL )
	{
		m_group->removeButton( this );
	}
}




void automatableButton::update( void )
{
	if( QPushButton::isChecked() != model()->value() )
	{
		QPushButton::setChecked( model()->value() );
	}
	QPushButton::update();
}




void automatableButton::contextMenuEvent( QContextMenuEvent * _me )
{
	if( model()->nullTrack() &&
			( m_group == NULL || m_group->model()->nullTrack() ) )
	{
		QPushButton::contextMenuEvent( _me );
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
		pattern = m_group->model()->getAutomationPattern();
	}
	else
	{
		target = this;
		pattern = model()->getAutomationPattern();
	}

	captionMenu contextMenu( target->accessibleName() );
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
		if( isCheckable() )
		{
			toggle();
		}
		_me->accept();
	}
	else
	{
		QPushButton::mousePressEvent( _me );
	}
}




void automatableButton::mouseReleaseEvent( QMouseEvent * _me )
{
	emit clicked();
}




void automatableButton::toggle( void )
{
	if( isCheckable() && m_group != NULL )
	{
		if( model()->value() == FALSE )
		{
			m_group->activateButton( this );
		}
	}
	else
	{
		model()->setValue( !model()->value() );
	}
}








automatableButtonGroup::automatableButtonGroup( QWidget * _parent,
						const QString & _name ) :
	QWidget( _parent ),
	autoModelView()
{
	setModel( new autoModel( 0, 0, 0, 1, NULL, TRUE ) );
	hide();
	setAccessibleName( _name );
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
	_btn->model()->setValue( FALSE );
	// disable journalling as we're recording changes of states of 
	// button-group members on our own
	_btn->model()->setJournalling( FALSE );

	m_buttons.push_back( _btn );
	model()->setRange( 0, m_buttons.size() - 1 );
	updateButtons();
}




void automatableButtonGroup::removeButton( automatableButton * _btn )
{
	m_buttons.erase( qFind( m_buttons.begin(), m_buttons.end(), _btn ) );
	_btn->m_group = NULL;

	model()->setRange( 0, m_buttons.size() - 1 );
}




void automatableButtonGroup::activateButton( automatableButton * _btn )
{
	if( _btn != m_buttons[model()->value()] &&
					m_buttons.indexOf( _btn ) != -1 )
	{
		model()->setValue( m_buttons.indexOf( _btn ) );
	}
}




void automatableButtonGroup::modelChanged( void )
{
	connect( model(), SIGNAL( dataChanged() ),
			this, SLOT( updateButtons() ) );
	autoModelView::modelChanged();
}




void automatableButtonGroup::updateButtons( void )
{
	int i = 0;
	foreach( automatableButton * btn, m_buttons )
	{
		btn->setValue( i == value() );
		++i;
	}
}


#include "automatable_button.moc"

#endif
