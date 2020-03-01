/*
 * GroupBox.h - LMMS-groupbox
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef GROUP_BOX_H
#define GROUP_BOX_H

#include <QWidget>

#include "AutomatableModelView.h"
#include "PixmapButton.h"


class QPixmap;


class GroupBox : public QWidget, public BoolModelView
{
	Q_OBJECT
public:
	GroupBox( const QString & _caption, QWidget * _parent = NULL );
	virtual ~GroupBox();

	void modelChanged() override;

	PixmapButton * ledButton()
	{
		return m_led;
	}

	int titleBarHeight() const
	{
		return m_titleBarHeight;
	}


protected:
	void mousePressEvent( QMouseEvent * _me ) override;
	void paintEvent( QPaintEvent * _pe ) override;


private:
	void updatePixmap();

	PixmapButton * m_led;
	QString m_caption;
	const int m_titleBarHeight;

} ;




#endif
