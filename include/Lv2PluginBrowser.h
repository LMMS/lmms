/*
 * Lv2PluginBrowser.h - include file for Lv2PluginBrowser
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef LV2_PLUGIN_BROWSER_H
#define LV2_PLUGIN_BROWSER_H

#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QTreeWidget>

#include "SideBarWidget.h"

class QLineEdit;

class Lv2PluginItem;
class InstrumentTrack;
class Lv2PluginBrowserTreeWidget;
class PlayHandle;
class TrackContainer;

typedef struct LilvPluginImpl LilvPlugin;

class Lv2PluginBrowser : public SideBarWidget
{
	Q_OBJECT
public:
	Lv2PluginBrowser(
			const QString & title, const QPixmap & pm,
			QWidget * parent);
	virtual ~Lv2PluginBrowser();

private slots:
	void reloadTree( void );
	bool filterItems( const QString & filter, QTreeWidgetItem * item=NULL );
	void giveFocusToFilter();

private:
	virtual void keyPressEvent( QKeyEvent * ke );

	void addItems();

	Lv2PluginBrowserTreeWidget * m_treeWidget;

	QLineEdit * m_filterEdit;

	QString m_filter;

} ;




class Lv2PluginBrowserTreeWidget : public QTreeWidget
{
	Q_OBJECT
public:
	Lv2PluginBrowserTreeWidget( QWidget * parent );
	virtual ~Lv2PluginBrowserTreeWidget();

protected:
	virtual void contextMenuEvent( QContextMenuEvent * e );
	virtual void mousePressEvent( QMouseEvent * me );
	virtual void mouseMoveEvent( QMouseEvent * me );
	virtual void mouseReleaseEvent( QMouseEvent * me );


private:
	void handlePlugin(Lv2PluginItem* fi, InstrumentTrack * it );
	void openInNewInstrumentTrack( TrackContainer* tc );


	bool m_mousePressed;
	QPoint m_pressPos;

	QMutex m_pphMutex;

	Lv2PluginItem * m_contextMenuItem;


private slots:
	void activateListItem( QTreeWidgetItem * item, int column );
	void openInNewInstrumentTrackBBE( void );
	void openInNewInstrumentTrackSE( void );
	void sendToActiveInstrumentTrack( void );

} ;

class Lv2PluginItem : public QTreeWidgetItem
{
public:
	Lv2PluginItem (const QString & uri);

	inline const QString & get_uri()
	{
		return plugin_uri;
	}

private:
	void initPixmaps( void );

	static QPixmap * s_Lv2PluginPixmap;
	const QString & plugin_uri;
} ;


#endif
