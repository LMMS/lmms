/*
 * surround_area.cpp - a widget for setting position of a channel +
 *                     calculation of volume for each speaker
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QApplication>
#include <QPainter>
#include <QCursor>
#include <QMouseEvent>
#include <QImage>

#else

#include <qapplication.h>
#include <qpainter.h>
#include <qcursor.h>
#include <qimage.h>

#endif


#include <math.h>

#include "surround_area.h"
#include "embed.h"
#include "templates.h"
#include "tooltip.h"



const QPoint surroundArea::s_defaultSpeakerPositions[SURROUND_CHANNELS] =
{
	QPoint( -SURROUND_AREA_SIZE, -SURROUND_AREA_SIZE ),
	QPoint( SURROUND_AREA_SIZE, -SURROUND_AREA_SIZE)
#ifndef DISABLE_SURROUND
,
	QPoint( -SURROUND_AREA_SIZE, SURROUND_AREA_SIZE ),
	QPoint( SURROUND_AREA_SIZE, SURROUND_AREA_SIZE )
#endif
} ;

QPixmap * surroundArea::s_backgroundArtwork = NULL;




surroundArea::surroundArea( QWidget * _parent ) :
	QWidget( _parent ),
	m_sndSrcPos( QPoint() )
{
	if( s_backgroundArtwork == NULL )
	{
		s_backgroundArtwork = new QPixmap( embed::getIconPixmap(
							"surround_area" ) );
	}

	setFixedSize( s_backgroundArtwork->width(),
						s_backgroundArtwork->height() );
#ifndef QT4
	setBackgroundMode( Qt::NoBackground );
#endif
	toolTip::add( this,
			tr( "click to where this channel should be audible" ) );
}




surroundArea::~surroundArea()
{
}




volumeVector surroundArea::getVolumeVector( float _v_scale ) const
{
	volumeVector v;
	for( Uint8 chnl = 0; chnl < SURROUND_CHANNELS; ++chnl )
	{
		v.vol[chnl] = getVolume( s_defaultSpeakerPositions[chnl],
								_v_scale );
	}

	return( v );
}




void surroundArea::setValue( const QPoint & _p )
{
	if( tLimit( _p.x(), -SURROUND_AREA_SIZE, SURROUND_AREA_SIZE ) !=
								_p.x() ||
	    tLimit( _p.y(), -SURROUND_AREA_SIZE, SURROUND_AREA_SIZE ) != _p.y() )
	{
		m_sndSrcPos = QPoint( 0, 0 );
	}
	else
	{
		m_sndSrcPos = _p;
	}
	update();
}




FASTCALL float surroundArea::getVolume( const QPoint & _speaker_pos,
							float _v_scale ) const
{
	const int x = _speaker_pos.x() - m_sndSrcPos.x();
	const int y = _speaker_pos.y() - m_sndSrcPos.y();
	const float new_vol = 2.0f - sqrt( x*x + y*y ) *
						( 1.0f / SURROUND_AREA_SIZE );

	return( tLimit( new_vol, 0.0f, 1.0f ) * _v_scale );
}




void surroundArea::paintEvent( QPaintEvent * )
{
#ifdef QT4
	QPainter p( this );
	if( s_backgroundArtwork->size() != size() )
	{
		p.drawPixmap( 0, 0, *s_backgroundArtwork );
	}
	else
	{
		p.drawPixmap( 0, 0, s_backgroundArtwork->scaled(
						width(), height(),
						Qt::IgnoreAspectRatio,
						Qt::SmoothTransformation ) );
	}
#else
	QPixmap pm;
	if( s_backgroundArtwork->size() != size() )
	{
		pm.convertFromImage(
			s_backgroundArtwork->convertToImage().smoothScale(
							width(), height() ) );
	}
	else
	{
		pm = *s_backgroundArtwork;
	}
	QPainter p( &pm );
#endif
	const int x = ( width() + m_sndSrcPos.x() * ( width() - 4 ) /
						SURROUND_AREA_SIZE ) / 2;
	const int y = ( height() + m_sndSrcPos.y() * ( height() - 4 ) /
						SURROUND_AREA_SIZE ) / 2;
	p.setPen( QColor( 64, 255, 64 ) );
	p.drawPoint( x, y - 1 );
	p.drawPoint( x - 1, y );
	p.drawPoint( x, y );
	p.drawPoint( x + 1, y );
	p.drawPoint( x, y + 1 );

#ifndef QT4
	// blit drawn pixmap to actual widget
	bitBlt( this, rect().topLeft(), &pm );
#endif
}




void surroundArea::mousePressEvent( QMouseEvent * _me )
{
	const int w = width();//s_backgroundArtwork->width();
	const int h = height();//s_backgroundArtwork->height();
	if( _me->x() > 1 && _me->x() < w-1 && _me->y() > 1 && _me->y() < h-1 )
	{
		m_sndSrcPos.setX( ( _me->x() * 2 - w ) * SURROUND_AREA_SIZE /
								( w - 4 ) );
		m_sndSrcPos.setY( ( _me->y() * 2 - h ) * SURROUND_AREA_SIZE /
								( h - 4 ) );
		update();
		if( _me->button() != Qt::NoButton )
		{
			QApplication::setOverrideCursor( Qt::BlankCursor );
		}
	}
	else
	{
		int x = tLimit( _me->x(), 2, w - 2 );
		int y = tLimit( _me->y(), 2, h - 2 );
		QCursor::setPos( mapToGlobal( QPoint( x, y ) ) );
	}

	emit valueChanged( m_sndSrcPos );
}




void surroundArea::mouseMoveEvent( QMouseEvent * _me )
{
	mousePressEvent( _me );
}




void surroundArea::mouseReleaseEvent( QMouseEvent * )
{
	QApplication::restoreOverrideCursor();
}




#include "surround_area.moc"

