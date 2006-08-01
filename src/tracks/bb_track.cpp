#ifndef SINGLE_SOURCE_COMPILE

/*
 * bb_track.cpp - implementation of class bbTrack and bbTCO
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtGui/QColorDialog>
#include <QtGui/QMenu>
#include <QtGui/QPainter>

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
#include "templates.h"



bbTrack::infoMap bbTrack::s_infoMap;


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
	tact t = eng()->getBBEditor()->lengthOfBB(
					bbTrack::numOfBBTrack( getTrack() ) );
	if( t > 0 )
	{
		saveJournallingState( FALSE );
		changeLength( midiTime( t, 0 ) );
		restoreJournallingState();
	}
}




bbTCO::~bbTCO()
{
}




void bbTCO::constructContextMenu( QMenu * _cm )
{
#ifdef QT4
	QAction * a = new QAction( embed::getIconPixmap( "bb_track" ),
					tr( "Open in Beat+Baseline-Editor" ),
					_cm );
	_cm->insertAction( _cm->actions()[0], a );
	connect( a, SIGNAL( triggered( bool ) ), this,
					SLOT( openInBBEditor( bool ) ) );
#else
	_cm->insertItem( embed::getIconPixmap( "bb_track" ),
					tr( "Open in Beat+Baseline-Editor" ),
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
	if( getTrack()->muted() || muted() )
	{
		col = QColor( 160, 160, 160 );
	}
	if( isSelected() == TRUE )
	{
		col = QColor( tMax( col.red() - 128, 0 ),
					tMax( col.green() - 128, 0 ), 255 );
	}
#ifdef QT4
	QPainter p( this );

	QLinearGradient lingrad( 0, 0, 0, height() );
	lingrad.setColorAt( 0, col.light( 130 ) );
	lingrad.setColorAt( 1, col.light( 70 ) );
	p.fillRect( rect(), lingrad );
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

	tact t = eng()->getBBEditor()->lengthOfBB( bbTrack::numOfBBTrack(
								getTrack() ) );
	if( length() > 64 && t > 0 )
	{
		for( int x = static_cast<int>( t * pixelsPerTact() );
								x < width()-2;
			x += static_cast<int>( t * pixelsPerTact() ) )
		{
			p.setPen( col.light( 80 ) );
			p.drawLine( x, 1, x, 5 );
			p.setPen( col.light( 120 ) );
			p.drawLine( x, height() - 6, x, height() - 2 );
		}
	}

	p.setPen( col.dark() );
#ifndef QT3
	p.drawRect( 0, 0, rect().right(), rect().bottom() );
#else
	p.drawRect( rect() );
#endif

	p.setFont( pointSize<7>( p.font() ) );
	p.setPen( QColor( 0, 0, 0 ) );
	p.drawText( 2, p.fontMetrics().height() - 1, m_name );

	if( muted() )
	{
		p.drawPixmap( 3, p.fontMetrics().height() + 1,
				embed::getIconPixmap( "muted", 16, 16 ) );
	}

#ifndef QT4
	bitBlt( this, rect().topLeft(), &pm );
#endif
}




void bbTCO::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "name", m_name );
	if( _this.parentNode().nodeName() == "clipboard" )
	{
		_this.setAttribute( "pos", -1 );
	}
	else
	{
		_this.setAttribute( "pos", startPosition() );
	}
	_this.setAttribute( "len", length() );
	_this.setAttribute( "muted", muted() );
	_this.setAttribute( "color", m_color.rgb() );
}




void bbTCO::loadSettings( const QDomElement & _this )
{
	m_name = _this.attribute( "name" );
	if( _this.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( _this.attribute( "pos" ).toInt() );
	}
	changeLength( _this.attribute( "len" ).toInt() );
	if( _this.attribute( "muted" ).toInt() != muted() )
	{
		toggleMute();
	}

	if( _this.attribute( "color" ).toUInt() != 0 )
	{
		m_color.setRgb( _this.attribute( "color" ).toUInt() );
	}
}




void bbTCO::openInBBEditor( bool )
{
	eng()->getBBEditor()->setCurrentBB( bbTrack::numOfBBTrack(
								getTrack() ) );
	eng()->getBBEditor()->show();
	eng()->getBBEditor()->setFocus();
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
		eng()->getSongEditor()->setModified();
		update();
	}
}





bbTrack::bbTrack( trackContainer * _tc ) :
	track( _tc )
{
	getTrackWidget()->setFixedHeight( 32 );
	// drag'n'drop with bb-tracks only causes troubles (and makes no sense
	// too), so disable it
	getTrackWidget()->setAcceptDrops( FALSE );

	csize bbNum = s_infoMap.size();
	s_infoMap[this] = bbNum;

	m_trackLabel = new nameLabel( tr( "Beat/Baseline %1" ).arg( bbNum ),
					getTrackSettingsWidget(), eng() );
	m_trackLabel->setPixmap( embed::getIconPixmap( "bb_track" ) );
	m_trackLabel->setGeometry( 1, 1, DEFAULT_SETTINGS_WIDGET_WIDTH - 2,
									29 );
	m_trackLabel->show();
	connect( m_trackLabel, SIGNAL( clicked() ),
			this, SLOT( clickedTrackLabel() ) );
	connect( m_trackLabel, SIGNAL( nameChanged() ),
			eng()->getBBEditor(), SLOT( updateComboBox() ) );
	connect( m_trackLabel, SIGNAL( pixmapChanged() ),
			eng()->getBBEditor(), SLOT( updateComboBox() ) );


	eng()->getBBEditor()->setCurrentBB( bbNum );
	eng()->getBBEditor()->updateComboBox();

	_tc->updateAfterTrackAdd();
}




bbTrack::~bbTrack()
{
	csize bb = s_infoMap[this];
	eng()->getBBEditor()->removeBB( bb );
	for( infoMap::iterator it = s_infoMap.begin(); it != s_infoMap.end();
									++it )
	{
#ifdef QT4
		if( it.value() > bb )
		{
			--it.value();
		}
#else
		if( it.data() > bb )
		{
			--it.data();
		}
#endif
	}
	s_infoMap.remove( this );
	eng()->getBBEditor()->updateComboBox();
}




track::trackTypes bbTrack::type( void ) const
{
	return( BB_TRACK );
}



// play _frames frames of given TCO within starting with _start/_start_frame
bool FASTCALL bbTrack::play( const midiTime & _start,
						const f_cnt_t _start_frame,
						const fpab_t _frames,
						const f_cnt_t _frame_base,
							Sint16 _tco_num )
{
	if( _tco_num >= 0 )
	{
		return( eng()->getBBEditor()->play( _start, _start_frame,
							_frames,
							_frame_base,
							s_infoMap[this] ) );
	}

	vlist<trackContentObject *> tcos;
	getTCOsInRange( tcos, _start, _start +static_cast<Sint32>( _frames *
						64 / eng()->framesPerTact() ) );
	
	if ( tcos.size() == 0 )
	{
		return( FALSE );
	}

	midiTime lastPosition;
	midiTime lastLen;
	for( vlist<trackContentObject *>::iterator it = tcos.begin();
							it != tcos.end(); ++it )
	{
		if( !( *it )->muted() &&
				( *it )->startPosition() >= lastPosition )
		{
			lastPosition = ( *it )->startPosition();
			lastLen = ( *it )->length();
		}
	}
	if( _start - lastPosition < lastLen )
	{
		return( eng()->getBBEditor()->play( _start - lastPosition,
							_start_frame, _frames,
							_frame_base,
							s_infoMap[this] ) );
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
							QDomElement & _this )
{
	_this.setAttribute( "name", m_trackLabel->text() );
	_this.setAttribute( "icon", m_trackLabel->pixmapFile() );
/*	_this.setAttribute( "current", s_infoMap[this] ==
					eng()->getBBEditor()->currentBB() );*/
	if( s_infoMap[this] == 0 &&
			_this.parentNode().nodeName() != "clone" &&
			_this.parentNode().nodeName() != "journaldata" )
	{
		( (journallingObject *)( eng()->getBBEditor() ) )->saveState(
								_doc, _this );
	}
}




void bbTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
	m_trackLabel->setText( _this.attribute( "name" ) );
	if( _this.attribute( "icon" ) != "" )
	{
		m_trackLabel->setPixmapFile( _this.attribute( "icon" ) );
	}
	if( _this.firstChild().isElement() )
	{
		( (journallingObject *)( eng()->getBBEditor() ) )->restoreState(
					_this.firstChild().toElement() );
	}
/*	doesn't work yet because bbTrack-ctor also sets current bb so if
	bb-tracks are created after this function is called, this doesn't
	help at all....
	if( _this.attribute( "current" ).toInt() )
	{
		eng()->getBBEditor()->setCurrentBB( s_infoMap[this] );
	}*/
}




// return pointer to bbTrack specified by _bb_num
bbTrack * bbTrack::findBBTrack( csize _bb_num, engine * _engine )
{
	for( infoMap::iterator it = s_infoMap.begin(); it != s_infoMap.end();
									++it )
	{
#ifdef QT4
		if( it.value() == _bb_num && it.key()->eng() == _engine )
#else
		if( it.data() == _bb_num && it.key()->eng() == _engine )
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
		return( s_infoMap[dynamic_cast<bbTrack *>( _track )] );
	}
	return( 0 );
}




void bbTrack::swapBBTracks( track * _track1, track * _track2 )
{
	bbTrack * t1 = dynamic_cast<bbTrack *>( _track1 );
	bbTrack * t2 = dynamic_cast<bbTrack *>( _track2 );
	if( t1 != NULL && t2 != NULL )
	{
		qSwap( s_infoMap[t1], s_infoMap[t2] );
		_track1->eng()->getBBEditor()->swapBB( s_infoMap[t1],
								s_infoMap[t2] );
		_track1->eng()->getBBEditor()->setCurrentBB( s_infoMap[t1] );
	}
}




void bbTrack::clickedTrackLabel( void )
{
	eng()->getBBEditor()->setCurrentBB( s_infoMap[this] );
	eng()->getBBEditor()->show();
}




#include "bb_track.moc"


#endif
