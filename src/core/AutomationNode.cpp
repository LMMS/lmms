/*
 * AutomationPattern.cpp - Implementation of class AutomationNode which
 *                         holds information on a single automation pattern node
 *
 * Copyright (c) 2020 Ian Caio <iancaio_dev/at/hotmail.com>
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

#include "AutomationNode.h"


// Dummy constructor for the QMap
AutomationNode::AutomationNode() :
	m_pattern( nullptr ),
	m_key( 0 ),
	m_inValue( 0 ),
	m_outValue( 0 ),
	m_inTangent( 0 ),
	m_outTangent( 0 )
{
}

AutomationNode::AutomationNode( AutomationPattern * pat, float value, int key ) :
	m_pattern( pat ),
	m_key( key ),
	m_inValue( value ),
	m_outValue( value ),
	m_inTangent( 0 ),
	m_outTangent( 0 )
{
}

AutomationNode::AutomationNode( AutomationPattern * pat, float inValue, float outValue, int key ) :
	m_pattern( pat ),
	m_key( key ),
	m_inValue( inValue ),
	m_outValue( outValue ),
	m_inTangent( 0 ),
	m_outTangent( 0 )
{
}
