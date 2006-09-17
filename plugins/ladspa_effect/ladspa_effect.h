/*
 * ladspa_effect.h - class for handling LADSPA effect plugins
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

#ifndef _LADSPA_EFFECT_H
#define _LADSPA_EFFECT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#include "effect.h"
#include "ladspa_2_lmms.h"
#include "mixer.h"
#include "ladspa_control.h"
#include "ladspa_control_dialog.h"
#include "ladspa_base.h"
#include "main_window.h"


typedef vvector<port_desc_t *> multi_proc_t;

class ladspaEffect : public effect
{
public:
	ladspaEffect( effect::constructionData * _cdata );
	virtual ~ladspaEffect();

	virtual bool FASTCALL processAudioBuffer( surroundSampleFrame * _buf,
							const fpab_t _frames );
	
	void FASTCALL setControl( Uint16 _control, LADSPA_Data _data );

	inline const multi_proc_t & getControls( void )
	{
		return( m_controls );
	}

	virtual inline QString publicName( void ) const
	{
		return( m_effName );
	}
	
	inline void setPublicName( const QString & _name )
	{
		m_effName = _name;
	}

	virtual inline effectControlDialog * createControlDialog(
								track * _track )
	{
		return( new ladspaControlDialog(
					eng()->getMainWindow()->workspace(),
							this, _track ) );
	}

	inline virtual QString nodeName( void ) const
	{
		return( "ladspaeffect" );
	}


private:
	QString m_effName;
	ladspa_key_t m_key;
	ladspa2LMMS * m_ladspa;
	Uint16 m_effectChannels;
	Uint16 m_portCount;
	fpab_t m_bufferSize;
			
	const LADSPA_Descriptor * m_descriptor;
	vvector<LADSPA_Handle> m_handles;

	vvector<multi_proc_t> m_ports;
	multi_proc_t m_controls;
} ;

#endif
