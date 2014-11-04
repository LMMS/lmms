/*
 * MeterModel.cpp - model for meter specification
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "MeterModel.h"
#include "AutomationPattern.h"


MeterModel::MeterModel( ::Model * _parent ) :
	Model( _parent ),
	m_numeratorModel( 4, 1, 32, this, tr( "Numerator" ) ),
	m_denominatorModel( 4, 1, 32, this, tr( "Denominator" ) )
{
	connect( &m_numeratorModel, SIGNAL( dataChanged() ), 
				this, SIGNAL( dataChanged() ) );
	connect( &m_denominatorModel, SIGNAL( dataChanged() ), 
				this, SIGNAL( dataChanged() ) );
}




MeterModel::~MeterModel()
{
}




void MeterModel::reset()
{
	m_numeratorModel.setValue( 4 );
	m_denominatorModel.setValue( 4 );

	AutomationPattern::globalAutomationPattern( &m_numeratorModel )->clear();
	AutomationPattern::globalAutomationPattern( &m_denominatorModel )->clear();
}




void MeterModel::saveSettings( QDomDocument & _doc, QDomElement & _this,
								const QString & _name )
{
	m_numeratorModel.saveSettings( _doc, _this, _name + "_numerator" );
	m_denominatorModel.saveSettings( _doc, _this, _name + "_denominator" );
}




void MeterModel::loadSettings( const QDomElement & _this,
								const QString & _name )
{
	m_numeratorModel.loadSettings( _this, _name + "_numerator" );
	m_denominatorModel.loadSettings( _this, _name + "_denominator" );
}



#include "moc_MeterModel.cxx"

