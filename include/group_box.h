/*
 * group_box.h - LMMS-groupbox
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _GROUP_BOX_H
#define _GROUP_BOX_H

#include "qt3support.h"

#ifdef QT4

#include <QWidget>

#else

#include <qwidget.h>

#endif

#include "pixmap_button.h"

class QPixmap;


class groupBox : public QWidget
{
	Q_OBJECT
public:
	groupBox( const QString & _caption, QWidget * _parent );
	virtual ~groupBox();

	bool isActive( void ) const
	{
#ifdef QT4
		return( m_led->isChecked() );
#else
		return( m_led->isOn() );
#endif
	}


public slots:
	void setState( bool _on, bool _anim = FALSE );
	void animate( void );


protected:
	virtual void resizeEvent( QResizeEvent * _re );
	virtual void mousePressEvent( QMouseEvent * _me );


private:
	void updatePixmap( void );

	static QPixmap * s_ledBg;

	pixmapButton * m_led;
	QString m_caption;

	int m_origHeight;
	bool m_animating;

} ;

#endif
