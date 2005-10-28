/*
 * bb_editor.cpp - basic main-window for editing of beats and basslines
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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

#include <QPainter>
#include <QKeyEvent>
#include <QCloseEvent>

#else

#include <qpainter.h>

#endif


#include "bb_editor.h"
#include "song_editor.h"
#include "embed.h"
#include "pixmap_button.h"
#include "track_container.h"
#include "bb_track.h"
#include "name_label.h"
#include "templates.h"
#include "debug.h"
#include "spc_bg_hndl_widget.h"
#include "tooltip.h"



const int BBE_PPT = 192;



bbEditor * bbEditor::s_instanceOfMe = NULL;
QPixmap * bbEditor::s_titleArtwork = NULL;



bbEditor::bbEditor() :
	trackContainer()
{
	if( s_titleArtwork == NULL )
	{
		s_titleArtwork = new QPixmap( embed::getIconPixmap(
						"bb_editor_title_artwork" ) );
	}

	setWindowIcon( embed::getIconPixmap( "bb_track" ) );
	setWindowTitle( tr( "Beat+Bassline Editor" ) );
	setMinimumWidth( TRACK_OP_WIDTH + DEFAULT_SETTINGS_WIDGET_WIDTH +
					BBE_PPT + 2 * TCO_BORDER_WIDTH );
	if( lmmsMainWin::inst()->workspace() != NULL )
	{
		setGeometry( 10, 340, minimumWidth(), 300 );
	}
	else
	{
		setGeometry( 210, 340, minimumWidth(), 300 );
	}

	containerWidget()->move( 0, 47 );
	setPixelsPerTact( BBE_PPT );
	updateBackground();

	m_playButton = new pixmapButton( this );
	m_playButton->move( 96, 7 );
	m_playButton->setCheckable( FALSE );
	m_playButton->setActiveGraphic( embed::getIconPixmap( "play" ) );
	m_playButton->setInactiveGraphic( embed::getIconPixmap( "play" ) );
	m_playButton->setBgGraphic( specialBgHandlingWidget::getBackground(
							m_playButton ) );
	connect( m_playButton, SIGNAL( clicked() ), this, SLOT( play() ) );

	m_stopButton = new pixmapButton( this );
	m_stopButton->move( 136, 7 );
	m_stopButton->setCheckable( FALSE );
	m_stopButton->setActiveGraphic( embed::getIconPixmap( "stop" ) );
	m_stopButton->setInactiveGraphic( embed::getIconPixmap( "stop" ) );
	m_stopButton->setBgGraphic( specialBgHandlingWidget::getBackground(
							m_playButton ) );
	connect( m_stopButton, SIGNAL( clicked() ), this, SLOT( stop() ) );


	toolTip::add( m_playButton,
			tr( "Play/pause current beat/bassline (Space)" ) );
	toolTip::add( m_stopButton,
			tr( "Stop playing of current beat/bassline (Space)" ) );
#ifdef QT4
	m_playButton->setWhatsThis(
#else
	QWhatsThis::add( m_playButton,
#endif
		tr( "Click here, if you want to play the current "
			"beat/bassline. The beat/bassline is automatically "
			"looped when its end is reached." ) );
#ifdef QT4
	m_stopButton->setWhatsThis(
#else
	QWhatsThis::add( m_stopButton,
#endif
		tr( "Click here, if you want to stop playing of current "
							"beat/bassline." ) );

#ifndef QT4
	setBackgroundMode( Qt::NoBackground );
#endif

	show();
}




bbEditor::~bbEditor()
{
}




csize bbEditor::currentBB( void ) const
{
	return( static_cast<csize>( currentPosition().getTact() ) );
}




void bbEditor::setCurrentBB( csize _bb )
{
	// first make sure, all channels have a TCO at current BB
	createTCOsForBB( _bb );

	realignTracks();

	// now update all track-labels (the current one has to become white,
	// the others green)
	for( csize i = 0; i < bbEditor::inst()->numOfBBs(); ++i )
	{
		bbTrack::findBBTrack( i )->trackLabel()->update();
	}

	emit positionChanged( m_currentPosition = midiTime( _bb, 0 ) );
}




tact bbEditor::lengthOfBB( csize _bb )
{
	midiTime max_length;
	
	trackVector tv = tracks();
	for( trackVector::iterator it = tv.begin(); it != tv.end(); ++it )
	{
		trackContentObject * tco = ( *it )->getTCO( _bb );
#ifdef LMMS_DEBUG
		assert( tco != NULL );
#endif
		max_length = tMax( max_length, tco->length() );
	}
	if( max_length.getTact64th() == 0 )
	{
		return( max_length.getTact() );
	}

	return( max_length.getTact() + 1 );
}




bool FASTCALL bbEditor::play( midiTime _start, Uint32 _start_frame,
				Uint32 _frames, Uint32 _frame_base,
							Sint16 _tco_num )
{
	bool played_a_note = FALSE;
	if( lengthOfBB( _tco_num ) <= 0 )
	{
		return( played_a_note );
	}

	_start = ( _start.getTact() % lengthOfBB( _tco_num ) ) * 64 +
							_start.getTact64th();
	trackVector tv = tracks();
	for( trackVector::iterator it = tv.begin(); it != tv.end(); ++it )
	{
		if( ( *it )->play( _start, _start_frame, _frames, _frame_base,
							_tco_num ) == TRUE )
		{
			played_a_note = TRUE;
		}
	}

	return( played_a_note );
}




csize bbEditor::numOfBBs( void ) const
{
	return( songEditor::inst()->countTracks( track::BB_TRACK ) );
}




void bbEditor::removeBB( csize _bb )
{
	trackVector tv = tracks();
	for( trackVector::iterator it = tv.begin(); it != tv.end(); ++it )
	{
		( *it )->removeTCO( _bb );
		( *it )->getTrackContentWidget()->removeTact( _bb * 64 );
	}
/*	if( _bb == currentBB() && numOfBBs() > 0 )
	{*/
		if( _bb > 0)
		{
			setCurrentBB( _bb - 1 );
		}
		else
		{
			setCurrentBB( 0 );
		}
//	}
}



// close-handler for bb-editor-window because closing of bb-editor isn't allowed
// instead of closing it's being hidden
void bbEditor::closeEvent( QCloseEvent * _ce )
{
	_ce->ignore();
	hide();
}




void bbEditor::keyPressEvent( QKeyEvent * _ke )
{
	if ( _ke->key() == Qt::Key_Space )
	{
		if( songEditor::inst()->playing() )
		{
			stop();
		}
		else
		{
			play();
		}
	}
	else if ( _ke->key() == Qt::Key_Plus )
	{
		if( currentBB() + 1 < numOfBBs() )
		{
			setCurrentBB( currentBB() + 1 );
		}
	}
	else if ( _ke->key() == Qt::Key_Minus )
	{
		if( currentBB() > 0 )
		{
			setCurrentBB( currentBB() - 1 );
		}
	}
	else
	{
		// ignore event and pass to parent-widget
		_ke->ignore();
	}

}




void bbEditor::resizeEvent( QResizeEvent * _re )
{
	updateBackground();
	setPixelsPerTact( width() - ( TRACK_OP_WIDTH +
					DEFAULT_SETTINGS_WIDGET_WIDTH + 2 *
					TCO_BORDER_WIDTH ) );
	trackContainer::resizeEvent( _re );
}




void bbEditor::updateBackground( void )
{
	QPixmap draw_pm( size() );
#ifdef QT4
	draw_pm.fill( containerWidget()->palette().brush(
				containerWidget()->backgroundRole() ).color() );
#else
	draw_pm.fill( containerWidget()->paletteBackgroundColor() );
#endif

	QPainter p( &draw_pm );

	p.fillRect( 0, 0, width(), s_titleArtwork->height(),
						QColor( 74, 125, 213 ) );
	p.drawPixmap( 0, 0, *s_titleArtwork );

#ifdef QT4
	QPalette pal = palette();
	pal.setBrush( backgroundRole(), QBrush( draw_pm ) );
	setPalette( pal );
#else
	setErasePixmap( draw_pm );
#endif
}




void bbEditor::play( void )
{
	if( songEditor::inst()->playing() )
	{
		if( songEditor::inst()->playMode() != songEditor::PLAY_BB )
		{
			songEditor::inst()->stop();
			songEditor::inst()->playBB();
			m_playButton->setInactiveGraphic(
					embed::getIconPixmap( "pause" ) );
		}
		else
		{
			songEditor::inst()->pause();
			m_playButton->setInactiveGraphic(
					embed::getIconPixmap( "play" ) );
		}
	}
	else if( songEditor::inst()->paused() )
	{
		songEditor::inst()->resumeFromPause();
		m_playButton->setInactiveGraphic(
					embed::getIconPixmap( "pause" ) );
	}
	else
	{
		m_playButton->setInactiveGraphic(
					embed::getIconPixmap( "pause" ) );
		songEditor::inst()->playBB();
	}

}




void bbEditor::stop( void )
{
	songEditor::inst()->stop();
	m_playButton->setInactiveGraphic( embed::getIconPixmap( "play" ) );
	m_playButton->update();
}






void bbEditor::saveSettings( QDomDocument & _doc, QDomElement & _parent )
{
	trackContainer::saveSettings( _doc, _parent );
}




void bbEditor::loadSettings( const QDomElement & _this )
{
	trackContainer::loadSettings( _this );
}




void bbEditor::updateAfterTrackAdd( void )
{
	// make sure, new track(s) have TCOs for every beat/bassline
	for( csize i = 0; i < tMax<csize>( 1, numOfBBs() ); ++i )
	{
		createTCOsForBB( i );
	}
}




void bbEditor::createTCOsForBB( csize _bb )
{
	trackVector tv = tracks();
	for( trackVector::iterator it = tv.begin(); it != tv.end(); ++it )
	{
		while( ( *it )->numOfTCOs() < _bb + 1 )
		{
			midiTime position = midiTime( ( *it )->numOfTCOs(), 0 );
			trackContentObject * tco = ( *it )->addTCO(
					( *it )->createTCO( position ) );
			tco->movePosition( position );
			tco->changeLength( midiTime( 1, 0 ) );
		}
	}
}




void bbEditor::swapBB( csize _bb1, csize _bb2 )
{
	trackVector tv = tracks();
	for( trackVector::iterator it = tv.begin(); it != tv.end(); ++it )
	{
		( *it )->swapPositionOfTCOs( _bb1, _bb2 );
	}
}



#include "bb_editor.moc"

