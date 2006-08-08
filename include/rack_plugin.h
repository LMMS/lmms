/*
 * effect_tab_widget.h - tab-widget in channel-track-window for setting up
 *                       effects
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
#ifndef _RACK_PLUGIN_H
#define _RACK_PLUGIN_H

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#include <qwidget.h>
#include <qgroupbox.h>
#include <qpushbutton.h>
#include <qlabel.h>

#include "journalling_object.h"
#include "led_checkbox.h"
#include "instrument_track.h"
#include "effect.h"
#include "ladspa_control_dialog.h"


class knob;


class rackPlugin: public QWidget, public journallingObject
{
	Q_OBJECT

public:
	rackPlugin( QWidget * _parent, ladspa_key_t _key, instrumentTrack * _track, engine * _engine );
	~rackPlugin();
	
	QString nodeName( void ) const
	{
		return( "plugin" );
	}
	
public slots:
	void editControls( void );
	void bypassed( bool _state );
	void setWetDry( float _value );
	void setAutoQuit( float _value );
	void setGate( float _value );
	
private:
	ledCheckBox * m_bypass;
	knob * m_wetDry;
	knob * m_autoQuit;
	knob * m_gate;
	QGroupBox * m_grouper;
	QGroupBox * m_controls;
	QLabel * m_label;
	QPushButton * m_editButton;
	effect * m_effect;
	ladspaControlDialog * m_controlView;
};

#endif

#endif
