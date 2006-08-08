/*
 * ladspa_browser.h - dialog to display information about installed LADSPA
 *                    plugins
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


#ifndef _LADSPA_BROWSER_H
#define _LADSPA_BROWSER_H

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QDialog>

#else

#include <qdialog.h>

#endif

#include "engine.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QSlider;

class tabBar;


class ladspaBrowser : public QDialog, public engineObject
{
	Q_OBJECT
public:
	ladspaBrowser( engine * _engine );
	virtual ~ladspaBrowser();

	inline void labelWidget( QWidget * _w, const QString & _txt );

public slots:
	void showPorts( const ladspa_key_t & _key );
	void testLADSPA( const ladspa_key_t & _key );
	
private:
	tabBar * m_tabBar;

} ;

#endif

#endif
