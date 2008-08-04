/*
 * vst_effect_controls.cpp - controls for VST effect plugins
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "vst_effect_controls.h"
#include "vst_effect.h"



vstEffectControls::vstEffectControls( vstEffect * _eff ) :
	effectControls( _eff ),
	m_effect( _eff )
{
}




void vstEffectControls::loadSettings( const QDomElement & _this )
{
	m_effect->closePlugin();
	m_effect->openPlugin( _this.attribute( "plugin" ) );
	m_effect->m_pluginMutex.lock();
	if( m_effect->m_plugin != NULL )
	{
		m_effect->m_plugin->loadSettings( _this );
	}
	m_effect->m_pluginMutex.unlock();
}




void vstEffectControls::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "plugin", m_effect->m_key.attributes["file"] );
	m_effect->m_pluginMutex.lock();
	if( m_effect->m_plugin != NULL )
	{
		m_effect->m_plugin->saveSettings( _doc, _this );
	}
	m_effect->m_pluginMutex.unlock();
}



#include "moc_vst_effect_controls.cxx"

