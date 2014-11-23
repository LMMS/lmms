/*
 * pixmap_button.h - declaration of class pixmapButton
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _PIXMAP_BUTTON_H
#define _PIXMAP_BUTTON_H

#include <QtGui/QPixmap>

#include "automatable_button.h"


class EXPORT pixmapButton : public automatableButton
{
	Q_OBJECT
public:
	pixmapButton( QWidget * _parent,
					const QString & _name = QString::null );
	virtual ~pixmapButton();

	void setActiveGraphic( const QPixmap & _pm );
	void setInactiveGraphic( const QPixmap & _pm, bool _update = true );


signals:
	void doubleClicked();


protected:
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );


private:
	QPixmap m_activePixmap;
	QPixmap m_inactivePixmap;
	bool	m_pressed;

} ;

#endif
