/*
 * RowTableView.h - table with rows that act like single cells
 *
 * Copyright (c) 2016 Javier Serrano Polo <javier@jasp.net>
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

#ifndef ROW_TABLE_VIEW_H
#define ROW_TABLE_VIEW_H

#include <QTableView>


class RowDelegate;


class RowTableView : public QTableView
{
	Q_OBJECT
public:
	RowTableView( QWidget * parent = 0 );
	virtual ~RowTableView();

	void setModel( QAbstractItemModel * model ) override;


protected:
	void keyPressEvent( QKeyEvent * event ) override;


private:
	RowDelegate * m_rowDelegate;

} ;



#endif
