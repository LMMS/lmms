/*
 * lv2_port_dialog.h - dialog to test an LV2 plugin
 *
 * Copyright (c) 2009 Martin Andrews <mdda/at/users.sourceforge.net>
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


#ifndef _LV2_PORT_DIALOG_H
#define _LV2_PORT_DIALOG_H


#include <QtGui/QDialog>

#include "lv2_manager.h"




class lv2PortDialog : public QDialog
{
	Q_OBJECT
public:
	lv2PortDialog( const lv2_key_t & _key );
	virtual ~lv2PortDialog();

};




#endif
