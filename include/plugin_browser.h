/*
 * plugin_browser.h - include file for pluginBrowser
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _PLUGIN_BROWSER_H
#define _PLUGIN_BROWSER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "qt3support.h"

#ifdef QT4

#include <QVector>
#include <QPixmap>

#else

#include <qvaluevector.h>
#include <qpixmap.h>

#endif


#include "side_bar_widget.h"
#include "plugin.h"


class trackContainer;


class pluginBrowser : public sideBarWidget, public engineObject
{
	Q_OBJECT
public:
	pluginBrowser( QWidget * _parent, engine * _engine );
	virtual ~pluginBrowser();


private:
	vvector<plugin::descriptor> m_pluginDescriptors;

	QWidget * m_view;

} ;




class pluginDescWidget : public QWidget, public engineObject
{
public:
	pluginDescWidget( const plugin::descriptor & _pd, QWidget * _parent,
							engine * _engine );
	virtual ~pluginDescWidget();


protected:
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );


private:
	const plugin::descriptor & m_pluginDescriptor;
	QPixmap m_logo;

	bool m_mouseOver;

} ;


#endif
