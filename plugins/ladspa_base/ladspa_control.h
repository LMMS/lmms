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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef _LADSPA_CONTROL_H
#define _LADSPA_CONTROL_H

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QWidget>
#include <QtGui/QLayout>
#include <QtCore/QMutex>

#else

#include <qwidget.h>
#include <qmutex.h>
#include <qlayout.h>

#endif


#include "journalling_object.h"
#include "track.h"
#include "knob.h"
#include "led_checkbox.h"
#include "ladspa_manager.h"


typedef struct portDescription port_desc_t;


class ladspaControl : public QWidget, public journallingObject
{
	Q_OBJECT
public:
	ladspaControl( QWidget * _parent, port_desc_t * _port, 
					engine * _engine, track * _track,
					bool _link = FALSE );
	~ladspaControl();
	
	LADSPA_Data getValue( void );
	void FASTCALL setValue( LADSPA_Data _value );
	void FASTCALL setLink( bool _state );
	
	void FASTCALL linkControls( ladspaControl * _control );
	void FASTCALL unlinkControls( ladspaControl * _control );

	inline ledCheckBox * getToggle( void )
	{
		return( m_toggle );
	}
	
	inline knob * getKnob( void )
	{
		return( m_knob );
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
	void ledChange( bool _state );
	void knobChange( float _value );
	void portLink( bool _state );
	
private:
	port_desc_t * m_port;
	track * m_track;
	QHBoxLayout * m_layout;
	ledCheckBox * m_link;
	ledCheckBox * m_toggle;
	knob * m_knob;
	
	QMutex m_processLock;
};

#endif
