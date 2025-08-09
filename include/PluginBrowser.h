/*
 * PluginBrowser.h - include file for PluginBrowser
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

#ifndef LMMS_GUI_PLUGIN_BROWSER_H
#define LMMS_GUI_PLUGIN_BROWSER_H

#include <QPixmap>

#include "SideBarWidget.h"
#include "Plugin.h"

class QTreeWidget;

namespace lmms::gui
{

class PluginBrowser : public SideBarWidget
{
	Q_OBJECT
public:
	PluginBrowser( QWidget * _parent );
	~PluginBrowser() override = default;

private slots:
	void onFilterChanged( const QString & filter );

private:
	void addPlugins();
	void updateRootVisibility( int index );
	void updateRootVisibilities();

	QWidget * m_view;
	QTreeWidget * m_descTree;
};


class PluginDescWidget : public QWidget
{
	Q_OBJECT
public:
	using PluginKey = Plugin::Descriptor::SubPluginFeatures::Key;
	PluginDescWidget( const PluginKey & _pk, QWidget * _parent );
	QString name() const;
	void openInNewInstrumentTrack(QString value);

protected:
	void enterEvent( QEvent * _e ) override;
	void leaveEvent( QEvent * _e ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void paintEvent( QPaintEvent * _pe ) override;
	void contextMenuEvent(QContextMenuEvent* e) override;

private:
	constexpr static int DEFAULT_HEIGHT{24};

	PluginKey m_pluginKey;
	QPixmap m_logo;

	bool m_mouseOver;
};


} // namespace lmms::gui

#endif // LMMS_GUI_PLUGIN_BROWSER_H
