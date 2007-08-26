/*
 * spc_bg_hndl_widget.h - class specialBgHandlingWidget
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _SPC_BG_HNDL_WIDGET_H
#define _SPC_BG_HNDL_WIDGET_H

#include <QtGui/QWidget>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>



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
				QPainter p( &pm );
				p.drawPixmap( 0, 0, s->backgroundPixmap(),
						_w->x(), _w->y(), _w->width(),
						_w->height() );
			}
			else
			{
				pm.fill( s->backgroundColor() );
			}
		}
		else
		{
			QPainter p( &pm );
			const QBrush & br = pw->palette().brush(
							pw->backgroundRole() );
			if( br.style() == Qt::TexturePattern )
			{
				p.drawPixmap( 0, 0, br.texture(),
						_w->x(), _w->y(),
						_w->width(), _w->height() );
			}
			else
			{
				pm.fill( br.color() );
			}
		}
		return( pm );
	}



private:
	QPixmap m_backgroundPixmap;
	QColor m_backgroundColor;

} ;

#endif
