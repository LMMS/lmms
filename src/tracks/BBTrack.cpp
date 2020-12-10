/*
 * BBTrack.cpp - implementation of class BBTrack and bbClip
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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
#include "BBTrack.h"

#include <QDomElement>
#include <QMenu>
#include <QPainter>

#include "BBEditor.h"
#include "BBTrackContainer.h"
#include "embed.h"
#include "Engine.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "RenameDialog.h"
#include "Song.h"
#include "SongEditor.h"
#include "ToolTip.h"
#include "TrackLabelButton.h"



BBTrack::infoMap BBTrack::s_infoMap;


BBClip::BBClip( Track * _track ) :
	Clip( _track )
{
	bar_t t = Engine::getBBTrackContainer()->lengthOfBB( bbTrackIndex() );
	if( t > 0 )
	{
		saveJournallingState( false );
		changeLength( TimePos( t, 0 ) );
		restoreJournallingState();
	}
	setAutoResize( false );
}

void BBClip::saveSettings( QDomDocument & doc, QDomElement & element )
{
	element.setAttribute( "name", name() );
	if( element.parentNode().nodeName() == "clipboard" )
	{
		element.setAttribute( "pos", -1 );
	}
	else
	{
		element.setAttribute( "pos", startPosition() );
	}
	element.setAttribute( "len", length() );
	element.setAttribute( "muted", isMuted() );
	if( usesCustomClipColor() )
	{
		element.setAttribute( "color", color().name() );
	}
}




void BBClip::loadSettings( const QDomElement & element )
{
	setName( element.attribute( "name" ) );
	if( element.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( element.attribute( "pos" ).toInt() );
	}
	changeLength( element.attribute( "len" ).toInt() );
	if( element.attribute( "muted" ).toInt() != isMuted() )
	{
		toggleMute();
	}
	
	// for colors saved in 1.3-onwards
	if( element.hasAttribute( "color" ) && !element.hasAttribute( "usestyle" ) )
	{
		useCustomClipColor( true );
		setColor( element.attribute( "color" ) );
	}
	
	// for colors saved before 1.3
	else
	{
		if( element.hasAttribute( "color" ) )
		{ setColor( QColor( element.attribute( "color" ).toUInt() ) ); }
		
		// usestyle attribute is no longer used
	}
}



int BBClip::bbTrackIndex()
{
	return dynamic_cast<BBTrack *>( getTrack() )->index();
}



ClipView * BBClip::createView( TrackView * _tv )
{
	return new BBClipView( this, _tv );
}




BBClipView::BBClipView( Clip * _clip, TrackView * _tv ) :
	ClipView( _clip, _tv ),
	m_bbClip( dynamic_cast<BBClip *>( _clip ) ),
	m_paintPixmap()
{
	connect( _clip->getTrack(), SIGNAL( dataChanged() ), 
			this, SLOT( update() ) );

	setStyle( QApplication::style() );
}

void BBClipView::constructContextMenu( QMenu * _cm )
{
	QAction * a = new QAction( embed::getIconPixmap( "bb_track" ),
					tr( "Open in Beat+Bassline-Editor" ),
					_cm );
	_cm->insertAction( _cm->actions()[0], a );
	connect( a, SIGNAL( triggered( bool ) ),
			this, SLOT( openInBBEditor() ) );
	_cm->insertSeparator( _cm->actions()[1] );
	_cm->addSeparator();
	_cm->addAction( embed::getIconPixmap( "reload" ), tr( "Reset name" ),
						this, SLOT( resetName() ) );
	_cm->addAction( embed::getIconPixmap( "edit_rename" ),
						tr( "Change name" ),
						this, SLOT( changeName() ) );
}




void BBClipView::mouseDoubleClickEvent( QMouseEvent * )
{
	openInBBEditor();
}




void BBClipView::paintEvent( QPaintEvent * )
{
	QPainter painter( this );

	if( !needsUpdate() )
	{
		painter.drawPixmap( 0, 0, m_paintPixmap );
		return;
	}

	setNeedsUpdate( false );

	if (m_paintPixmap.isNull() || m_paintPixmap.size() != size())
	{
		m_paintPixmap = QPixmap(size());
	}

	QPainter p( &m_paintPixmap );

	QLinearGradient lingrad( 0, 0, 0, height() );
	QColor c = getColorForDisplay( painter.background().color() );
	
	lingrad.setColorAt( 0, c.lighter( 130 ) );
	lingrad.setColorAt( 1, c.lighter( 70 ) );

	// paint a black rectangle under the pattern to prevent glitches with transparent backgrounds
	p.fillRect( rect(), QColor( 0, 0, 0 ) );

	if( gradient() )
	{
		p.fillRect( rect(), lingrad );
	}
	else
	{
		p.fillRect( rect(), c );
	}
	
	// bar lines
	const int lineSize = 3;
	p.setPen( c.darker( 200 ) );

	bar_t t = Engine::getBBTrackContainer()->lengthOfBB( m_bbClip->bbTrackIndex() );
	if( m_bbClip->length() > TimePos::ticksPerBar() && t > 0 )
	{
		for( int x = static_cast<int>( t * pixelsPerBar() );
								x < width() - 2;
			x += static_cast<int>( t * pixelsPerBar() ) )
		{
			p.drawLine( x, Clip_BORDER_WIDTH, x, Clip_BORDER_WIDTH + lineSize );
			p.drawLine( x, rect().bottom() - ( Clip_BORDER_WIDTH + lineSize ),
			 	x, rect().bottom() - Clip_BORDER_WIDTH );
		}
	}

	// pattern name
	paintTextLabel(m_bbClip->name(), p);

	// inner border
	p.setPen( c.lighter( 130 ) );
	p.drawRect( 1, 1, rect().right() - Clip_BORDER_WIDTH,
		rect().bottom() - Clip_BORDER_WIDTH );	

	// outer border
	p.setPen( c.darker( 300 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );
	
	// draw the 'muted' pixmap only if the pattern was manualy muted
	if( m_bbClip->isMuted() )
	{
		const int spacing = Clip_BORDER_WIDTH;
		const int size = 14;
		p.drawPixmap( spacing, height() - ( size + spacing ),
			embed::getIconPixmap( "muted", size, size ) );
	}
	
	p.end();
	
	painter.drawPixmap( 0, 0, m_paintPixmap );
	
}




void BBClipView::openInBBEditor()
{
	Engine::getBBTrackContainer()->setCurrentBB( m_bbClip->bbTrackIndex() );

	gui->mainWindow()->toggleBBEditorWin( true );
}




void BBClipView::resetName() { m_bbClip->setName(""); }




void BBClipView::changeName()
{
	QString s = m_bbClip->name();
	RenameDialog rename_dlg( s );
	rename_dlg.exec();
	m_bbClip->setName( s );
}



void BBClipView::update()
{
	ToolTip::add(this, m_bbClip->name());

	ClipView::update();
}



BBTrack::BBTrack( TrackContainer* tc ) :
	Track( Track::BBTrack, tc )
{
	int bbNum = s_infoMap.size();
	s_infoMap[this] = bbNum;

	setName( tr( "Beat/Bassline %1" ).arg( bbNum ) );
	Engine::getBBTrackContainer()->createClipsForBB( bbNum );
	Engine::getBBTrackContainer()->setCurrentBB( bbNum );
	Engine::getBBTrackContainer()->updateComboBox();

	connect( this, SIGNAL( nameChanged() ),
		Engine::getBBTrackContainer(), SLOT( updateComboBox() ) );
}




BBTrack::~BBTrack()
{
	Engine::mixer()->removePlayHandlesOfTypes( this,
					PlayHandle::TypeNotePlayHandle
					| PlayHandle::TypeInstrumentPlayHandle
					| PlayHandle::TypeSamplePlayHandle );

	const int bb = s_infoMap[this];
	Engine::getBBTrackContainer()->removeBB( bb );
	for( infoMap::iterator it = s_infoMap.begin(); it != s_infoMap.end();
									++it )
	{
		if( it.value() > bb )
		{
			--it.value();
		}
	}
	s_infoMap.remove( this );

	// remove us from TC so bbTrackContainer::numOfBBs() returns a smaller
	// value and thus combobox-updating in bbTrackContainer works well
	trackContainer()->removeTrack( this );
	Engine::getBBTrackContainer()->updateComboBox();
}




// play _frames frames of given Clip within starting with _start
bool BBTrack::play( const TimePos & _start, const fpp_t _frames,
					const f_cnt_t _offset, int _clip_num )
{
	if( isMuted() )
	{
		return false;
	}

	if( _clip_num >= 0 )
	{
		return Engine::getBBTrackContainer()->play( _start, _frames, _offset, s_infoMap[this] );
	}

	clipVector clips;
	getClipsInRange( clips, _start, _start + static_cast<int>( _frames / Engine::framesPerTick() ) );

	if( clips.size() == 0 )
	{
		return false;
	}

	TimePos lastPosition;
	TimePos lastLen;
	for( clipVector::iterator it = clips.begin(); it != clips.end(); ++it )
	{
		if( !( *it )->isMuted() &&
				( *it )->startPosition() >= lastPosition )
		{
			lastPosition = ( *it )->startPosition();
			lastLen = ( *it )->length();
		}
	}

	if( _start - lastPosition < lastLen )
	{
		return Engine::getBBTrackContainer()->play( _start - lastPosition, _frames, _offset, s_infoMap[this] );
	}
	return false;
}




TrackView * BBTrack::createView( TrackContainerView* tcv )
{
	return new BBTrackView( this, tcv );
}




Clip* BBTrack::createClip(const TimePos & pos)
{
	BBClip* bbclip = new BBClip(this);
	bbclip->movePosition(pos);
	return bbclip;
}




void BBTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
//	_this.setAttribute( "icon", m_trackLabel->pixmapFile() );
/*	_this.setAttribute( "current", s_infoMap[this] ==
					engine::getBBEditor()->currentBB() );*/
	if( s_infoMap[this] == 0 &&
			_this.parentNode().parentNode().nodeName() != "clone" &&
			_this.parentNode().parentNode().nodeName() != "journaldata" )
	{
		( (JournallingObject *)( Engine::getBBTrackContainer() ) )->
						saveState( _doc, _this );
	}
	if( _this.parentNode().parentNode().nodeName() == "clone" )
	{
		_this.setAttribute( "clonebbt", s_infoMap[this] );
	}
}




void BBTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
/*	if( _this.attribute( "icon" ) != "" )
	{
		m_trackLabel->setPixmapFile( _this.attribute( "icon" ) );
	}*/

	if( _this.hasAttribute( "clonebbt" ) )
	{
		const int src = _this.attribute( "clonebbt" ).toInt();
		const int dst = s_infoMap[this];
		TrackContainer::TrackList tl =
					Engine::getBBTrackContainer()->tracks();
		// copy Clips of all tracks from source BB (at bar "src") to destination
		// Clips (which are created if they do not exist yet)
		for( TrackContainer::TrackList::iterator it = tl.begin();
							it != tl.end(); ++it )
		{
			Clip::copyStateTo( ( *it )->getClip( src ),
				( *it )->getClip( dst ) );
		}
		setName( tr( "Clone of %1" ).arg(
					_this.parentNode().toElement().attribute( "name" ) ) );
	}
	else
	{
		QDomNode node = _this.namedItem(
					TrackContainer::classNodeName() );
		if( node.isElement() )
		{
			( (JournallingObject *)Engine::getBBTrackContainer() )->
					restoreState( node.toElement() );
		}
	}
/*	doesn't work yet because BBTrack-ctor also sets current bb so if
	bb-tracks are created after this function is called, this doesn't
	help at all....
	if( _this.attribute( "current" ).toInt() )
	{
		engine::getBBEditor()->setCurrentBB( s_infoMap[this] );
	}*/
}




// return pointer to BBTrack specified by _bb_num
BBTrack * BBTrack::findBBTrack( int _bb_num )
{
	for( infoMap::iterator it = s_infoMap.begin(); it != s_infoMap.end();
									++it )
	{
		if( it.value() == _bb_num )
		{
			return it.key();
		}
	}
	return NULL;
}




void BBTrack::swapBBTracks( Track * _track1, Track * _track2 )
{
	BBTrack * t1 = dynamic_cast<BBTrack *>( _track1 );
	BBTrack * t2 = dynamic_cast<BBTrack *>( _track2 );
	if( t1 != NULL && t2 != NULL )
	{
		qSwap( s_infoMap[t1], s_infoMap[t2] );
		Engine::getBBTrackContainer()->swapBB( s_infoMap[t1],
								s_infoMap[t2] );
		Engine::getBBTrackContainer()->setCurrentBB( s_infoMap[t1] );
	}
}









BBTrackView::BBTrackView( BBTrack * _bbt, TrackContainerView* tcv ) :
	TrackView( _bbt, tcv ),
	m_bbTrack( _bbt )
{
	setFixedHeight( 32 );
	// drag'n'drop with bb-tracks only causes troubles (and makes no sense
	// too), so disable it
	setAcceptDrops( false );

	m_trackLabel = new TrackLabelButton( this, getTrackSettingsWidget() );
	m_trackLabel->setIcon( embed::getIconPixmap( "bb_track" ) );
	m_trackLabel->move( 3, 1 );
	m_trackLabel->show();
	connect( m_trackLabel, SIGNAL( clicked( bool ) ),
			this, SLOT( clickedTrackLabel() ) );
	setModel( _bbt );
}




BBTrackView::~BBTrackView()
{
	gui->getBBEditor()->removeBBView( BBTrack::s_infoMap[m_bbTrack] );
}




bool BBTrackView::close()
{
	gui->getBBEditor()->removeBBView( BBTrack::s_infoMap[m_bbTrack] );
	return TrackView::close();
}




void BBTrackView::clickedTrackLabel()
{
	Engine::getBBTrackContainer()->setCurrentBB( m_bbTrack->index() );
	gui->getBBEditor()->parentWidget()->show();
	gui->getBBEditor()->setFocus( Qt::ActiveWindowFocusReason );
}
