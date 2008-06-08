/*
 * mv_base.cpp - base for M/V-architecture of LMMS
 *
 * Copyright (c) 2007-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <assert.h>

#include <QtGui/QWidget>


#include "mv_base.h"



QString model::fullDisplayName( void ) const
{
	const QString & n = displayName();
	if( parentModel() ) 
	{
		if( !n.isEmpty() )
		{
			return parentModel()->fullDisplayName() + ": " + n;
		}
		else
		{
			return parentModel()->fullDisplayName();
		}
	}
	else
	{
		return n;
	}
}




modelView::modelView( model * _model ) :
	m_model( _model )
{
}




void modelView::setModel( model * _model, bool _old_model_valid )
{
	QWidget * w = dynamic_cast<QWidget *>( this );
	assert( w != NULL );

	if( _old_model_valid && m_model != NULL )
	{
		if( m_model->defaultConstructed() )
		{
			delete m_model;
		}
		else
		{
			m_model->disconnect( w );
		}
	}
	m_model = _model;

	doConnections();

	w->update();

	modelChanged();
}




void modelView::doConnections( void )
{
	if( m_model != NULL )
	{
		QWidget * w = dynamic_cast<QWidget *>( this );
		QObject::connect( m_model, SIGNAL( dataChanged() ),
				w, SLOT( update() ),
				Qt::QueuedConnection );

		QObject::connect( m_model, SIGNAL( propertiesChanged() ),
				w, SLOT( update() ), Qt::QueuedConnection );
	}
}


#include "mv_base.moc"

