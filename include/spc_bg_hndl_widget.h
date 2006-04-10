/*
 * spc_bg_hndl_widget.h - class specialBgHandlingWidget
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


#ifndef _SPC_BG_HNDL_WIDGET_H
#define _SPC_BG_HNDL_WIDGET_H

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QWidget>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#else

#include <qwidget.h>
#include <qpixmap.h>

#endif


class specialBgHandlingWidget
{
public:
	specialBgHandlingWidget( const QPixmap & _pm ) :
		m_backgroundPixmap( _pm ),
		m_backgroundColor( QColor( 0, 0, 0 ) )
	{
	}

	specialBgHandlingWidget( const QColor & _c ) :
		m_backgroundPixmap(),
		m_backgroundColor( _c )
	{
	}

	~specialBgHandlingWidget()
	{
	}

	const QPixmap & backgroundPixmap( void ) const
	{
		return( m_backgroundPixmap );
	}
	const QColor & backgroundColor( void ) const
	{
		return( m_backgroundColor );
	}

	static QPixmap getBackground( const QWidget * _w )
	{
		QPixmap pm( _w->size() );
		const QWidget * pw = _w->parentWidget();
		if( dynamic_cast<const specialBgHandlingWidget *>( pw ) )
		{
			const specialBgHandlingWidget * s = dynamic_cast<
					const specialBgHandlingWidget *>( pw );
			if( s->backgroundPixmap().isNull() == FALSE )
			{
#ifdef QT4
				QPainter p( &pm );
				p.drawPixmap( 0, 0, s->backgroundPixmap(),
						_w->x(), _w->y(), _w->width(),
						_w->height() );
#else
				bitBlt( &pm, 0, 0, &s->backgroundPixmap(),
						_w->x(), _w->y(), _w->width(),
						_w->height() );
#endif
			}
			else
			{
				pm.fill( s->backgroundColor() );
			}
		}
		else
		{
#ifdef QT4
			QPainter p( &pm );
			// TODO: fix that for background-pixmaps, because
			// drawing is started at the top left edge even
			// if this widget isn't posated there
			p.fillRect( _w->rect(), pw->palette().brush(
						pw->backgroundRole() ) );
#else
			const QPixmap * pbp = pw->paletteBackgroundPixmap();
			if( pbp == NULL )
			{
				pbp = pw->erasePixmap();
			}
			if( pbp )
			{
				bitBlt( &pm, 0, 0, pbp, _w->x(), _w->y(),
						_w->width(), _w->height() );
			}
			else
			{
				pm.fill( pw->paletteBackgroundColor() );
			}
#endif
		}
		return( pm );
	}



private:
	QPixmap m_backgroundPixmap;
	QColor m_backgroundColor;

} ;

#endif
