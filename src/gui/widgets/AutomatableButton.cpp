/*
 * AutomatableButton.cpp - implementation of class AutomatableButton and
 *                         AutomatableButtonGroup
 *
 * Copyright (c) 2006-2011 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include "AutomatableButton.h"

#include <QMouseEvent>

#include "CaptionMenu.h"
#include "StringPairDrag.h"
#include "KeyboardShortcuts.h"


namespace lmms::gui
{

AutomatableButton::AutomatableButton( QWidget * _parent,
						const QString & _name ) :
	QPushButton( _parent ),
	BoolModelView( new BoolModel( false, nullptr, _name, true ), this ),
	m_group( nullptr )
{
	setWindowTitle( _name );
	doConnections();
	setFocusPolicy( Qt::NoFocus );
}




AutomatableButton::~AutomatableButton()
{
	if( m_group != nullptr )
	{
		m_group->removeButton( this );
	}
}




void AutomatableButton::modelChanged()
{
	if( QPushButton::isChecked() != model()->value() )
	{
		QPushButton::setChecked( model()->value() );
	}
}




void AutomatableButton::update()
{
	if( QPushButton::isChecked() != model()->value() )
	{
		QPushButton::setChecked( model()->value() );
	}
	QPushButton::update();
}




void AutomatableButton::contextMenuEvent( QContextMenuEvent * _me )
{
	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent( nullptr );

	if ( m_group != nullptr )
	{
		CaptionMenu contextMenu( m_group->model()->displayName() );
		m_group->addDefaultActions( &contextMenu );
		contextMenu.exec( QCursor::pos() );
	}
	else
	{
		CaptionMenu contextMenu( model()->displayName() );
		addDefaultActions( &contextMenu );
		contextMenu.exec( QCursor::pos() );
	}

}




void AutomatableButton::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
			! ( _me->modifiers() & KBD_COPY_MODIFIER ) )
	{
        // User simply clicked, toggle if needed
		if( isCheckable() )
		{
			toggle();
		}
		_me->accept();
	}
	else
	{
		// Ctrl-clicked, need to prepare drag-drop
		if( m_group )
		{
			// A group, we must get process it instead
			auto groupView = (AutomatableModelView*)m_group;
			new StringPairDrag( "automatable_model",
					QString::number( groupView->modelUntyped()->id() ),
					QPixmap(), widget() );
			// TODO: ^^ Maybe use a predefined icon instead of the button they happened to select
			_me->accept();
		}
		else
		{
			// Otherwise, drag the standalone button
			AutomatableModelView::mousePressEvent( _me );
			QPushButton::mousePressEvent( _me );
		}
	}
}




void AutomatableButton::mouseReleaseEvent( QMouseEvent * _me )
{
	if( _me && _me->button() == Qt::LeftButton )
	{
		emit clicked();
	}
}




void AutomatableButton::toggle()
{
	if( isCheckable() && m_group != nullptr )
	{
		if( model()->value() == false )
		{
			m_group->activateButton( this );
		}
	}
	else
	{
		model()->setValue( !model()->value() );
	}
}








AutomatableButtonGroup::AutomatableButtonGroup( QWidget * _parent,
						const QString & _name ) :
	QWidget( _parent ),
	IntModelView( new IntModel( 0, 0, 0, nullptr, _name, true ), this )
{
	hide();
	setWindowTitle( _name );
}




AutomatableButtonGroup::~AutomatableButtonGroup()
{
	for (const auto& button : m_buttons)
	{
		button->m_group = nullptr;
	}
}




void AutomatableButtonGroup::addButton( AutomatableButton * _btn )
{
	_btn->m_group = this;
	_btn->setCheckable( true );
	_btn->model()->setValue( false );
	// disable journalling as we're recording changes of states of 
	// button-group members on our own
	_btn->model()->setJournalling( false );

	m_buttons.push_back( _btn );
	model()->setRange( 0, m_buttons.size() - 1 );
	updateButtons();
}




void AutomatableButtonGroup::removeButton( AutomatableButton * _btn )
{
	m_buttons.erase( std::find( m_buttons.begin(), m_buttons.end(), _btn ) );
	_btn->m_group = nullptr;
}




void AutomatableButtonGroup::activateButton( AutomatableButton * _btn )
{
	if( _btn != m_buttons[model()->value()] &&
					m_buttons.indexOf( _btn ) != -1 )
	{
		model()->setValue( m_buttons.indexOf( _btn ) );
		for( AutomatableButton * btn : m_buttons )
		{
			btn->update();
		}
	}
}




void AutomatableButtonGroup::modelChanged()
{
	connect( model(), SIGNAL(dataChanged()),
			this, SLOT(updateButtons()));
	IntModelView::modelChanged();
	updateButtons();
}




void AutomatableButtonGroup::updateButtons()
{
	model()->setRange( 0, m_buttons.size() - 1 );
	int i = 0;
	for( AutomatableButton * btn : m_buttons )
	{
		btn->model()->setValue( i == model()->value() );
		++i;
	}
}



} // namespace lmms::gui
