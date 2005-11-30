/*
 * tab_widget.h - LMMS-tabwidget
 *
 * Copyright (c) 2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _TAB_WIDGET_H
#define _TAB_WIDGET_H

#include "qt3support.h"

#ifdef QT4

#include <QWidget>
#include <QMap>

#else

#include <qwidget.h>
#include <qmap.h>

#include "spc_bg_hndl_widget.h"

#endif



class tabWidget : public QWidget
#ifndef QT4
					, public specialBgHandlingWidget
#endif
{
	Q_OBJECT
public:
	tabWidget( const QString & _caption, QWidget * _parent );
	~tabWidget();

	void addTab( QWidget * _w, const QString & _name, int _idx = -1 );

	inline void setActiveTab( int _idx )
	{
		if( m_widgets.contains( _idx ) )
		{
			m_activeTab = _idx;
			m_widgets[m_activeTab].w->raise();
			update();
		}
	}

	inline int activeTab( void ) const
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
		QWidget * w;	// ptr to widget
		QString name;	// name for widget
		int nwidth;	// width of name when painting
	} ;
	typedef QMap<int, widgetDesc> widgetStack;

	widgetStack m_widgets;
	int m_activeTab;
	QString m_caption;

} ;

#endif
