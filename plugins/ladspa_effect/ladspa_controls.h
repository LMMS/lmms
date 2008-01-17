/*
 * ladspa_controls.h - model for LADSPA-plugin controls
 *
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

#ifndef _LADSPA_CONTROLS_H
#define _LADSPA_CONTROLS_H

#include "effect_controls.h"
#include "ladspa_control.h"
#include "ladspa_control_dialog.h"


typedef QVector<ladspaControl *> control_list_t;

class ladspaEffect;


class ladspaControls : public effectControls
{
	Q_OBJECT
public:
	ladspaControls( ladspaEffect * _eff, track * _track );
	virtual ~ladspaControls();

	inline ch_cnt_t getControlCount( void )
	{
		return( m_controlCount );
	}

	virtual void FASTCALL saveSettings( QDomDocument & _doc, 
						QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "ladspacontrols" );
	}

	virtual effectControlDialog * createView( void )
	{
		return( new ladspaControlDialog( this ) );
	}


protected slots:
	void updateChannelLinkState( void );
	void linkPort( Uint16 _port, bool _state );


private:
	ladspaEffect * m_effect;
	ch_cnt_t m_processors;
	ch_cnt_t m_controlCount;
	track * m_track;
	bool m_noLink;
	boolModel m_stereoLinkModel;
	QVector<control_list_t> m_controls;


	friend class ladspaControlDialog;

} ;

#endif
