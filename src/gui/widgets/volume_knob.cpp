#ifndef SINGLE_SOURCE_COMPILE

/*
 * volume_knob.cpp - defines a knob that display it's value as either a
 *                   percentage or in dBV.
 *
 * Copyright (c) 2006-2008  Danny McRae <khjklujn/at/users.sourceforge.net>
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


#include <QtGui/QApplication>
#include <QtGui/QInputDialog>
#include <QtGui/QMouseEvent>

#include <math.h>

#include "volume_knob.h"
#include "main_window.h"
#include "config_mgr.h"
#include "engine.h"
#include "text_float.h"
#include "string_pair_drag.h"



volumeKnob::volumeKnob( int _knob_num, QWidget * _parent,
						const QString & _name ) :
	knob( _knob_num, _parent, _name )
{
}




volumeKnob::~volumeKnob()
{
}




//! Mouse press event handler
void volumeKnob::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
			   engine::getMainWindow()->isCtrlPressed() == FALSE )
	{
		model()->prepareJournalEntryFromOldVal();

		const QPoint & p = _me->pos();
		m_origMousePos = p;

		if( configManager::inst()->value( "knobs",
		    "classicalusability").toInt() )
		{
			m_mouseOffset = getValue( p ) - model()->value();
		}
		emit sliderPressed();

		if( !configManager::inst()->value( "knobs", "classicalusability"
						 ).toInt() )
		{
			QApplication::setOverrideCursor( Qt::BlankCursor );
		}

		QString val;
		if( configManager::inst()->value( "app", "displaydbv" ).toInt() )
		{
			val = QString( " %1 dBV" ).arg(
					20.0 * log10( model()->value() / 100.0 ),
					3, 'f', 2 );
		}
		else
		{
			val = QString( " %1%" ).arg( model()->value(), 3, 'f', 0 );
		}
		s_textFloat->setText( m_description + val );
		
		s_textFloat->moveGlobal( this,
				QPoint( width() + 2, 0 ) );
		s_textFloat->show();
		m_buttonPressed = TRUE;
	}
	else if( _me->button() == Qt::LeftButton &&
			engine::getMainWindow()->isCtrlPressed() == TRUE )
	{
		new stringPairDrag( "float_value", QString::number( model()->value() ),
				    			QPixmap(), this );
	}
	else if( _me->button() == Qt::MidButton )
	{
		model()->reset();
	}
}




void volumeKnob::mouseMoveEvent( QMouseEvent * _me )
{
	// TODO: merge code with knob::mouseMoveEvent
	if( m_buttonPressed == TRUE )
	{
		setPosition( _me->pos() );
		emit sliderMoved( model()->value() );
//		emit valueChanged();
		if( !configManager::inst()->value( "knobs",
		     "classicalusability").toInt() )
		{
			QCursor::setPos( mapToGlobal( m_origMousePos ) );
		}
	}

	QString val;
	if( configManager::inst()->value( "app", "displaydbv" ).toInt() )
	{
		val = QString( " %1 dBV" ).arg(
				20.0 * log10( model()->value() / 100.0 ),
				3, 'f', 2 );
	}
	else
	{
		val = QString( " %1%" ).arg( model()->value(), 3, 'f', 0 );
	}
	s_textFloat->setText( m_description + val );
}




void volumeKnob::wheelEvent( QWheelEvent * _we )
{
	// TODO: merge code with knob::mouseMoveEvent
	_we->accept();
	const int inc = ( _we->delta() > 0 ) ? 1 : -1;
	model()->incValue( inc );


	QString val;
	if( configManager::inst()->value( "app", "displaydbv" ).toInt() )
	{
		val = QString( " %1 dBV" ).arg(
				20.0 * log10( model()->value() / 100.0 ),
				3, 'f', 2 );
	}
	else
	{
		val = QString( " %1%" ).arg( model()->value(), 3, 'f', 0 );
	}
	s_textFloat->setText( m_description + val );
	
	s_textFloat->moveGlobal( this, QPoint( width() + 2, 0 ) );
	s_textFloat->setVisibilityTimeOut( 1000 );

	emit sliderMoved( model()->value() );
//	emit valueChanged();
}




void volumeKnob::enterValue( void )
{
	bool ok;
	float new_val;
	if( configManager::inst()->value( "app", "displaydbv" ).toInt() )
	{
		new_val = QInputDialog::getDouble(
			this, accessibleName(),
			tr( "Please enter a new value between "
					"-96.0 dBV and 6.0 dBV:" ),
				20.0 * log10( model()->value() / 100.0 ),
							-96.0, 6.0, 4, &ok );
		if( new_val <= -96.0 )
		{
			new_val = 0.0f;
		}
		else
		{
			new_val = pow( 10.0, ( new_val / 20.0 ) ) * 100.0;
		}
	}
	else
	{
		new_val = QInputDialog::getDouble(
				this, accessibleName(),
				tr( "Please enter a new value between "
						"%1 and %2:" ).
					arg( model()->minValue() ).
					arg( model()->maxValue() ),
					model()->value(),
					model()->minValue(),
					model()->maxValue(), 4, &ok );
	}

	if( ok )
	{
		model()->setValue( new_val );
	}
}




#include "volume_knob.moc"

#endif
