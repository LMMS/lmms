/*
 * pixmap_button.h - declaration of class pixmapButton
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _PIXMAP_BUTTON_H
#define _PIXMAP_BUTTON_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <QPushButton>
#include <QPixmap>

#else

#include <qpushbutton.h>
#include <qpixmap.h>

#endif



class pixmapButton : public QPushButton
{
	Q_OBJECT
public:
	pixmapButton( QWidget * _parent );
	virtual ~pixmapButton();
	virtual void FASTCALL setActiveGraphic( const QPixmap & _pm );
	virtual void FASTCALL setInactiveGraphic( const QPixmap & _pm,
							bool _update = TRUE );
	void FASTCALL setBgGraphic( const QPixmap & _pm );


signals:
	void doubleClicked( void );
	void clickedRight( void );


protected:
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );


private:
	QPixmap * m_activePixmap;
	QPixmap * m_inactivePixmap;
	QPixmap * m_bgPixmap;

} ;

#endif
