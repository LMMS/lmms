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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */
#ifndef _RIGHT_FRAME_H
#define _RIGHT_FRAME_H

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#include <qwidget.h>
#include <qlayout.h>
#include <qscrollview.h>
#include <qvbox.h>
#include <qptrlist.h>

#include "types.h"
#include "engine.h"
#include "rack_plugin.h"
#include "instrument_track.h"


class rackView: public QWidget, public engineObject
{
	Q_OBJECT

public:
	rackView( QWidget * _parent, engine * _engine, instrumentTrack * _track );
	~rackView();

	void addPlugin( ladspa_key_t _key );

private:
	QPtrList<rackPlugin> m_rackInserts;
	int m_insertIndex;
		
	QVBoxLayout * m_mainLayout;
	QScrollView * m_scrollView;
	QVBox * m_rack;
	
	instrumentTrack * m_instrumentTrack;
};

#endif

#endif
