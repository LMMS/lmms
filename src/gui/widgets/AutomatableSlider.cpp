/*
 * AutomatableSlider.cpp - implementation of class AutomatableSlider
 *
 * Copyright (c) 2006-2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2007-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AutomatableSlider.h"

#include <QMouseEvent>

#include "CaptionMenu.h"


namespace lmms::gui
{


AutomatableSlider::AutomatableSlider( QWidget * _parent,
						const QString & _name ) :
	QSlider( _parent ),
	IntModelView( new IntModel( 0, 0, 0, nullptr, _name, true ), this ),
	m_showStatus( false )
{
	setWindowTitle( _name );

	connect( this, SIGNAL(valueChanged(int)),
					this, SLOT(changeValue(int)));
	connect( this, SIGNAL(sliderMoved(int)),
					this, SLOT(moveSlider(int)));
}







void AutomatableSlider::contextMenuEvent( QContextMenuEvent * _me )
{
	CaptionMenu contextMenu( model()->displayName() );
	addDefaultActions( &contextMenu );
	contextMenu.exec( QCursor::pos() );
}




void AutomatableSlider::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
	   ! ( _me->modifiers() & Qt::ControlModifier ) )
	{
		AutomatableModel* thisModel = model();
		if (thisModel)
		{
			thisModel->addJournalCheckPoint();
			thisModel->saveJournallingState(false);
		}

		m_showStatus = true;
		QSlider::mousePressEvent( _me );
	}
	else
	{
		IntModelView::mousePressEvent( _me );
	}
}




void AutomatableSlider::mouseReleaseEvent( QMouseEvent * _me )
{
	AutomatableModel* thisModel = model();
	if (thisModel)
	{
		thisModel->restoreJournallingState();
	}

	m_showStatus = false;
	QSlider::mouseReleaseEvent( _me );
}




void AutomatableSlider::wheelEvent( QWheelEvent * _me )
{
	bool old_status = m_showStatus;
	m_showStatus = true;
	QSlider::wheelEvent( _me );
	m_showStatus = old_status;
}




void AutomatableSlider::modelChanged()
{
	QSlider::setRange( model()->minValue(), model()->maxValue() );
	updateSlider();
	connect( model(), SIGNAL(dataChanged()),
				this, SLOT(updateSlider()));
}




void AutomatableSlider::changeValue( int _value )
{
	model()->setValue( _value );
	emit logicValueChanged( model()->value() );
}




void AutomatableSlider::moveSlider( int _value )
{
	model()->setValue( _value );
	emit logicSliderMoved( model()->value() );
}




void AutomatableSlider::updateSlider()
{
	QSlider::setValue( model()->value() );
}




} // namespace lmms::gui

