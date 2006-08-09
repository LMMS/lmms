/*
 * ladspa_control.h - widget for controlling a LADSPA port
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

#ifndef _LADSPA_CONTROL_H
#define _LADSPA_CONTROL_H

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#include <qwidget.h>

#include "journalling_object.h"
#include "instrument_track.h"
#include "knob.h"
#include "led_checkbox.h"

typedef struct portDescription port_desc_t;


class ladspaControl : public QWidget, public journallingObject
{
	Q_OBJECT
public:
	ladspaControl( QWidget * _parent, port_desc_t * _port, engine * _engine, instrumentTrack * _track );
	~ladspaControl();
	
	LADSPA_Data getValue( void );
	void FASTCALL setValue( LADSPA_Data _value );
	
	virtual void FASTCALL saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "port" );
	}

private:
	port_desc_t * m_port;
	ledCheckBox * m_toggle;
	knob * m_knob;
	
	QMutex m_processLock;
};

#endif

#endif
