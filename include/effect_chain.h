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

#include "mv_base.h"
#include "serializing_object.h"
#include "mixer.h"
#include "automatable_model.h"

class effect;


class effectChain : public model, public serializingObject
{
	Q_OBJECT
public:
	effectChain( model * _parent );
	virtual ~effectChain();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName( void ) const
	{
		return "fxchain";
	}

	void appendEffect( effect * _effect );
	void removeEffect( effect * _effect );
	void moveDown( effect * _effect );
	void moveUp( effect * _effect );
	bool processAudioBuffer( sampleFrame * _buf, const fpp_t _frames );
	void startRunning( void );
	bool isRunning( void );

	void clear( void );

	void setEnabled( bool _on )
	{
		m_enabledModel.setValue( _on );
	}


private:
	typedef QVector<effect *> effectList;
	effectList m_effects;

	boolModel m_enabledModel;


	friend class effectRackView;


signals:
	void aboutToClear( void );

} ;

#endif

