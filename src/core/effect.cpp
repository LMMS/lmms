#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect.cpp - base-class for effects
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "effect.h"
#include "engine.h"
#include "dummy_effect.h"
#include "effect_chain.h"
#include "effect_view.h"


effect::effect( const plugin::descriptor * _desc,
			model * _parent,
			const descriptor::subPluginFeatures::key * _key ) :
	plugin( _desc, _parent ),
	m_key( _key ? *_key : descriptor::subPluginFeatures::key()  ),
	m_okay( TRUE ),
	m_noRun( FALSE ),
	m_running( FALSE ),
	m_bufferCount( 0 ),
	m_enabledModel( TRUE, FALSE, TRUE, boolModel::defaultRelStep(), this ),
	m_wetDryModel( 1.0f, 0.0f, 1.0f, 0.01f, this ),
	m_gateModel( 0.0f, 0.0f, 1.0f, 0.01f, this ),
	m_autoQuitModel( 1.0f, 1.0f, 8000.0f, 100.0f, this )
{
}




effect::~effect()
{
}




void effect::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "on", m_enabledModel.value() );
	_this.setAttribute( "wet", m_wetDryModel.value() );
	_this.setAttribute( "autoquit", m_autoQuitModel.value() );
	_this.setAttribute( "gate", m_gateModel.value() );
//	m_controlView->saveState( _doc, _this );
}




void effect::loadSettings( const QDomElement & _this )
{
	m_enabledModel.setValue( _this.attribute( "on" ).toInt() );
	m_wetDryModel.setValue( _this.attribute( "wet" ).toFloat() );
	m_autoQuitModel.setValue( _this.attribute( "autoquit" ).toFloat() );
	m_gateModel.setValue( _this.attribute( "gate" ).toFloat() );
/*
	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( m_controlView->nodeName() == node.nodeName() )
			{
				m_controlView->restoreState( 
							node.toElement() );
			}
		}
		node = node.nextSibling();
	}*/
}





bool effect::processAudioBuffer( surroundSampleFrame * _buf, 
							const fpp_t _frames )
{
	return( FALSE );
}




effect * effect::instantiate( const QString & _plugin_name,
				model * _parent,
				descriptor::subPluginFeatures::key * _key )
{
	plugin * p = plugin::instantiate( _plugin_name, _parent, _key );
	// check whether instantiated plugin is an instrument
	if( dynamic_cast<effect *>( p ) != NULL )
	{
		// everything ok, so return pointer
		return( dynamic_cast<effect *>( p ) );
	}

	// not quite... so delete plugin and return dummy instrument
	delete p;
	return( new dummyEffect( _parent ) );
}




pluginView * effect::instantiateView( QWidget * _parent )
{
	return( new effectView( this, _parent ) );
}


#endif
