/*
 * ladspa_control_dialog.h - dialog for displaying and editing control port
 *                           values for LADSPA plugins
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

#ifndef _LADSPA_CONTROL_DIALOG_H
#define _LADSPA_CONTROL_DIALOG_H

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QGroupBox>
#include <QtGui/QLayout>

#else

#include <qgroupbox.h>
#include <qlayout.h>

#endif

#include "effect_control_dialog.h"
#include "ladspa_control.h"
#include "track.h"
#include "led_checkbox.h"


typedef vvector<ladspaControl *> control_list_t;

class ladspaEffect;


class ladspaControlDialog : public effectControlDialog
{
	Q_OBJECT
public:
	ladspaControlDialog( QWidget * _parent, ladspaEffect * _eff, 
							track * _track );
	virtual ~ladspaControlDialog();

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


protected slots:
	void link( bool _state );
	void linkPort( Uint16 _port, bool _state );


private:
	ladspaEffect * m_effect;
	ch_cnt_t m_processors;
	ch_cnt_t m_controlCount;
	track * m_track;
	bool m_noLink;
	audioPort * m_port;
	ledCheckBox * m_stereoLink;
	vvector<QWidget *> m_blanks;
	vvector<control_list_t> m_controls;
	
	QVBoxLayout * m_mainLay;
	QHBoxLayout * m_effectLay;

} ;

#endif
