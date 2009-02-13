/*
 * plugin_browser.h - include file for pluginBrowser
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _PLUGIN_BROWSER_H
#define _PLUGIN_BROWSER_H

#include <QtCore/QTimer>
#include <QtCore/QVector>
#include <QtGui/QWidget>
#include <QtGui/QPixmap>


#include "plugin.h"


class trackContainer;


class pluginBrowser : public QWidget
{
	Q_OBJECT
public:
	pluginBrowser( QWidget * _parent = NULL );
	virtual ~pluginBrowser();


private:
	QVector<plugin::descriptor> m_pluginDescriptors;

} ;




class pluginDescWidget : public QWidget
{
	Q_OBJECT
public:
	pluginDescWidget( const plugin::descriptor & _pd, QWidget * _parent );
	virtual ~pluginDescWidget();


protected:
	virtual void enterEvent( QEvent * _e );
	virtual void leaveEvent( QEvent * _e );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );


private slots:
	void updateHeight( void );


private:
	QTimer m_updateTimer;

	const plugin::descriptor & m_pluginDescriptor;
	QPixmap m_logo;

	bool m_mouseOver;
	int m_targetHeight;

} ;


#endif
