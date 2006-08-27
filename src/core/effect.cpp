#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect.cpp - class for processing LADSPA effects
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

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#ifdef QT4

#include <QtGui/QMessageBox>

#else

#include "qmessagebox.h"

#endif


#include "effect.h"


effect::effect( engine * _engine ) :
	engineObject( _engine ),
	m_okay( TRUE ),
	m_noRun( FALSE ),
	m_running( FALSE ),
	m_bypass( FALSE ),
	m_bufferCount( 0 ),
	m_silenceTimeout( 10 ),
	m_wetDry( 1.0f ),
	m_gate( 0.0f )
{
}




effect::~effect()
{
}




bool FASTCALL effect::processAudioBuffer( surroundSampleFrame * _buf, 
							const fpab_t _frames )
{
	return( FALSE );
}




void FASTCALL effect::setGate( float _level )
{
	m_processLock.lock();
	m_gate = _level * _level * m_processors * 
				eng()->getMixer()->framesPerAudioBuffer();
	m_processLock.unlock();
}




#endif

#endif
