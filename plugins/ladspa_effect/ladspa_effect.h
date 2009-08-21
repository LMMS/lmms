/*
 * ladspa_effect.h - class for handling LADSPA effect plugins
 *
 * Copyright (c) 2006-2009 Danny McRae <khjklujn/at/users.sourceforge.net>
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

#include <QtCore/QMutex>

#include "effect.h"
#include "ladspa_base.h"
#include "ladspa_controls.h"


typedef QVector<port_desc_t *> multi_proc_t;

class ladspaEffect : public effect
{
	Q_OBJECT
public:
	ladspaEffect( model * _parent,
			const descriptor::subPluginFeatures::key * _key );
	virtual ~ladspaEffect();

	virtual bool processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames );
	
	void setControl( int _control, LADSPA_Data _data );

	virtual effectControls * getControls( void )
	{
		return( m_controls );
	}

	inline const multi_proc_t & getPortControls( void )
	{
		return( m_portControls );
	}


private slots:
	void changeSampleRate( void );


private:
	void pluginInstantiation( void );
	void pluginDestruction( void );

	static sample_rate_t maxSamplerate( const QString & _name );


	QMutex m_pluginMutex;
	ladspaControls * m_controls;

	sample_rate_t m_maxSampleRate;
	ladspa_key_t m_key;
	int m_portCount;

	const LADSPA_Descriptor * m_descriptor;
	QVector<LADSPA_Handle> m_handles;

	QVector<multi_proc_t> m_ports;
	multi_proc_t m_portControls;
} ;

#endif
