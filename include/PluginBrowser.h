/*
 * PluginBrowser.h - include file for pluginBrowser
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#ifndef PLUGIN_BROWSER_H
#define PLUGIN_BROWSER_H

#include <QtCore/QTimer>
#include <QPixmap>
#include <QtGui/QLabel>
#include <memory>

#include "SideBarWidget.h"
#include "Plugin.h"


class PluginBrowser : public SideBarWidget
{
	Q_OBJECT
public:
	PluginBrowser( QWidget * _parent );
	virtual ~PluginBrowser() = default;

private slots:
	void highlightPlugin(Plugin::Descriptor const& pd);
	void highlightNone();


private:
	QWidget * m_view;

	std::unique_ptr<QLabel> m_hint;

};




class PluginDescList : public QWidget
{
	Q_OBJECT
public:
	PluginDescList(QWidget* parent);

signals:
	void highlight(Plugin::Descriptor const&);
	void unhighlight();

private slots:
	void receiveHighlight(Plugin::Descriptor const&);
	void receiveUnhighlight();
};




class PluginDescWidget : public QWidget
{
	Q_OBJECT
public:
	PluginDescWidget( const Plugin::Descriptor & _pd, QWidget * _parent );


protected:
	void enterEvent( QEvent * _e ) override;
	void leaveEvent( QEvent * _e ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void paintEvent( QPaintEvent * _pe ) override;

	signals:
	void highlight(Plugin::Descriptor const& pd);
	void unhighlight();


private:
	constexpr static int DEFAULT_HEIGHT{24};

	QTimer m_updateTimer;

	const Plugin::Descriptor & m_pluginDescriptor;
	QPixmap m_logo;

	bool m_mouseOver;
};


#endif
