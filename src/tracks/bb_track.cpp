/*
 * bb_track.cpp - implementation of class bbTrack
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


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QPainter>
#include <QColorDialog>
#include <QMenu>

#else

#include <qdom.h>
#include <qpainter.h>
#include <qcolordialog.h>
#include <qpopupmenu.h>

#endif


#include "bb_track.h"
#include "song_editor.h"
#include "bb_editor.h"
#include "gui_templates.h"
#include "name_label.h"
#include "embed.h"
#include "rename_dialog.h"



QMap<bbTrack *, bbTrack::bbInfoStruct> bbTrack::s_bbNums;


bbTCO::bbTCO( track * _track, const QColor & _c ) :
	trackContentObject( _track ),
	m_name( ( dynamic_cast<bbTrack *>( _track ) != NULL ) ?
		dynamic_cast<bbTrack *>( _track )->trackLabel()->text() :
								QString( "" ) ),
	m_color( _c.isValid() ? _c : QColor( 64, 128, 255 ) )
{
#ifndef QT4
	setBackgroundMode( Qt::NoBackground );
#endif
	tact t = bbEditor::inst()->lengthOfBB(
					bbTrack::numOfBBTrack( getTrack() ) );
	if( t > 0 )
	{
		changeLength( midiTime( t, 0 ) );
	}
}




bbTCO::~bbTCO()
{
}




void bbTCO::constructContextMenu( QMenu * _cm )
{
#ifdef QT4
	QAction * a = new QAction( embed::getIconPixmap( "bb_track" ),
					tr( "Open in Beat+Bassline-Editor" ),
					_cm );
	_cm->insertAction( _cm->actions()[0], a );
	connect( a, SIGNAL( triggered( bool ) ), this,
					SLOT( openInBBEditor( bool ) ) );
#else
	_cm->insertItem( embed::getIconPixmap( "bb_track" ),
					tr( "Open in Beat+Bassline-Editor" ),
					this, SLOT( openInBBEditor() ),
								0, -1, 0 );
#endif
#ifdef QT4
	_cm->insertSeparator( _cm->actions()[1] );
#else
	_cm->insertSeparator( 1 );
#endif
#ifdef QT4
	_cm->addSeparator();
#else
	_cm->insertSeparator();
#endif
	_cm->addAction( embed::getIconPixmap( "reload" ), tr( "Reset name" ),
						this, SLOT( resetName() ) );
	_cm->addAction( embed::getIconPixmap( "rename" ), tr( "Change name" ),
						this, SLOT( changeName() ) );
	_cm->addAction( embed::getIconPixmap( "colorize" ),
			tr( "Change color" ), this, SLOT( changeColor() ) );
}




void bbTCO::mouseDoubleClickEvent( QMouseEvent * )
{
	openInBBEditor();
}




void bbTCO::paintEvent( QPaintEvent * )
{
	QColor col = m_color;
	if( getTrack()->muted() )
	{
		col = QColor( 160, 160, 160 );
	}
#ifdef QT4
	QPainter p( this );
	// TODO: set according brush/pen for gradient!
	p.fillRect( rect(), col );
#else
	// create pixmap for whole widget
	QPixmap pm( rect().size() );
	// and a painter for it
	QPainter p( &pm );

	// COOL gradient ;-)
	for( int y = 1; y < height() - 1; ++y )
	{
		p.setPen( col.light( 130 - y * 60 / height() ) );
		p.drawLine( 1, y, width() - 1, y );
	}
#endif

	tact t = bbEditor::inst()->lengthOfBB( bbTrack::numOfBBTrack(
								getTrack() ) );
	if( length().getTact() > 1 && t > 0 )
	{
		for( int x = TCO_BORDER_WIDTH + static_cast<int>( t *
						pixelsPerTact() ); x < width();
			x += static_cast<int>( t * pixelsPerTact() ) )
		{
			p.setPen( col.light( 80 ) );
			p.drawLine( x, 1, x, 5 );
			p.setPen( col.light( 120 ) );
			p.drawLine( x, height() - 6, x, height() - 2 );
		}
	}

	p.setPen( col.dark() );
	p.drawRect( 0, 0, width(), height() );

	p.setFont( pointSize<7>( p.font() ) );
	p.setPen( QColor( 0, 0, 0 ) );
	p.drawText( 2, QFontMetrics( p.font() ).height() - 1, m_name );

#ifndef QT4
	bitBlt( this, rect().topLeft(), &pm );
#endif
}




void bbTCO::saveSettings( QDomDocument & _doc, QDomElement & _parent )
{
	QDomElement bbtco_de = _doc.createElement( nodeName() );
	bbtco_de.setAttribute( "name", m_name );
	if( _parent.nodeName() == "clipboard" )
	{
		bbtco_de.setAttribute( "pos", -1 );
	}
	else
	{
		bbtco_de.setAttribute( "pos", startPosition() );
	}
	bbtco_de.setAttribute( "len", length() );
	bbtco_de.setAttribute( "color", m_color.rgb() );
	_parent.appendChild( bbtco_de );
}




void bbTCO::loadSettings( const QDomElement & _this )
{
	m_name = _this.attribute( "name" );
	if( _this.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( _this.attribute( "pos" ).toInt() );
	}
	changeLength( _this.attribute( "len" ).toInt() );
	if( _this.attribute( "color" ).toUInt() != 0 )
	{
		m_color.setRgb( _this.attribute( "color" ).toUInt() );
	}
}




void bbTCO::openInBBEditor( bool )
{
	bbEditor::inst()->setCurrentBB( bbTrack::numOfBBTrack( getTrack() ) );
	bbEditor::inst()->show();
	bbEditor::inst()->setFocus();
}




void bbTCO::openInBBEditor( void )
{
	openInBBEditor( FALSE );
}




void bbTCO::resetName( void )
{
	if( dynamic_cast<bbTrack *>( getTrack() ) != NULL )
	{
		m_name = dynamic_cast<bbTrack *>( getTrack() )->
							trackLabel()->text();
	}
}




void bbTCO::changeName( void )
{
	renameDialog rename_dlg( m_name );
	rename_dlg.exec();
}




void bbTCO::changeColor( void )
{
	QColor _new_color = QColorDialog::getColor( m_color );
	if( _new_color.isValid() && _new_color != m_color )
	{
		m_color = _new_color;
		songEditor::inst()->setModified();
		update();
	}
}





bbTrack::bbTrack( trackContainer * _tc )
	: track( _tc )
{
	getTrackWidget()->setFixedHeight( 32 );
	// drag'n'drop with bb-tracks only causes troubles (and makes no sense
	// too), so disable it
	getTrackWidget()->setAcceptDrops( FALSE );

	csize bbNum = s_bbNums.size();
	bbInfoStruct bis = { bbNum, "" };
	s_bbNums[this] = bis;
	m_trackLabel = new nameLabel( tr( "Beat/Bassline %1" ).arg( bbNum ),
					getTrackSettingsWidget(),
					embed::getIconPixmap( "bb_track" ) );
	m_trackLabel->setGeometry( 1, 1, DEFAULT_SETTINGS_WIDGET_WIDTH-2, 29 );
	m_trackLabel->show();
	connect( m_trackLabel, SIGNAL( clicked() ), this,
						SLOT( clickedTrackLabel() ) );
	bbEditor::inst()->setCurrentBB( bbNum );

	_tc->updateAfterTrackAdd();
}




bbTrack::~bbTrack()
{
	csize bb = s_bbNums[this].num;
	bbEditor::inst()->removeBB( bb );
	for( QMap<bbTrack *, bbTrack::bbInfoStruct>::iterator it =
				s_bbNums.begin(); it != s_bbNums.end(); ++it )
	{
#ifdef QT4
		if( it.value().num > bb )
		{
			--it.value().num;
		}
#else
		if( it.data().num > bb )
		{
			--it.data().num;
		}
#endif
	}
	s_bbNums.remove( this );
}




track::trackTypes bbTrack::type( void ) const
{
	return( BB_TRACK );
}



// play _frames frames of given TCO within starting with _start/_start_frame
bool FASTCALL bbTrack::play( const midiTime & _start, Uint32 _start_frame,
					Uint32 _frames, Uint32 _frame_base,
							Sint16 _tco_num )
{
	if( _tco_num >= 0 )
	{
		return( bbEditor::inst()->play( _start, _start_frame, _frames,
							_frame_base,
							s_bbNums[this].num ) );
	}

	vlist<trackContentObject *> tcos;
	getTCOsInRange( tcos, _start, _start +static_cast<Sint32>( _frames *
				64 / songEditor::inst()->framesPerTact() ) );
	
	if ( tcos.size() == 0 )
	{
		return( FALSE );
	}

	midiTime lastPosition;
	midiTime lastLen;
	for( vlist<trackContentObject *>::iterator it = tcos.begin();
							it != tcos.end(); ++it )
	{
		if( ( *it )->startPosition() >= lastPosition )
		{
			lastPosition = ( *it )->startPosition();
			lastLen = ( *it )->length();
		}
	}
	if( _start - lastPosition < lastLen )
	{
		return( bbEditor::inst()->play( _start - lastPosition,
							_start_frame, _frames,
							_frame_base,
							s_bbNums[this].num ) );
	}
	return( FALSE );
}




trackContentObject * bbTrack::createTCO( const midiTime & _pos )
{
	// if we're creating a new bbTCO, we colorize it according to the
	// previous bbTCO, so we have to get all TCOs from 0 to _pos and
	// pickup the last and take the color if it
	vlist<trackContentObject *> tcos;
	getTCOsInRange( tcos, 0, _pos );
	if( tcos.size() > 0 && dynamic_cast<bbTCO *>( tcos.back() ) != NULL )
	{
		return( new bbTCO( this, 
			dynamic_cast<bbTCO *>( tcos.back() )->color() ) );
		
	}
	return( new bbTCO( this ) );
}






void bbTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent )
{
	QDomElement bbt_de = _doc.createElement( nodeName() );
	bbt_de.setAttribute( "name", m_trackLabel->text() );
/*	bbt_de.setAttribute( "current", s_bbNums[this].num ==
					bbEditor::inst()->currentBB() );*/
	_parent.appendChild( bbt_de );
	if( s_bbNums[this].num == 0 &&
				_parent.parentNode().nodeName() != "clone" )
	{
		bbEditor::inst()->saveSettings( _doc, bbt_de );
	}
}




void bbTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
	m_trackLabel->setText( _this.attribute( "name" ) );
	if( _this.firstChild().isElement() )
	{
		bbEditor::inst()->loadSettings(
					_this.firstChild().toElement() );
	}
/*	doesn't work yet because bbTrack-ctor also sets current bb so if
	bb-tracks are created after this function is called, this doesn't
	help at all....
	if( _this.attribute( "current" ).toInt() )
	{
		bbEditor::inst()->setCurrentBB( s_bbNums[this].num );
	}*/
}




// return pointer to bbTrack specified by _bb_num
bbTrack * bbTrack::findBBTrack( csize _bb_num )
{
	for( QMap<bbTrack *, bbTrack::bbInfoStruct>::iterator it =
							s_bbNums.begin();
		it != s_bbNums.end(); ++it )
	{
#ifdef QT4
		if( it.value().num == _bb_num )
#else
		if( it.data().num == _bb_num )
#endif
		{
			return( it.key() );
		}
	}
	return( NULL );
}




csize bbTrack::numOfBBTrack( track * _track )
{
	if( dynamic_cast<bbTrack *>( _track ) != NULL )
	{
		return( s_bbNums[dynamic_cast<bbTrack *>( _track )].num );
	}
	return( 0 );
}




void bbTrack::swapBBTracks( track * _track1, track * _track2 )
{
	bbTrack * t1 = dynamic_cast<bbTrack *>( _track1 );
	bbTrack * t2 = dynamic_cast<bbTrack *>( _track2 );
	if( t1 != NULL && t2 != NULL )
	{
		qSwap( s_bbNums[t1].num, s_bbNums[t2].num );
		bbEditor::inst()->swapBB( s_bbNums[t1].num, s_bbNums[t2].num );
		bbEditor::inst()->setCurrentBB( s_bbNums[t2].num );
	}
}




void bbTrack::clickedTrackLabel( void )
{
	bbEditor::inst()->setCurrentBB( s_bbNums[this].num );
	bbEditor::inst()->show();
}




#include "bb_track.moc"

