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
	for( Uint32 eff = 0; eff < m_effects.count(); eff++ )
	{
		free( m_effects[eff] );
	}
	m_effects.clear();
}




void FASTCALL effectChain::appendEffect( effect * _effect )
{
	m_effects.append( _effect );
}




bool FASTCALL effectChain::processAudioBuffer( surroundSampleFrame * _buf, const fpab_t _frames )
{
	if( m_bypassed )
	{
		return( FALSE );
	}
	
	bool more_effects = FALSE;
	for( effect_list_t::iterator it = m_effects.begin(); it != m_effects.end(); it++ )
	{
		more_effects |= (*it)->processAudioBuffer( _buf, _frames );
	}
	return( more_effects );
}




void effectChain::setRunning( void )
{
	if( m_bypassed )
	{
		return;
	}
	
	for( effect_list_t::iterator it = m_effects.begin(); it != m_effects.end(); it++ )
	{
		(*it)->setRunning();
	}
}




bool effectChain::isRunning( void )
{
	if( m_bypassed )
	{
		return( FALSE );
	}
	
	bool running = FALSE;
	
	for( effect_list_t::iterator it = m_effects.begin(); it != m_effects.end() || !running; it++ )
	{
		running = (*it)->isRunning() && running;
	}
	return( running );
}
	



void FASTCALL effectChain::swapEffects( effect * _eff1, effect * _eff2 )
{
	Uint32 eff1_loc = m_effects.count();
	Uint32 eff2_loc = m_effects.count();
	
	Uint32 count = 0;
	for( effect_list_t::iterator it = m_effects.begin(); it != m_effects.end(); it++ )
	{
		if( (*it) == _eff1 )
		{
			eff1_loc = count;
		}
		if( (*it) == _eff2 )
		{
			eff2_loc = count;
		}
		count++;
	}
	
	if( eff1_loc < m_effects.count() && eff2_loc < m_effects.count() )
	{
		m_effects[eff1_loc] = _eff2;
		m_effects[eff2_loc] = _eff1;
	}
}

#endif

#endif
