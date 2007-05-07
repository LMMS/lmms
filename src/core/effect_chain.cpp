#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect_chain.cpp - class for processing and effects chain
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
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


#include "effect_chain.h"
#include "engine.h"




effectChain::effectChain( void ) :
	m_bypassed( TRUE )
{
}




effectChain::~effectChain()
{
	for( effect_list_t::size_type eff = 0; eff < m_effects.count(); eff++ )
	{
		delete m_effects[eff];
	}
	m_effects.clear();
}




void FASTCALL effectChain::appendEffect( effect * _effect )
{
	engine::getMixer()->lock();
	m_effects.append( _effect );
	engine::getMixer()->unlock();
}



void FASTCALL effectChain::removeEffect( effect * _effect )
{
	engine::getMixer()->lock();
	m_effects.erase( qFind( m_effects.begin(), m_effects.end(), _effect ) );
	engine::getMixer()->unlock();
}




void FASTCALL effectChain::moveDown( effect * _effect )
{
	if( _effect != m_effects.last() )
	{
		int i = 0;
		for( effect_list_t::iterator it = m_effects.begin(); 
					it != m_effects.end(); it++, i++ )
		{
			if( (*it) == _effect )
			{
				break;
			}
		}
		
		effect * temp = m_effects[i + 1];
		m_effects[i + 1] = _effect;
		m_effects[i] = temp;	
	}
}




void FASTCALL effectChain::moveUp( effect * _effect )
{
	if( _effect != m_effects.first() )
	{
		int i = 0;
		for( effect_list_t::iterator it = m_effects.begin(); 
					it != m_effects.end(); it++, i++ )
		{
			if( (*it) == _effect )
			{
				break;
			}
		}
		
		effect * temp = m_effects[i - 1];
		m_effects[i - 1] = _effect;
		m_effects[i] = temp;	
	}
}




bool FASTCALL effectChain::processAudioBuffer( surroundSampleFrame * _buf, 
							const fpab_t _frames )
{
	if( m_bypassed )
	{
		return( FALSE );
	}
	bool more_effects = FALSE;
	for( effect_list_t::iterator it = m_effects.begin(); 
						it != m_effects.end(); it++ )
	{
		more_effects |= (*it)->processAudioBuffer( _buf, _frames );
	}
	return( more_effects );
}




void effectChain::startRunning( void )
{
	if( m_bypassed )
	{
		return;
	}
	
	for( effect_list_t::iterator it = m_effects.begin(); 
						it != m_effects.end(); it++ )
	{
		(*it)->startRunning();
	}
}




bool effectChain::isRunning( void )
{
	if( m_bypassed )
	{
		return( FALSE );
	}
	
	bool running = FALSE;
	
	for( effect_list_t::iterator it = m_effects.begin(); 
				it != m_effects.end() || !running; it++ )
	{
		running = (*it)->isRunning() && running;
	}
	return( running );
}


#endif
