#ifndef SINGLE_SOURCE_COMPILE

/*
 * surround_area.cpp - a widget for setting position of a channel +
 *                     calculation of volume for each speaker
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


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

#else

#include <qapplication.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qpopupmenu.h>
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




surroundArea::surroundArea( QWidget * _parent, const QString & _name,
							track * _track ) :
	QWidget( _parent
#ifndef QT4
			, _name.ascii()
#endif
		),
	m_sndSrcPos( QPoint() )
{
	m_position_x = new knob( knobDark_28, NULL, tr ( "Surround area X" ),
								_track );
	m_position_x->setRange( -SURROUND_AREA_SIZE, SURROUND_AREA_SIZE, 1.0f );
	m_position_x->setInitValue( 0.0f );

	m_position_y = new knob( knobDark_28, NULL, tr ( "Surround area Y" ),
								_track );
	m_position_y->setRange( -SURROUND_AREA_SIZE, SURROUND_AREA_SIZE, 1.0f );
	m_position_y->setInitValue( 0.0f );

	connect( m_position_x, SIGNAL( valueChanged( float ) ), this,
					SLOT( updatePositionX( void ) ) );
	connect( m_position_y, SIGNAL( valueChanged( float ) ), this,
					SLOT( updatePositionY( void ) ) );

	if( s_backgroundArtwork == NULL )
	{
		s_backgroundArtwork = new QPixmap( embed::getIconPixmap(
							"surround_area" ) );
	}

	setFixedSize( s_backgroundArtwork->width(),
						s_backgroundArtwork->height() );
#ifdef QT4
	setAccessibleName( _name );
#else
	setBackgroundMode( Qt::NoBackground );
#endif
	toolTip::add( this,
			tr( "click to where this channel should be audible" ) );
}




surroundArea::~surroundArea()
{
	delete m_position_x;
	delete m_position_y;
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




void surroundArea::contextMenuEvent( QContextMenuEvent * )
{
	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent( NULL );

	QMenu contextMenu( this );
#ifdef QT4
	contextMenu.setTitle( accessibleName() );
#else
	QLabel * caption = new QLabel( "<font color=white><b>" +
			QString( accessibleName() ) + "</b></font>", this );
	caption->setPaletteBackgroundColor( QColor( 0, 0, 192 ) );
	caption->setAlignment( Qt::AlignCenter );
	contextMenu.addAction( caption );
#endif
	contextMenu.addAction( embed::getIconPixmap( "automation" ),
					tr( "Open &X in automation editor" ),
					m_position_x->getAutomationPattern(),
					SLOT( openInAutomationEditor() ) );
	contextMenu.addAction( embed::getIconPixmap( "automation" ),
					tr( "Open &Y in automation editor" ),
					m_position_y->getAutomationPattern(),
					SLOT( openInAutomationEditor() ) );
	contextMenu.exec( QCursor::pos() );
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
	if( _me->button() == Qt::RightButton )
	{
		return;
	}

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

	m_position_x->setValue( m_sndSrcPos.x() );
	m_position_y->setValue( m_sndSrcPos.y() );

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




void surroundArea::updatePositionX( void )
{
	m_sndSrcPos.setX( (int)roundf( m_position_x->value() ) );
	update();
}




void surroundArea::updatePositionY( void )
{
	m_sndSrcPos.setY( (int)roundf( m_position_y->value() ) );
	update();
}




void FASTCALL surroundArea::saveSettings( QDomDocument & _doc,
							QDomElement & _this,
							const QString & _name )
{
	m_position_x->saveSettings( _doc, _this, _name + "-x" );
	m_position_y->saveSettings( _doc, _this, _name + "-y" );
}




void FASTCALL surroundArea::loadSettings( const QDomElement & _this,
							const QString & _name )
{
	if( _this.hasAttribute( _name ) )
	{
		int i = _this.attribute( _name ).toInt();
		setValue( QPoint( ( i & 0xFFFF ) - 2 * SURROUND_AREA_SIZE,
					( i >> 16 ) - 2 * SURROUND_AREA_SIZE) );
		m_position_x->setValue( m_sndSrcPos.x() );
		m_position_y->setValue( m_sndSrcPos.y() );
	}
	else
	{
		m_position_x->loadSettings( _this, _name + "-x" );
		m_position_y->loadSettings( _this, _name + "-y" );
	}
}




#include "surround_area.moc"


#endif
