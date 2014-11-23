/*
 * LadspaControlView.h - widget for controlling a LADSPA port
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - http://lmms.io
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

#ifndef _LADSPA_CONTROL_VIEW_H
#define _LADSPA_CONTROL_VIEW_H

#include <QtGui/QWidget>

#include "ModelView.h"

class LadspaControl;


class EXPORT LadspaControlView : public QWidget, public ModelView
{
	Q_OBJECT
public:
	LadspaControlView( QWidget * _parent, LadspaControl * _ctl );
	virtual ~LadspaControlView();
	
private:
	LadspaControl * m_ctl;

} ;

#endif
