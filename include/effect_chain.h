/*
 * effect_chain.h - class for processing and effects chain
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

#ifndef _EFFECT_CHAIN_H
#define _EFFECT_CHAIN_H

#include "effect.h"


class effectChain : public journallingObject, public model
{
public:
	effectChain( audioPort * _port, track * _track );
	virtual ~effectChain();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName( void ) const
	{
		return( "fxchain" );
	}

	void appendEffect( effect * _effect );
	void removeEffect( effect * _effect );
	void moveDown( effect * _effect );
	void moveUp( effect * _effect );
	bool processAudioBuffer( surroundSampleFrame * _buf, 
							const fpp_t _frames );
	void startRunning( void );
	bool isRunning( void );

/*	inline const effect_list_t & getEffects( void )
	{
		return( m_effects );
	}*/

	void deleteAllPlugins( void );


private:
	typedef QVector<effect *> effectList;
	effectList m_effects;

	audioPort * m_port;
	track * m_track;

	boolModel m_enabledModel;


	friend class effectRackView;

} ;

#endif

