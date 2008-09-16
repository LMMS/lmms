/*
 * effect_chain.cpp - class for processing and effects chain
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
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

#include "effect_chain.h"
#include "effect.h"
#include "engine.h"
#include "debug.h"
#include "dummy_effect.h"



effectChain::effectChain( model * _parent ) :
	model( _parent ),
	serializingObject(),
	m_enabledModel( FALSE, NULL, tr( "Effects enabled" ) )
{
}




effectChain::~effectChain()
{
	clear();
}




void effectChain::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "enabled", m_enabledModel.value() );
	_this.setAttribute( "numofeffects", m_effects.count() );
	for( effectList::iterator it = m_effects.begin(); 
					it != m_effects.end(); it++ )
	{
		QDomElement ef = ( *it )->saveState( _doc, _this );
		ef.setAttribute( "name", ( *it )->getDescriptor()->name );
		ef.appendChild( ( *it )->getKey().saveXML( _doc ) );
	}
}




void effectChain::loadSettings( const QDomElement & _this )
{
	clear();

	m_enabledModel.setValue( _this.attribute( "enabled" ).toInt() );

	const int plugin_cnt = _this.attribute( "numofeffects" ).toInt();

	QDomNode node = _this.firstChild();
	int fx_loaded = 0;
	while( !node.isNull() && fx_loaded < plugin_cnt )
	{
		if( node.isElement() && node.nodeName() == "effect" )
		{
			QDomElement cn = node.toElement();
			const QString name = cn.attribute( "name" );
			effectKey key( cn.elementsByTagName( "key" ).
							item( 0 ).toElement() );
			effect * e = effect::instantiate( name, this, &key );
			if( e->isOkay() )
			{
				if( node.isElement() )
				{
					if( e->nodeName() == node.nodeName() )
					{
				e->restoreState( node.toElement() );
					}
				}
			}
			else
			{
				delete e;
				e = new dummyEffect( parentModel() );
			}
			m_effects.push_back( e );
			++fx_loaded;
		}
		node = node.nextSibling();
	}

	emit dataChanged();
}




void effectChain::appendEffect( effect * _effect )
{
	engine::getMixer()->lock();
	m_effects.append( _effect );
	engine::getMixer()->unlock();

	emit dataChanged();
}




void effectChain::removeEffect( effect * _effect )
{
	engine::getMixer()->lock();
	m_effects.erase( qFind( m_effects.begin(), m_effects.end(), _effect ) );
	engine::getMixer()->unlock();
}




void effectChain::moveDown( effect * _effect )
{
	if( _effect != m_effects.last() )
	{
		int i = 0;
		for( effectList::iterator it = m_effects.begin(); 
					it != m_effects.end(); it++, i++ )
		{
			if( *it == _effect )
			{
				break;
			}
		}
		
		effect * temp = m_effects[i + 1];
		m_effects[i + 1] = _effect;
		m_effects[i] = temp;	
	}
}




void effectChain::moveUp( effect * _effect )
{
	if( _effect != m_effects.first() )
	{
		int i = 0;
		for( effectList::iterator it = m_effects.begin(); 
					it != m_effects.end(); it++, i++ )
		{
			if( *it == _effect )
			{
				break;
			}
		}
		
		effect * temp = m_effects[i - 1];
		m_effects[i - 1] = _effect;
		m_effects[i] = temp;	
	}
}




bool effectChain::processAudioBuffer( sampleFrame * _buf, const fpp_t _frames )
{
	if( m_enabledModel.value() == FALSE )
	{
		return( FALSE );
	}
	bool more_effects = FALSE;
	for( effectList::iterator it = m_effects.begin(); 
						it != m_effects.end(); ++it )
	{
		more_effects |= ( *it )->processAudioBuffer( _buf, _frames );
#ifdef LMMS_DEBUG
		for( int f = 0; f < _frames; ++f )
		{
			if( fabs( _buf[f][0] ) > 5 || fabs( _buf[f][1] ) > 5 )
			{
				it = m_effects.end()-1;
				printf( "numerical overflow after processing "
					"plugin \"%s\"\n", ( *it )->
					publicName().toAscii().constData() );
				break;
			}
		}
#endif
	}
	return( more_effects );
}




void effectChain::startRunning( void )
{
	if( m_enabledModel.value() == FALSE )
	{
		return;
	}
	
	for( effectList::iterator it = m_effects.begin(); 
						it != m_effects.end(); it++ )
	{
		( *it )->startRunning();
	}
}




bool effectChain::isRunning( void )
{
	if( m_enabledModel.value() == FALSE )
	{
		return( FALSE );
	}
	
	bool running = FALSE;
	
	for( effectList::iterator it = m_effects.begin(); 
				it != m_effects.end() || !running; it++ )
	{
		running = ( *it )->isRunning() && running;
	}
	return( running );
}




void effectChain::clear( void )
{
	emit aboutToClear();

	m_enabledModel.setValue( false );
	for( int i = 0; i < m_effects.count(); ++i )
	{
		delete m_effects[i];
	}
	m_effects.clear();
}



#include "moc_effect_chain.cxx"

