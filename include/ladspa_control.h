/*
 * ladspa_control.h - model for controlling a LADSPA port
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

#ifndef _LADSPA_CONTROL_H
#define _LADSPA_CONTROL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_LADSPA_H
#include <ladspa.h>
#else
#include "ladspa-1.1.h"
#endif


#include "automatable_model.h"
#include "knob.h"
#include "tempo_sync_knob.h"


class track;


typedef struct portDescription port_desc_t;


class ladspaControl : public model, public journallingObject
{
	Q_OBJECT
public:
	ladspaControl( model * _parent, port_desc_t * _port, track * _track,
							bool _link = FALSE );
	~ladspaControl();

	LADSPA_Data getValue( void );
	void FASTCALL setValue( LADSPA_Data _value );
	void FASTCALL setLink( bool _state );

	void FASTCALL linkControls( ladspaControl * _control );
	void FASTCALL unlinkControls( ladspaControl * _control );

	inline boolModel * getToggledModel( void )
	{
		return( &m_toggledModel );
	}

	inline knobModel * getKnobModel( void )
	{
		return( &m_knobModel );
	}

	inline knobModel * getTempoSyncKnobModel( void )
	{
		return( &m_knobModel );
	}

	inline port_desc_t * getPort( void )
	{
		return( m_port );
	}

	virtual void FASTCALL saveSettings( QDomDocument & _doc, 
				QDomElement & _parent, const QString & _name );
	virtual void FASTCALL loadSettings( const QDomElement & _this, 
						const QString & _name );
	inline virtual QString nodeName( void ) const
	{
		return( "port" );
	}


signals:
	void changed( Uint16 _port, LADSPA_Data _value );
	void linkChanged( Uint16 _port, bool _state );


protected slots:
	void ledChanged( void );
	void knobChanged( void );
	void tempoKnobChanged( void );
	void linkStateChanged( void );


private:
	bool m_link;
	port_desc_t * m_port;

	boolModel m_linkEnabledModel;
	boolModel m_toggledModel;
	knobModel m_knobModel;
	tempoSyncKnobModel m_tempoSyncKnobModel;


	friend class ladspaControlView;

} ;

#endif
