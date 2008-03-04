#ifndef SINGLE_SOURCE_COMPILE

/*
 * surround_area.cpp - a widget for setting position of a channel +
 *                     calculation of volume for each speaker
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "surround_area.h"


#include <QtGui/QApplication>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <Qt/QtXml>


#include "automatable_model_templates.h"
#include "caption_menu.h"
#include "embed.h"
#include "templates.h"
#include "tooltip.h"




QPixmap * surroundArea::s_backgroundArtwork = NULL;




surroundArea::surroundArea( QWidget * _parent, const QString & _name ) :
	QWidget( _parent ),
	modelView( new surroundAreaModel( NULL, NULL, TRUE ) )
{
	if( s_backgroundArtwork == NULL )
	{
		s_backgroundArtwork = new QPixmap( embed::getIconPixmap(
							"surround_area" ) );
	}

	setFixedSize( s_backgroundArtwork->width(),
						s_backgroundArtwork->height() );
	setAccessibleName( _name );
	toolTip::add( this,
			tr( "click to where this channel should be audible" ) );
}




surroundArea::~surroundArea()
{
}






void surroundArea::contextMenuEvent( QContextMenuEvent * )
{
	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent( NULL );

	captionMenu contextMenu( accessibleName() );
	contextMenu.addAction( embed::getIconPixmap( "automation" ),
					tr( "Open &X in automation editor" ),
					model()->automationPatternX(),
					SLOT( openInAutomationEditor() ) );
	contextMenu.addAction( embed::getIconPixmap( "automation" ),
					tr( "Open &Y in automation editor" ),
					model()->automationPatternY(),
					SLOT( openInAutomationEditor() ) );
	contextMenu.exec( QCursor::pos() );
}




void surroundArea::paintEvent( QPaintEvent * )
{
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
	const int x = ( width() + model()->x() * ( width() - 4 ) /
						SURROUND_AREA_SIZE ) / 2;
	const int y = ( height() + model()->y() * ( height() - 4 ) /
						SURROUND_AREA_SIZE ) / 2;
	p.setPen( QColor( 64, 255, 64 ) );
	p.drawPoint( x, y - 1 );
	p.drawPoint( x - 1, y );
	p.drawPoint( x, y );
	p.drawPoint( x + 1, y );
	p.drawPoint( x, y + 1 );
}




void surroundArea::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::RightButton )
	{
		return;
	}

	model()->prepareJournalEntryFromOldVal();

	const int w = width();//s_backgroundArtwork->width();
	const int h = height();//s_backgroundArtwork->height();
	if( _me->x() > 1 && _me->x() < w-1 && _me->y() > 1 && _me->y() < h-1 )
	{
		model()->setX( ( _me->x() * 2 - w ) * SURROUND_AREA_SIZE /
								( w - 4 ) );
		model()->setY( ( _me->y() * 2 - h ) * SURROUND_AREA_SIZE /
								( h - 4 ) );
		//update();
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
}




void surroundArea::mouseMoveEvent( QMouseEvent * _me )
{
	mousePressEvent( _me );
}




void surroundArea::mouseReleaseEvent( QMouseEvent * )
{
	model()->addJournalEntryFromOldToCurVal();
	QApplication::restoreOverrideCursor();
}








surroundAreaModel::surroundAreaModel( ::model * _parent, track * _track,
						bool _default_constructed ) :
	model( _parent, _default_constructed ),
	m_posX( 0, -SURROUND_AREA_SIZE, SURROUND_AREA_SIZE, 1, _parent ),
	m_posY( 0, -SURROUND_AREA_SIZE, SURROUND_AREA_SIZE, 1, _parent )
{
	m_posX.setTrack( _track );
	m_posY.setTrack( _track );
	connect( &m_posX, SIGNAL( dataChanged() ),
					this, SIGNAL( dataChanged() ) );
	connect( &m_posY, SIGNAL( dataChanged() ),
					this, SIGNAL( dataChanged() ) );
}




volumeVector surroundAreaModel::getVolumeVector( float _v_scale ) const
{
	volumeVector v = { { _v_scale, _v_scale
#ifndef DISABLE_SURROUND
					, _v_scale, _v_scale
#endif
			} } ;

	if( x() >= 0 )
	{
		v.vol[0] *= 1.0f - x() / (float)SURROUND_AREA_SIZE;
#ifndef DISABLE_SURROUND
		v.vol[2] *= 1.0f - x() / (float)SURROUND_AREA_SIZE;
#endif
	}
	else
	{
		v.vol[1] *= 1.0f + x() / (float)SURROUND_AREA_SIZE;
#ifndef DISABLE_SURROUND
		v.vol[3] *= 1.0f + x() / (float)SURROUND_AREA_SIZE;
#endif
	}

	if( y() >= 0 )
	{
		v.vol[0] *= 1.0f - y() / (float)SURROUND_AREA_SIZE;
		v.vol[1] *= 1.0f - y() / (float)SURROUND_AREA_SIZE;
	}
#ifndef DISABLE_SURROUND
	else
	{
		v.vol[2] *= 1.0f + y() / (float)SURROUND_AREA_SIZE;
		v.vol[3] *= 1.0f + y() / (float)SURROUND_AREA_SIZE;
	}
#endif

	return( v );
}



void surroundAreaModel::saveSettings( QDomDocument & _doc,
							QDomElement & _this,
							const QString & _name )
{
	m_posX.saveSettings( _doc, _this, _name + "-x" );
	m_posY.saveSettings( _doc, _this, _name + "-y" );
}




void surroundAreaModel::loadSettings( const QDomElement & _this,
							const QString & _name )
{
	if( _this.hasAttribute( _name ) )
	{
		const int i = _this.attribute( _name ).toInt();
		m_posX.setValue( ( i & 0xFFFF ) - 2 * SURROUND_AREA_SIZE );
		m_posY.setValue( ( i >> 16 ) - 2 * SURROUND_AREA_SIZE );
	}
	else
	{
		m_posX.loadSettings( _this, _name + "-x" );
		m_posY.loadSettings( _this, _name + "-y" );
	}
}



automationPattern * surroundAreaModel::automationPatternX( void )
{
	return( m_posX.getAutomationPattern() );
}

automationPattern * surroundAreaModel::automationPatternY( void )
{
	return( m_posY.getAutomationPattern() );
}





#include "surround_area.moc"


#endif
