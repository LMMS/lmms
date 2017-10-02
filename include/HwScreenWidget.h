/*
 * LcdWidget.h - a widget for displaying numbers in LCD style
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef HW_SCREEN_WIDGET_H
#define HW_SCREEN_WIDGET_H

/*

#include <QPaintEvent>
#include <QPixmap>
#include <QWidget>

#include "export.h"
#include "QQ_MACROS.h"

class EXPORT HwScreenWidget : public QWidget
{
	Q_OBJECT

	QQ_LOCAL_PROPERTY(QString,text,text,setText)
	QQ_LOCAL_PROPERTY(QString,colorSet,colorSet,setColorSet)
	QQ_LOCAL_PROPERTY(int,columns,columns,setColumns)
	QQ_LOCAL_PROPERTY(int,rows,rows,setRows)
	QQ_LOCAL_PROPERTY(QString,label,label,setLabel)

	HwScreenWidget(QWidget* parent = NULL, int columns = 16, int rows = 2,
		       const QString & label = QString(""),
		       const QString & colorSet = QString("gob"));

	virtual ~HwScreenWidget();

public slots:
	//virtual void setMarginWidth( int width );


protected:
	static const int CHARS_PER_PIXMAP = 96;

        QSize m_cellSize;
	QPixmap* m_screenPixmap;

        virtual void paintEvent( QPaintEvent * pe );

	virtual void updateSize();

	void initUi(int columns, int rows, const QString & label, const QString & colorSet);

private:

*/

} ;

#endif
