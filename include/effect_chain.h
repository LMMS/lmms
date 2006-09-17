/*
 * effect_chain.h - class for processing and effects chain
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef _EFFECT_CHAIN_H
#define _EFFECT_CHAIN_H

#ifdef QT4

#include <QtCore/QMutex>

#else

#include <qmutex.h>

#endif

#include "qt3support.h"

#include "engine.h"
#include "effect.h"

typedef vvector<effect *> effect_list_t;

class effectChain: public engineObject
{
public:
	effectChain( engine * _engine );
	virtual ~effectChain();
	
	void FASTCALL appendEffect( effect * _effect );
	void FASTCALL deleteEffect( effect * _effect );
	void FASTCALL moveDown( effect * _effect );
	void FASTCALL moveUp( effect * _effect );
	bool FASTCALL processAudioBuffer( surroundSampleFrame * _buf, 
							const fpab_t _frames );
	void startRunning( void );
	bool isRunning( void );
	
	inline void setBypass( bool _mode )
	{
		m_bypassed = _mode;
	}
	
	inline const effect_list_t & getEffects( void )
	{
		return( m_effects );
	}
	
private:
	effect_list_t m_effects;
	
	bool m_bypassed;
	QMutex m_processLock;
};

#endif

