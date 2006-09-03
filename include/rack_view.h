/*
 * right_frame.h - provides the display for the rackInsert instances
 *
 * Copyright (c) 2006 Danny McRae <khjklujn@netscape.net>
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
#ifndef _RIGHT_FRAME_H
#define _RIGHT_FRAME_H

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#ifdef QT4

#include <QtGui/QWidget>
#include <QtGui/QLayout>
#include <QtGui/QScrollArea>
#include <QtGui/QVBoxLayout>

#else

#include <qwidget.h>
#include <qlayout.h>
#include <qscrollview.h>
#include <qvbox.h>

#endif


#include "types.h"
#include "journalling_object.h"
#include "rack_plugin.h"
#include "track.h"
#include "ladspa_2_lmms.h"
#include "audio_port.h"


class rackView: public QWidget, public journallingObject
{
	Q_OBJECT

public:
	rackView( QWidget * _parent, engine * _engine, 
					track * _track, audioPort * _port );
	~rackView();

	void addPlugin( ladspa_key_t _key );
	
	virtual void FASTCALL saveSettings( QDomDocument & _doc, 
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "rack" );
	}

public slots:
	void moveUp( rackPlugin * _plugin );
	void moveDown( rackPlugin * _plugin );
	void deletePlugin( rackPlugin * _plugin );
	
private:
	void redraw();
	
	vvector<rackPlugin *> m_rackInserts;
		
	QVBoxLayout * m_mainLayout;
	QScrollArea * m_scrollArea;
	
	track * m_track;
	audioPort * m_port;
	
	Uint32 m_lastY;
	
	ladspa2LMMS * m_ladspa;
};

#endif

#endif
