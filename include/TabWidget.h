/*
 * TabWidget.h - LMMS-tabwidget
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef TAB_WIDGET_H
#define TAB_WIDGET_H

#include <QWidget>
#include <QtCore/QMap>

#include <string>

class TabWidget : public QWidget
{
	Q_OBJECT
public:
	TabWidget( const QString & _caption, QWidget * _parent );
	virtual ~TabWidget();

	void addTab( QWidget * _w, const QString & _name, const char * pixmapName, int _idx = -1 );

	void setActiveTab( int _idx );

	inline int activeTab() const
	{
		return( m_activeTab );
	}


protected:
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re );
	virtual void wheelEvent( QWheelEvent * _we );


private:
	struct widgetDesc
	{
		QWidget       * w;	   // ptr to widget
		const char    *pixmapName; // artwork for the widget 
		QString       name;	   // name for widget
		int           nwidth;	   // width of name when painting
	} ;
	typedef QMap<int, widgetDesc> widgetStack;

	widgetStack m_widgets;
	int m_activeTab;
	QString m_caption;
	quint8 m_tabheight;
} ;

#endif
