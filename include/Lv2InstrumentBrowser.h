/*
 * Lv2InstrumentBrowser.h - include file for Lv2InstrumentBrowser
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

class Lv2InstrumentItem;
class InstrumentTrack;
class Lv2InstrumentBrowserTreeWidget;
class PlayHandle;
class TrackContainer;

typedef struct LilvPluginImpl LilvPlugin;

class Lv2InstrumentBrowser : public SideBarWidget
{
	Q_OBJECT
public:
	Lv2InstrumentBrowser(
			const QString & title, const QPixmap & pm,
			QWidget * parent);
	virtual ~Lv2InstrumentBrowser();

private slots:
	void reloadTree( void );
	bool filterItems( const QString & filter, QTreeWidgetItem * item=NULL );
	void giveFocusToFilter();

private:
	virtual void keyPressEvent( QKeyEvent * ke );

	void addItems();

	Lv2InstrumentBrowserTreeWidget * m_treeWidget;

	QLineEdit * m_filterEdit;

	QString m_filter;

} ;




class Lv2InstrumentBrowserTreeWidget : public QTreeWidget
{
	Q_OBJECT
public:
	Lv2InstrumentBrowserTreeWidget( QWidget * parent );
	virtual ~Lv2InstrumentBrowserTreeWidget();

protected:
	virtual void contextMenuEvent( QContextMenuEvent * e );
	virtual void mousePressEvent( QMouseEvent * me );
	virtual void mouseMoveEvent( QMouseEvent * me );
	virtual void mouseReleaseEvent( QMouseEvent * me );


private:
	void handlePlugin(Lv2InstrumentItem* fi, InstrumentTrack * it );
	void openInNewInstrumentTrack( TrackContainer* tc );


	bool m_mousePressed;
	QPoint m_pressPos;

	QMutex m_pphMutex;

	Lv2InstrumentItem * m_contextMenuItem;


private slots:
	void activateListItem( QTreeWidgetItem * item, int column );
	void openInNewInstrumentTrackBBE( void );
	void openInNewInstrumentTrackSE( void );
	void sendToActiveInstrumentTrack( void );

} ;

class Lv2InstrumentItem : public QTreeWidgetItem
{
public:
	Lv2InstrumentItem (const LilvPlugin * plugin);

	inline const LilvPlugin* getPlugin()
	{
		return m_plugin;
	}

private:
	void initPixmaps( void );

	static QPixmap * s_Lv2PluginPixmap;
	const LilvPlugin * m_plugin;
} ;


#endif
