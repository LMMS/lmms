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

#ifdef QT4

#include <QtGui/QWidget>
#include <QtGui/QGroupBox>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>

#else

#include <qwidget.h>
#include <qgroupbox.h>
#include <qpushbutton.h>
#include <qlabel.h>

#endif


#include "journalling_object.h"
#include "led_checkbox.h"
#include "track.h"
#include "effect.h"
#include "ladspa_control_dialog.h"
#include "audio_port.h"


class knob;
class tempoSyncKnob;


class rackPlugin: public QWidget, public journallingObject
{
	Q_OBJECT

public:
	rackPlugin( QWidget * _parent, ladspa_key_t _key, track * _track, engine * _engine, audioPort * _port );
	~rackPlugin();
	
	inline effect * getEffect()
	{
		return( m_effect );
	}
	
	inline const ladspa_key_t & getKey( void )
	{
		return( m_key );
	}
	
	virtual void FASTCALL saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "effect" );
	}

public slots:
	void editControls( void );
	void bypassed( bool _state );
	void setWetDry( float _value );
	void setAutoQuit( float _value );
	void setGate( float _value );
	void moveUp( void );
	void moveDown( void );
	void deletePlugin( void );
	void displayHelp( void );
	void closeEffects( void );
	
signals:
	void moveUp( rackPlugin * _plugin );
	void moveDown( rackPlugin * _plugin );
	void deletePlugin( rackPlugin * _plugin );

protected:
	void contextMenuEvent( QContextMenuEvent * _me );
	
private:
	ledCheckBox * m_bypass;
	knob * m_wetDry;
	tempoSyncKnob * m_autoQuit;
	knob * m_gate;
	QGroupBox * m_grouper;
	QGroupBox * m_controls;
	QLabel * m_label;
	QPushButton * m_editButton;
	effect * m_effect;
	ladspaControlDialog * m_controlView;
	track * m_track;
	audioPort * m_port;
	QMenu * m_contextMenu;
	ladspa_key_t m_key;
	QString m_name;
	bool m_show;
};

#endif

#endif
