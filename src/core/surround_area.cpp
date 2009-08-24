/*
 * surround_area.cpp - a widget for setting position of a channel +
 *                     calculation of volume for each speaker
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtXml/QDomElement>

#include "surround_area.h"
#include "templates.h"


surroundAreaModel::surroundAreaModel( ::Model * _parent,
						bool _default_constructed ) :
	Model( _parent, QString::null, _default_constructed ),
	m_posX( 0, -SURROUND_AREA_SIZE, SURROUND_AREA_SIZE, _parent ),
	m_posY( 0, -SURROUND_AREA_SIZE, SURROUND_AREA_SIZE, _parent )
{
	connect( &m_posX, SIGNAL( dataChanged() ),
					this, SIGNAL( dataChanged() ) );
	connect( &m_posY, SIGNAL( dataChanged() ),
					this, SIGNAL( dataChanged() ) );
}




surroundVolumeVector surroundAreaModel::getVolumeVector( float _v_scale ) const
{
	surroundVolumeVector v = { { _v_scale, _v_scale
#ifndef LMMS_DISABLE_SURROUND
					, _v_scale, _v_scale
#endif
			} } ;

	if( x() >= 0 )
	{
		v.vol[0] *= 1.0f - x() / (float)SURROUND_AREA_SIZE;
#ifndef LMMS_DISABLE_SURROUND
		v.vol[2] *= 1.0f - x() / (float)SURROUND_AREA_SIZE;
#endif
	}
	else
	{
		v.vol[1] *= 1.0f + x() / (float)SURROUND_AREA_SIZE;
#ifndef LMMS_DISABLE_SURROUND
		v.vol[3] *= 1.0f + x() / (float)SURROUND_AREA_SIZE;
#endif
	}

	if( y() >= 0 )
	{
		v.vol[0] *= 1.0f - y() / (float)SURROUND_AREA_SIZE;
		v.vol[1] *= 1.0f - y() / (float)SURROUND_AREA_SIZE;
	}
#ifndef LMMS_DISABLE_SURROUND
	else
	{
		v.vol[2] *= 1.0f + y() / (float)SURROUND_AREA_SIZE;
		v.vol[3] *= 1.0f + y() / (float)SURROUND_AREA_SIZE;
	}
#endif

	return( v );
}



void surroundAreaModel::saveSettings( QDomDocument & _doc,
							QDomElement & _this,
							const QString & _name )
{
	m_posX.saveSettings( _doc, _this, _name + "-x" );
	m_posY.saveSettings( _doc, _this, _name + "-y" );
}




void surroundAreaModel::loadSettings( const QDomElement & _this,
							const QString & _name )
{
	if( _this.hasAttribute( _name ) )
	{
		const int i = _this.attribute( _name ).toInt();
		m_posX.setValue( (float)( ( i & 0xFFFF ) - 2 * SURROUND_AREA_SIZE ) );
		m_posY.setValue( (float)( ( i >> 16 ) - 2 * SURROUND_AREA_SIZE ) );
	}
	else
	{
		m_posX.loadSettings( _this, _name + "-x" );
		m_posY.loadSettings( _this, _name + "-y" );
	}
}


#include "moc_surround_area.cxx"


