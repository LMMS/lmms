/*
 * surround_area.h - class surroundArea which provides widget for setting
 *                   position of a channel + calculation of volume for each
 *                   speaker
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _SURROUND_AREA_H
#define _SURROUND_AREA_H

#include <QtGui/QWidget>

#include "mixer.h"


class QPixmap;
class knob;
class track;


const int SURROUND_AREA_SIZE = 1024;


class surroundArea : public QWidget
{
	Q_OBJECT
public:
	surroundArea( QWidget * _parent, const QString & _name,
							track * _track );
	virtual ~surroundArea();
	volumeVector getVolumeVector( float _v_scale ) const;
	inline const QPoint & value( void ) const
	{
		return( m_sndSrcPos );
	}
	void FASTCALL setValue( const QPoint & _p );

	void FASTCALL saveSettings( QDomDocument & _doc, QDomElement & _this,
					const QString & _name = "surpos" );
	void FASTCALL loadSettings( const QDomElement & _this,
					const QString & _name = "surpos" );


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );


signals:
	void valueChanged( const QPoint & _p );


private:
	QPoint m_sndSrcPos;

	static QPixmap * s_backgroundArtwork;

	knob * m_position_x;
	knob * m_position_y;


private slots:
	void updatePositionX( void );
	void updatePositionY( void );

} ;


#endif

