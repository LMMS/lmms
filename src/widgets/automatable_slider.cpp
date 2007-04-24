#ifndef SINGLE_SOURCE_COMPILE

/*
 * automatable_slider.cpp - implementation of class automatableSlider
 *
 * Copyright (c) 2006-2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#include "automatable_slider.h"

#ifndef QT3

#include <QtGui/QCursor>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMouseEvent>

#else

#include <qcursor.h>
#include <qlabel.h>
#include <qpopupmenu.h>

#endif

#include "automatable_object_templates.h"
#include "embed.h"




automatableSlider::automatableSlider( QWidget * _parent, const QString & _name,
							class track * _track ) :
	QSlider( _parent
#ifndef QT4
			, _name.ascii()
#endif
		),
	m_show_status( FALSE )
{
	m_knob = new knob( knobDark_28, NULL, _name, _track );

#ifdef QT4
	setAccessibleName( _name );
#endif

	connect( m_knob, SIGNAL( valueChanged( float ) ), this,
						SLOT( updateSlider( void ) ) );
	connect( this, SIGNAL( valueChanged( int ) ), this,
						SLOT( changeValue( int ) ) );
	connect( this, SIGNAL( sliderMoved( int ) ), this,
						SLOT( moveSlider( int ) ) );
}




automatableSlider::~automatableSlider()
{
	delete m_knob;
}




void automatableSlider::setRange( int _min, int _max )
{
	QSlider::setRange( _min, _max );
	m_knob->setRange( _min, _max, 1.0f );
}




void automatableSlider::setValue( int _value )
{
	QSlider::setValue( _value );
	m_knob->setValue( minimum() + maximum() - _value );
}




void automatableSlider::setInitValue( int _value )
{
	m_knob->setInitValue( _value );
	QSlider::setValue( minimum() + maximum() - _value );
}




void automatableSlider::contextMenuEvent( QContextMenuEvent * _me )
{
	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent( NULL );

	QMenu contextMenu( this );
#ifdef QT4
	contextMenu.setTitle( accessibleName() );
#else
	QLabel * caption = new QLabel( "<font color=white><b>" +
			QString( accessibleName() ) + "</b></font>", this );
	caption->setPaletteBackgroundColor( QColor( 0, 0, 192 ) );
	caption->setAlignment( Qt::AlignCenter );
	contextMenu.addAction( caption );
#endif
	contextMenu.addAction( embed::getIconPixmap( "automation" ),
					tr( "&Open in automation editor" ),
					m_knob->getAutomationPattern(),
					SLOT( openInAutomationEditor() ) );
	contextMenu.exec( QCursor::pos() );
}




void automatableSlider::mousePressEvent( QMouseEvent * _me )
{
	m_show_status = TRUE;
	QSlider::mousePressEvent( _me );
}




void automatableSlider::mouseReleaseEvent( QMouseEvent * _me )
{
	m_show_status = FALSE;
	QSlider::mouseReleaseEvent( _me );
}




void automatableSlider::wheelEvent( QWheelEvent * _me )
{
	bool old_status = m_show_status;
	m_show_status = TRUE;
	QSlider::wheelEvent( _me );
	m_show_status = old_status;
}




void automatableSlider::changeValue( int _value )
{
	setValue( _value );
	emit logicValueChanged( logicValue() );
}




void automatableSlider::moveSlider( int _value )
{
	setValue( _value );
	emit logicSliderMoved( logicValue() );
}




void automatableSlider::updateSlider( void )
{
	QSlider::setValue( minimum() + maximum() - logicValue() );
}




void automatableSlider::saveSettings( QDomDocument & _doc, QDomElement & _this,
							const QString & _name )
{
	m_knob->saveSettings( _doc, _this, _name );
}




void automatableSlider::loadSettings( const QDomElement & _this,
							const QString & _name )
{
	m_knob->loadSettings( _this, _name );
}




int automatableSlider::logicValue( void )
{
	return( (int)roundf( m_knob->value() ) );
}




void automatableSlider::clearAutomationValues( void )
{
	m_knob->getAutomationPattern()->clear();
}




#include "automatable_slider.moc"

#endif
