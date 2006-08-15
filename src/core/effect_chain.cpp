#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect_chain.cpp - class for processing and effects chain
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#include "effect_chain.h"

effectChain::effectChain( engine * _engine ):
	engineObject( _engine ),
	m_bypassed( TRUE )
{
}




effectChain::~ effectChain()
{
	m_processLock.lock();
	for( Uint32 eff = 0; eff < m_effects.count(); eff++ )
	{
		free( m_effects[eff] );
	}
	m_effects.clear();
	m_processLock.unlock();
}




void FASTCALL effectChain::appendEffect( effect * _effect )
{
	m_processLock.lock();
	m_effects.append( _effect );
	m_processLock.unlock();
}



void FASTCALL effectChain::deleteEffect( effect * _effect )
{
	m_processLock.lock();
	effect_list_t::iterator which = NULL;
	for( effect_list_t::iterator it = m_effects.begin(); 
						it != m_effects.end(); it++ )
	{
		if( (*it) == _effect )
		{
			which = it;
			break;
		}
	}
	
	if( which != NULL )
	{
		m_effects.erase( which );
	}
	
	m_processLock.unlock();
}




void FASTCALL effectChain::moveDown( effect * _effect )
{
	m_processLock.lock();
	
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
	
	m_processLock.unlock();
}




void FASTCALL effectChain::moveUp( effect * _effect )
{
	m_processLock.lock();
	
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
	
	m_processLock.unlock();
}




bool FASTCALL effectChain::processAudioBuffer( surroundSampleFrame * _buf, 
							const fpab_t _frames )
{
	if( m_bypassed || ! m_processLock.tryLock() )
	{
		return( FALSE );
	}
	bool more_effects = FALSE;
	for( effect_list_t::iterator it = m_effects.begin(); 
						it != m_effects.end(); it++ )
	{
		more_effects |= (*it)->processAudioBuffer( _buf, _frames );
	}
	m_processLock.unlock();
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

#endif
