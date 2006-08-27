#ifndef SINGLE_SOURCE_COMPILE

/*
 * bb_editor.cpp - basic main-window for editing of beats and basslines
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

#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>
#include <QtGui/QCloseEvent>
#include <QtGui/QLayout>

#else

#include <qpainter.h>
#include <qlayout.h>
#include <qwhatsthis.h>

#endif


#include "bb_editor.h"
#include "song_editor.h"
#include "embed.h"
#include "tool_button.h"
#include "track_container.h"
#include "bb_track.h"
#include "name_label.h"
#include "templates.h"
#include "debug.h"
#include "tooltip.h"
#include "combobox.h"
#include "main_window.h"


const int BBE_PPT = 192;




bbEditor::bbEditor( engine * _engine ) :
	trackContainer( _engine )
{
	// create toolbar
	m_toolBar = new QWidget( this );
	m_toolBar->setFixedHeight( 32 );
	m_toolBar->move( 0, 0 );
#ifdef QT4
	m_toolBar->setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( m_toolBar->backgroundRole(),
					embed::getIconPixmap( "toolbar_bg" ) );
	m_toolBar->setPalette( pal );
#else
	m_toolBar->setPaletteBackgroundPixmap( embed::getIconPixmap(
							"toolbar_bg" ) );
#endif

	QHBoxLayout * tb_layout = new QHBoxLayout( m_toolBar );
	tb_layout->setMargin( 0 );


	setWindowIcon( embed::getIconPixmap( "bb_track" ) );
	setWindowTitle( tr( "Beat+Baseline Editor" ) );
	setMinimumWidth( TRACK_OP_WIDTH + DEFAULT_SETTINGS_WIDGET_WIDTH +
				BBE_PPT + 2 * TCO_BORDER_WIDTH +
				DEFAULT_SCROLLBAR_SIZE );

	QWidget * w = ( parentWidget() != NULL ) ? parentWidget() : this;
	if( eng()->getMainWindow()->workspace() != NULL )
	{
		resize( minimumWidth(), 300 );
		w->move( 10, 340 );
	}
	else
	{
		resize( minimumWidth(), 300 );
		w->move( 210, 340 );
	}

	containerWidget()->move( 0, 32 );
	setPixelsPerTact( BBE_PPT );


	m_playButton = new toolButton( embed::getIconPixmap( "play" ),
			tr( "Play/pause current beat/bassline (Space)" ),
					this, SLOT( play() ), m_toolBar );

	m_stopButton = new toolButton( embed::getIconPixmap( "stop" ),
			tr( "Stop playing of current beat/bassline (Space)" ),
					this, SLOT( stop() ), m_toolBar );

	toolButton * add_bb_track = new toolButton(
					embed::getIconPixmap( "add_bb_track" ),
						tr( "Add beat/bassline" ),
				eng()->getSongEditor(), SLOT( addBBTrack() ),
								m_toolBar );


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

	QLabel * l = new QLabel( m_toolBar );
	l->setPixmap( embed::getIconPixmap( "drum" ) );

	m_bbComboBox = new comboBox( m_toolBar, NULL, eng(), NULL );
	m_bbComboBox->setFixedSize( 200, 22 );
	connect( m_bbComboBox, SIGNAL( valueChanged( int ) ),
				this, SLOT( setCurrentBB( int ) ) );

	tb_layout->addSpacing( 5 );
	tb_layout->addWidget( m_playButton );
	tb_layout->addWidget( m_stopButton );
	tb_layout->addSpacing( 20 );
	tb_layout->addWidget( m_bbComboBox );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( add_bb_track );
	tb_layout->addStretch();
	tb_layout->addWidget( l );
	tb_layout->addSpacing( 15 );

	show();
}




bbEditor::~bbEditor()
{
}




csize bbEditor::currentBB( void ) const
{
	return( static_cast<csize>( currentPosition().getTact() ) );
}




void bbEditor::setCurrentBB( int _bb )
{
	if( m_bbComboBox->value() != _bb )
	{
		m_bbComboBox->setValue( _bb );
	}

	// first make sure, all channels have a TCO at current BB
	createTCOsForBB( static_cast<csize>( _bb ) );

	realignTracks();

	// now update all track-labels (the current one has to become white,
	// the others green)
	for( csize i = 0; i < numOfBBs(); ++i )
	{
		bbTrack::findBBTrack( i, eng() )->trackLabel()->update();
	}

	emit positionChanged( m_currentPosition = midiTime(
					static_cast<csize>( _bb ), 0 ) );
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




bool FASTCALL bbEditor::play( midiTime _start, fpab_t _frames,
							f_cnt_t _frame_base,
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
		if( ( *it )->play( _start, _frames, _frame_base,
							_tco_num ) == TRUE )
		{
			played_a_note = TRUE;
		}
	}

	return( played_a_note );
}




csize bbEditor::numOfBBs( void ) const
{
	return( eng()->getSongEditor()->countTracks( track::BB_TRACK ) );
}




void bbEditor::removeBB( csize _bb )
{
	trackVector tv = tracks();
	for( trackVector::iterator it = tv.begin(); it != tv.end(); ++it )
	{
		( *it )->removeTCO( _bb );
		( *it )->getTrackContentWidget()->removeTact( _bb * 64 );
	}
	if( _bb <= currentBB() )
	{
		setCurrentBB( tMax( (int)currentBB() - 1, 0 ) );
	}
}



void bbEditor::updateBBTrack( trackContentObject * _tco )
{
	bbTrack * t = bbTrack::findBBTrack( _tco->startPosition() / 64, eng() );
	if( t != NULL )
	{
		t->getTrackContentWidget()->updateTCOs();
	}
}




void bbEditor::updateComboBox( void )
{
	disconnect( m_bbComboBox, SIGNAL( valueChanged( int ) ),
					this, SLOT( setCurrentBB( int ) ) );

	csize current_bb = currentBB();

	m_bbComboBox->clear();

	for( csize i = 0; i < numOfBBs(); ++i )
	{
		bbTrack * bbt = bbTrack::findBBTrack( i, eng() );
		m_bbComboBox->addItem( bbt->trackLabel()->text(),
					bbt->trackLabel()->pixmap() );
	}
	m_bbComboBox->setValue( current_bb );

	connect( m_bbComboBox, SIGNAL( valueChanged( int ) ),
					this, SLOT( setCurrentBB( int ) ) );
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
		if( eng()->getSongEditor()->playing() )
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
	setPixelsPerTact( width() - ( TRACK_OP_WIDTH +
					DEFAULT_SETTINGS_WIDGET_WIDTH + 2 *
					TCO_BORDER_WIDTH +
					DEFAULT_SCROLLBAR_SIZE ) );
	trackContainer::resizeEvent( _re );
	m_toolBar->setFixedWidth( width() );
}




QRect bbEditor::scrollAreaRect( void ) const
{
	return( QRect( 0, 0, (int) pixelsPerTact(),
					height() - m_toolBar->height() ) );
}




void bbEditor::play( void )
{
	if( eng()->getSongEditor()->playing() )
	{
		if( eng()->getSongEditor()->playMode() != songEditor::PLAY_BB )
		{
			eng()->getSongEditor()->stop();
			eng()->getSongEditor()->playBB();
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
		}
		else
		{
			eng()->getSongEditor()->pause();
			m_playButton->setIcon( embed::getIconPixmap(
								"play" ) );
		}
	}
	else if( eng()->getSongEditor()->paused() )
	{
		eng()->getSongEditor()->resumeFromPause();
		m_playButton->setIcon( embed::getIconPixmap( "pause" ) );
	}
	else
	{
		m_playButton->setIcon( embed::getIconPixmap( "pause" ) );
		eng()->getSongEditor()->playBB();
	}

}




void bbEditor::stop( void )
{
	eng()->getSongEditor()->stop();
	m_playButton->setIcon( embed::getIconPixmap( "play" ) );
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
	updateComboBox();
}



#include "bb_editor.moc"


#endif
