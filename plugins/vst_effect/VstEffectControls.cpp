/*
 * VstEffectControls.cpp - controls for VST effect plugins
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "VstEffectControls.h"
#include "VstEffect.h"



VstEffectControls::VstEffectControls( VstEffect * _eff ) :
	EffectControls( _eff ),
	m_effect( _eff )
{
}




void VstEffectControls::loadSettings( const QDomElement & _this )
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




void VstEffectControls::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "plugin", m_effect->m_key.attributes["file"] );
	m_effect->m_pluginMutex.lock();
	if( m_effect->m_plugin != NULL )
	{
		m_effect->m_plugin->saveSettings( _doc, _this );
	}
	m_effect->m_pluginMutex.unlock();
}




int VstEffectControls::controlCount()
{
	return m_effect->m_plugin != NULL &&
		m_effect->m_plugin->hasEditor() ?  1 : 0;
}



#include "moc_VstEffectControls.cxx"

