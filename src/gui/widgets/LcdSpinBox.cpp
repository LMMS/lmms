/*
 * lcd_spinbox.cpp - class LcdSpinBox, an improved QLCDNumber
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008 Paul Giblock <pgllama/at/gmail.com>
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

#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QFontMetrics>
#include <QtGui/QStyleOptionFrameV2>
#include <QtGui/QInputDialog>

#include "LcdSpinBox.h"
#include "caption_menu.h"
#include "engine.h"
#include "embed.h"
#include "gui_templates.h"
#include "templates.h"
#include "MainWindow.h"



LcdSpinBox::LcdSpinBox( int numDigits, QWidget* parent, const QString& name ) :
	LcdWidget( numDigits, parent, name ),
	IntModelView( new IntModel( 0, 0, 0, NULL, name, true ), this ),
	m_mouseMoving( false ),
	m_origMousePos(),
	m_displayOffset( 0 )
{
}




LcdSpinBox::LcdSpinBox( int numDigits, const QString& style, QWidget* parent, const QString& name ) :
	LcdWidget( numDigits, parent, name ),
	IntModelView( new IntModel( 0, 0, 0, NULL, name, true ), this ),
	m_mouseMoving( false ),
	m_origMousePos(),
	m_displayOffset( 0 )
{
}



LcdSpinBox::~LcdSpinBox()
{
}



void LcdSpinBox::update()
{
	setValue( model()->value() + m_displayOffset );

	QWidget::update();
}



void LcdSpinBox::contextMenuEvent( QContextMenuEvent* event )
{
	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent( NULL );

	captionMenu contextMenu( model()->displayName() );
	addDefaultActions( &contextMenu );
	contextMenu.exec( QCursor::pos() );
}




void LcdSpinBox::mousePressEvent( QMouseEvent* event )
{
	if( event->button() == Qt::LeftButton &&
		! ( event->modifiers() & Qt::ControlModifier ) &&
						event->y() < cellHeight() + 2  )
	{
		m_mouseMoving = true;
		m_origMousePos = event->globalPos();
		QApplication::setOverrideCursor( Qt::BlankCursor );

		AutomatableModel *thisModel = model();
		if( thisModel )
		{
			thisModel->addJournalCheckPoint();
			thisModel->saveJournallingState( false );
		}
	}
	else
	{
		IntModelView::mousePressEvent( event );
	}
}




void LcdSpinBox::mouseMoveEvent( QMouseEvent* event )
{
	if( m_mouseMoving )
	{
		int dy = event->globalY() - m_origMousePos.y();
		if( engine::mainWindow()->isShiftPressed() )
			dy = qBound( -4, dy/4, 4 );
		if( dy > 1 || dy < -1 )
		{
			model()->setInitValue( model()->value() -
						dy / 2 * model()->step<int>() );
			emit manualChange();
			QCursor::setPos( m_origMousePos );
		}
	}
}




void LcdSpinBox::mouseReleaseEvent( QMouseEvent* )
{
	if( m_mouseMoving )
	{
		model()->restoreJournallingState();

		QCursor::setPos( m_origMousePos );
		QApplication::restoreOverrideCursor();

		m_mouseMoving = false;
	}
}




void LcdSpinBox::wheelEvent( QWheelEvent * _we )
{
	_we->accept();
	model()->setInitValue( model()->value() +
			( ( _we->delta() > 0 ) ? 1 : -1 ) * model()->step<int>() );
	emit manualChange();
}

void LcdSpinBox::mouseDoubleClickEvent( QMouseEvent * )
{
	enterValue();
}

void LcdSpinBox::enterValue()
{
	bool ok;
	int new_val;

	new_val = QInputDialog::getInt(
			this, windowTitle(),
			tr( "Please enter a new value between %1 and %2:" ).
			arg( model()->minValue() ).
			arg( model()->maxValue() ),
			model()->value(),
			model()->minValue(),
			model()->maxValue(), 4, &ok );

	if( ok )
	{
		model()->setValue( new_val );
	}
}

#include "moc_LcdSpinBox.cxx"

