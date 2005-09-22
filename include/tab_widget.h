/*
 * tab_widget.h - LMMS-tabwidget
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
#include <QVector>

#else

#include <qwidget.h>
#include <qvaluevector.h>

#endif


class tabWidget : public QWidget
{
	Q_OBJECT
public:
	tabWidget( const QString & _caption, QWidget * _parent );
	~tabWidget();

	void addTab( QWidget * _w, const QString & _name );


protected:
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re );


private:
	struct widgetDesc
	{
		QWidget * w;	// ptr to widget
		QString name;	// name for widget
		int nwidth;	// width of name when painting
	} ;
	typedef vvector<widgetDesc> widgetStack;

	widgetStack m_widgets;
	int m_curWidget;
	QString m_caption;

} ;

#endif
