#ifndef SINGLE_SOURCE_COMPILE

/*
 * bb_track.cpp - implementation of class bbTrack and bbTCO
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


#include <Qt/QtXml>
#include <QtGui/QColorDialog>
#include <QtGui/QMenu>
#include <QtGui/QPainter>


#include "bb_track.h"
#include "bb_editor.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "mixer.h"
#include "name_label.h"
#include "rename_dialog.h"
#include "song_editor.h"
#include "templates.h"



bbTrack::infoMap bbTrack::s_infoMap;


bbTCO::bbTCO( track * _track, const QColor & _c ) :
	trackContentObject( _track ),
	m_name( ( dynamic_cast<bbTrack *>( _track ) != NULL ) ?
		dynamic_cast<bbTrack *>( _track )->trackLabel()->text() :
								QString( "" ) ),
	m_color( _c.isValid() ? _c : QColor( 64, 128, 255 ) )
{
	tact t = engine::getBBEditor()->lengthOfBB(
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
	QAction * a = new QAction( embed::getIconPixmap( "bb_track" ),
					tr( "Open in Beat+Baseline-Editor" ),
					_cm );
	_cm->insertAction( _cm->actions()[0], a );
	connect( a, SIGNAL( triggered( bool ) ), this,
					SLOT( openInBBEditor( bool ) ) );
	_cm->insertSeparator( _cm->actions()[1] );
	_cm->addSeparator();
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
	QPainter p( this );

	QLinearGradient lingrad( 0, 0, 0, height() );
	lingrad.setColorAt( 0, col.light( 130 ) );
	lingrad.setColorAt( 1, col.light( 70 ) );
	p.fillRect( rect(), lingrad );

	tact t = engine::getBBEditor()->lengthOfBB( bbTrack::numOfBBTrack(
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
	p.drawRect( 0, 0, rect().right(), rect().bottom() );

	p.setFont( pointSize<7>( p.font() ) );
	p.setPen( QColor( 0, 0, 0 ) );
	p.drawText( 2, p.fontMetrics().height() - 1, m_name );

	if( muted() )
	{
		p.drawPixmap( 3, p.fontMetrics().height() + 1,
				embed::getIconPixmap( "muted", 16, 16 ) );
	}
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
	engine::getBBEditor()->setCurrentBB( bbTrack::numOfBBTrack(
								getTrack() ) );
	engine::getBBEditor()->show();
	engine::getBBEditor()->setFocus();
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
	if( !_new_color.isValid() )
	{
		return;
	}
	if( isSelected() )
	{
		QVector<selectableObject *> selected =
				engine::getSongEditor()->selectedObjects();
		for( QVector<selectableObject *>::iterator it =
							selected.begin();
						it != selected.end(); ++it )
		{
			bbTCO * bb_tco = dynamic_cast<bbTCO *>( *it );
			if( bb_tco )
			{
				bb_tco->setColor( _new_color );
			}
		}
	}
	else
	{
		setColor( _new_color );
	}
}




void bbTCO::setColor( QColor _new_color )
{
	if( _new_color != m_color )
	{
		m_color = _new_color;
		engine::getSongEditor()->setModified();
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

	int bbNum = s_infoMap.size();
	s_infoMap[this] = bbNum;

	m_trackLabel = new nameLabel( tr( "Beat/Baseline %1" ).arg( bbNum ),
						getTrackSettingsWidget() );
	m_trackLabel->setPixmap( embed::getIconPixmap( "bb_track" ) );
	m_trackLabel->setGeometry( 1, 1, DEFAULT_SETTINGS_WIDGET_WIDTH - 2,
									29 );
	m_trackLabel->show();
	connect( m_trackLabel, SIGNAL( clicked() ),
			this, SLOT( clickedTrackLabel() ) );
	connect( m_trackLabel, SIGNAL( nameChanged() ),
			engine::getBBEditor(), SLOT( updateComboBox() ) );
	connect( m_trackLabel, SIGNAL( pixmapChanged() ),
			engine::getBBEditor(), SLOT( updateComboBox() ) );


	engine::getBBEditor()->setCurrentBB( bbNum );
	engine::getBBEditor()->updateComboBox();
}




bbTrack::~bbTrack()
{
	engine::getMixer()->removePlayHandles( this );

	int bb = s_infoMap[this];
	engine::getBBEditor()->removeBB( bb );
	for( infoMap::iterator it = s_infoMap.begin(); it != s_infoMap.end();
									++it )
	{
		if( it.value() > bb )
		{
			--it.value();
		}
	}
	s_infoMap.remove( this );
	engine::getBBEditor()->updateComboBox();
}




track::trackTypes bbTrack::type( void ) const
{
	return( BB_TRACK );
}



// play _frames frames of given TCO within starting with _start
bool FASTCALL bbTrack::play( const midiTime & _start,
						const fpp_t _frames,
						const f_cnt_t _offset,
							Sint16 _tco_num )
{
	sendMidiTime( _start );

	if( _tco_num >= 0 )
	{
		return( engine::getBBEditor()->play( _start, _frames,
							_offset,
							s_infoMap[this] ) );
	}

	QList<trackContentObject *> tcos;
	getTCOsInRange( tcos, _start, _start + static_cast<Sint32>( _frames /
						engine::framesPerTact64th() ) );

	if ( tcos.size() == 0 )
	{
		return( FALSE );
	}

	midiTime lastPosition;
	midiTime lastLen;
	for( QList<trackContentObject *>::iterator it = tcos.begin();
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
		return( engine::getBBEditor()->play( _start - lastPosition,
							_frames,
							_offset,
							s_infoMap[this] ) );
	}
	return( FALSE );
}




trackContentObject * bbTrack::createTCO( const midiTime & _pos )
{
	// if we're creating a new bbTCO, we colorize it according to the
	// previous bbTCO, so we have to get all TCOs from 0 to _pos and
	// pickup the last and take the color if it
	QList<trackContentObject *> tcos;
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
					engine::getBBEditor()->currentBB() );*/
	if( s_infoMap[this] == 0 &&
			_this.parentNode().parentNode().nodeName() != "clone" &&
			_this.parentNode().nodeName() != "journaldata" )
	{
		( (journallingObject *)( engine::getBBEditor() ) )->saveState(
								_doc, _this );
	}

	int track_num = 0;
	QList<track *> tracks = engine::getBBEditor()->tracks();
	for( int i = 0; i < tracks.size(); ++i, ++track_num )
	{
		if( automationDisabled( tracks[i] ) )
		{
			QDomElement disabled = _doc.createElement(
							"automation-disabled" );
			disabled.setAttribute( "track", track_num );
			_this.appendChild( disabled );
		}
	}
}




void bbTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
	m_trackLabel->setText( _this.attribute( "name" ) );
	if( _this.attribute( "icon" ) != "" )
	{
		m_trackLabel->setPixmapFile( _this.attribute( "icon" ) );
	}
	engine::getBBEditor()->updateComboBox();

	QDomNode node = _this.namedItem( trackContainer::classNodeName() );
	if( node.isElement() )
	{
		( (journallingObject *)engine::getBBEditor() )->restoreState(
							node.toElement() );
	}
/*	doesn't work yet because bbTrack-ctor also sets current bb so if
	bb-tracks are created after this function is called, this doesn't
	help at all....
	if( _this.attribute( "current" ).toInt() )
	{
		engine::getBBEditor()->setCurrentBB( s_infoMap[this] );
	}*/

	QList<track *> tracks = engine::getBBEditor()->tracks();
	node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement()
				&& node.nodeName() == "automation-disabled" )
		{
			disableAutomation( tracks[node.toElement().attribute(
							"track" ).toInt()] );
		}
		node = node.nextSibling();
	}
}




// return pointer to bbTrack specified by _bb_num
bbTrack * bbTrack::findBBTrack( int _bb_num )
{
	for( infoMap::iterator it = s_infoMap.begin(); it != s_infoMap.end();
									++it )
	{
		if( it.value() == _bb_num )
		{
			return( it.key() );
		}
	}
	return( NULL );
}




int bbTrack::numOfBBTrack( track * _track )
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
		engine::getBBEditor()->swapBB( s_infoMap[t1], s_infoMap[t2] );
		engine::getBBEditor()->setCurrentBB( s_infoMap[t1] );
	}
}




void bbTrack::clickedTrackLabel( void )
{
	engine::getBBEditor()->setCurrentBB( s_infoMap[this] );
	engine::getBBEditor()->show();
}




#include "bb_track.moc"


#endif
