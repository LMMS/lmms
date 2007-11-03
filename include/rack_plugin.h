/*
 * rack_plugin.h - tab-widget in channel-track-window for setting up
 *                 effects
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
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
#ifndef _RACK_PLUGIN_H
#define _RACK_PLUGIN_H

#include <QtGui/QWidget>

#include "journalling_object.h"


class QGroupBox;
class QLabel;
class QPushButton;

class audioPort;
class effect;
class effectControlDialog;
class knob;
class ledCheckBox;
class tempoSyncKnob;
class track;


class rackPlugin: public QWidget, public journallingObject
{
	Q_OBJECT

public:
	rackPlugin( QWidget * _parent, effect * _eff, track * _track,
							audioPort * _port );
	virtual ~rackPlugin();
	
	inline effect * getEffect()
	{
		return( m_effect );
	}

	virtual void FASTCALL saveSettings( QDomDocument & _doc, 
						QDomElement & _parent );
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
	effectControlDialog * m_controlView;
	track * m_track;
	audioPort * m_port;
	bool m_show;

} ;

#endif
