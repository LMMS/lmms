/*
 * combobox.h - class comboBox, a very cool combo-box
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008-2009 Paul Giblock <pgib/at/users.sourceforge.net>
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


#ifndef _COMBOBOX_H
#define _COMBOBOX_H

#include <QtGui/QMenu>
#include <QtGui/QWidget>

#include "ComboBoxModel.h"
#include "AutomatableModelView.h"



class EXPORT comboBox : public QWidget, public IntModelView
{
	Q_OBJECT
public:
	comboBox( QWidget * _parent, const QString & _name = QString() );
	virtual ~comboBox();

	ComboBoxModel * model()
	{
		return( castModel<ComboBoxModel>() );
	}

	const ComboBoxModel * model() const
	{
		return( castModel<ComboBoxModel>() );
	}


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void wheelEvent( QWheelEvent * _we );


private:
	static QPixmap * s_background;
	static QPixmap * s_arrow;
	static QPixmap * s_arrowSelected;

	QMenu m_menu;

	bool m_pressed;


private slots:
	void setItem( QAction * _item );

} ;

#endif
