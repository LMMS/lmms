/*
 * ladspa_effect.h - class for handling LADSPA effect plugins
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
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

#ifndef _LADSPA_EFFECT_H
#define _LADSPA_EFFECT_H

#include <QtGui/QWorkspace>

#include "effect.h"
#include "engine.h"
#include "ladspa_base.h"
#include "ladspa_controls.h"
#include "main_window.h"
#include "mixer.h"


typedef QVector<port_desc_t *> multi_proc_t;

class ladspaEffect : public effect
{
public:
	ladspaEffect( model * _parent,
			const descriptor::subPluginFeatures::key * _key );
	virtual ~ladspaEffect();

	virtual bool processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames );
	
	void FASTCALL setControl( Uint16 _control, LADSPA_Data _data );

	virtual effectControls * getControls( void )
	{
		return( m_controls );
	}

	inline const multi_proc_t & getPortControls( void )
	{
		return( m_portControls );
	}

	virtual inline QString publicName( void ) const
	{
		return( m_effName );
	}
	
	inline void setPublicName( const QString & _name )
	{
		m_effName = _name;
	}


private:
	ladspaControls * m_controls;

	QString m_effName;
	ladspa_key_t m_key;
	Uint16 m_effectChannels;
	Uint16 m_portCount;
	fpp_t m_bufferSize;

	const LADSPA_Descriptor * m_descriptor;
	QVector<LADSPA_Handle> m_handles;

	QVector<multi_proc_t> m_ports;
	multi_proc_t m_portControls;
} ;

#endif
