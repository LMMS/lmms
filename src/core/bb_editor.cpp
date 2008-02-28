#ifndef SINGLE_SOURCE_COMPILE

/*
 * bb_editor.cpp - basic main-window for editing of beats and basslines
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


#include "bb_editor.h"


#include <QtGui/QKeyEvent>
#include <QtGui/QLayout>
#include <QtGui/QMdiArea>
#include <QtGui/QPainter>


#include "bb_track.h"
#include "combobox.h"
#include "embed.h"
#include "engine.h"
#include "main_window.h"
#include "name_label.h"
#include "song.h"
#include "templates.h"
#include "tool_button.h"
#include "tooltip.h"
#include "track_container.h"


bbTrackContainer::bbTrackContainer( void ) :
	trackContainer(),
	m_bbComboBoxModel( this )
{
	connect( &m_bbComboBoxModel, SIGNAL( dataChanged() ),
			this, SLOT( currentBBChanged() ),
			Qt::QueuedConnection );
	// we *always* want to receive updates even in case BB actually did
	// not change upon setCurrentBB()-call
	connect( &m_bbComboBoxModel, SIGNAL( dataUnchanged() ),
			this, SLOT( currentBBChanged() ),
			Qt::QueuedConnection );
}




bbTrackContainer::~bbTrackContainer()
{
}




bool bbTrackContainer::play( midiTime _start, fpp_t _frames,
							f_cnt_t _offset,
							Sint16 _tco_num )
{
	bool played_a_note = FALSE;
	if( lengthOfBB( _tco_num ) <= 0 )
	{
		return( played_a_note );
	}

	_start = ( _start.getTact() % lengthOfBB( _tco_num ) ) * 64 +
							_start.getTact64th();
	QList<track *> tl = tracks();
	for( int i = 0; i < tl.size(); ++i )
	{
		if( tl[i]->play( _start, _frames, _offset, _tco_num ) == TRUE )
		{
			played_a_note = TRUE;
		}
	}

	return( played_a_note );
}



/*
void bbTrackContainer::saveSettings( QDomDocument & _doc, QDomElement & _parent )
{
	trackContainer::saveSettings( _doc, _parent );
}




void bbTrackContainer::loadSettings( const QDomElement & _this )
{
	trackContainer::loadSettings( _this );
}
*/



void bbTrackContainer::updateAfterTrackAdd( void )
{
	// make sure, new track(s) have TCOs for every beat/bassline
	for( int i = 0; i < tMax<int>( 1, numOfBBs() ); ++i )
	{
		createTCOsForBB( i );
	}
}




tact bbTrackContainer::lengthOfBB( int _bb )
{
	midiTime max_length;

	QList<track *> tl = tracks();
	for( int i = 0; i < tl.size(); ++i )
	{
		trackContentObject * tco = tl[i]->getTCO( _bb );
		max_length = tMax( max_length, tco->length() );
	}
	if( max_length.getTact64th() == 0 )
	{
		return( max_length.getTact() );
	}

	return( max_length.getTact() + 1 );
}




int bbTrackContainer::numOfBBs( void ) const
{
	return( engine::getSong()->countTracks( track::BBTrack ) );
}




void bbTrackContainer::removeBB( int _bb )
{
	QList<track *> tl = tracks();
	for( int i = 0; i < tl.size(); ++i )
	{
		delete tl[i]->getTCO( _bb );
		tl[i]->removeTact( _bb * 64 );
	}
	if( _bb <= currentBB() )
	{
		setCurrentBB( tMax( currentBB() - 1, 0 ) );
	}
}




void bbTrackContainer::swapBB( int _bb1, int _bb2 )
{
	QList<track *> tl = tracks();
	for( int i = 0; i < tl.size(); ++i )
	{
		tl[i]->swapPositionOfTCOs( _bb1, _bb2 );
	}
	updateComboBox();
}




void bbTrackContainer::updateBBTrack( trackContentObject * _tco )
{
	bbTrack * t = bbTrack::findBBTrack( _tco->startPosition() / 64 );
	if( t != NULL )
	{
		t->dataChanged();
		//t->getTrackContentWidget()->update();
	}
}




void bbTrackContainer::play( void )
{
	if( engine::getSong()->playing() )
	{
		if( engine::getSong()->playMode() != song::Mode_PlayBB )
		{
			engine::getSong()->stop();
			engine::getSong()->playBB();
		}
		else
		{
			engine::getSong()->pause();
		}
	}
	else if( engine::getSong()->paused() )
	{
		engine::getSong()->resumeFromPause();
	}
	else
	{
		engine::getSong()->playBB();
	}

}




void bbTrackContainer::stop( void )
{
	engine::getSong()->stop();
}




void bbTrackContainer::updateComboBox( void )
{
	const int cur_bb = currentBB();

	m_bbComboBoxModel.clear();

	for( int i = 0; i < numOfBBs(); ++i )
	{
		bbTrack * bbt = bbTrack::findBBTrack( i );
		m_bbComboBoxModel.addItem( bbt->name(),
				bbt->pixmap() ? new QPixmap( *bbt->pixmap() )
								: NULL );
	}
	setCurrentBB( cur_bb );
}




void bbTrackContainer::currentBBChanged( void )
{
	// first make sure, all channels have a TCO at current BB
	createTCOsForBB( currentBB() );

	// now update all track-labels (the current one has to become white,
	// the others green)
	for( int i = 0; i < numOfBBs(); ++i )
	{
		bbTrack::findBBTrack( i )->dataChanged();
//trackLabel()->update();
	}

	//emit dataChanged();
	//emit positionChanged( NULL );
}




void bbTrackContainer::createTCOsForBB( int _bb )
{
	if( numOfBBs() == 0 || engine::getSong()->isLoadingProject() )
	{
		return;
	}

	QList<track *> tl = tracks();
	for( int i = 0; i < tl.size(); ++i )
	{
		while( tl[i]->numOfTCOs() < _bb + 1 )
		{
			midiTime position = midiTime( tl[i]->numOfTCOs(), 0 );
			trackContentObject * tco = tl[i]->addTCO(
						tl[i]->createTCO( position ) );
			tco->movePosition( position );
			tco->changeLength( midiTime( 1, 0 ) );
		}
	}
}


















bbEditor::bbEditor( bbTrackContainer * _tc ) :
	trackContainerView( _tc ),
	m_bbtc( _tc )
{
	// create toolbar
	m_toolBar = new QWidget;
	m_toolBar->setFixedHeight( 32 );
	m_toolBar->move( 0, 0 );
	m_toolBar->setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( m_toolBar->backgroundRole(),
					embed::getIconPixmap( "toolbar_bg" ) );
	m_toolBar->setPalette( pal );
	static_cast<QVBoxLayout *>( layout() )->insertWidget( 0, m_toolBar );

	QHBoxLayout * tb_layout = new QHBoxLayout( m_toolBar );
	tb_layout->setMargin( 0 );


	setWindowIcon( embed::getIconPixmap( "bb_track" ) );
	setWindowTitle( tr( "Beat+Baseline Editor" ) );
	// TODO: Use style sheet
	setMinimumWidth( TRACK_OP_WIDTH + DEFAULT_SETTINGS_WIDGET_WIDTH
						+ 2 * TCO_BORDER_WIDTH + 192 );


	m_playButton = new toolButton( embed::getIconPixmap( "play" ),
			tr( "Play/pause current beat/bassline (Space)" ),
					this, SLOT( play() ), m_toolBar );

	m_stopButton = new toolButton( embed::getIconPixmap( "stop" ),
			tr( "Stop playing of current beat/bassline (Space)" ),
					this, SLOT( stop() ), m_toolBar );

	toolButton * add_bb_track = new toolButton(
					embed::getIconPixmap( "add_bb_track" ),
						tr( "Add beat/bassline" ),
				engine::getSong(), SLOT( addBBTrack() ),
								m_toolBar );


	m_playButton->setWhatsThis(
		tr( "Click here, if you want to play the current "
			"beat/bassline. The beat/bassline is automatically "
			"looped when its end is reached." ) );
	m_stopButton->setWhatsThis(
		tr( "Click here, if you want to stop playing of current "
							"beat/bassline." ) );

	QLabel * l = new QLabel( m_toolBar );
	l->setPixmap( embed::getIconPixmap( "drum" ) );

	m_bbComboBox = new comboBox( m_toolBar );
	m_bbComboBox->setFixedSize( 200, 22 );
	m_bbComboBox->setModel( &_tc->m_bbComboBoxModel );

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

	if( engine::getMainWindow()->workspace() != NULL )
	{
		engine::getMainWindow()->workspace()->addSubWindow( this );
		parentWidget()->setAttribute( Qt::WA_DeleteOnClose, FALSE );
		parentWidget()->layout()->setSizeConstraint(
						QLayout::SetMinimumSize );
	}

	QWidget * w = ( parentWidget() != NULL ) ? parentWidget() : this;
	if( engine::getMainWindow()->workspace() != NULL )
	{
		w->resize( minimumWidth(), 300 );
		w->move( 10, 340 );
	}
	else
	{
		resize( minimumWidth(), 300 );
		w->move( 210, 340 );
	}

	w->show();


	setModel( _tc );
	connect( &_tc->m_bbComboBoxModel, SIGNAL( dataChanged() ),
			this, SLOT( updatePosition() ),
			Qt::QueuedConnection );
}




bbEditor::~bbEditor()
{
}




void bbEditor::removeBBView( int _bb )
{
	QList<trackView *> tl = trackViews();
	for( int i = 0; i < tl.size(); ++i )
	{
		tl[i]->getTrackContentWidget()->removeTCOView( _bb );
	}
}




void bbEditor::play( void )
{
	if( engine::getSong()->playing() )
	{
		if( engine::getSong()->playMode() != song::Mode_PlayBB )
		{
			engine::getSong()->stop();
			engine::getSong()->playBB();
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
		}
		else
		{
			engine::getSong()->pause();
			m_playButton->setIcon( embed::getIconPixmap(
								"play" ) );
		}
	}
	else if( engine::getSong()->paused() )
	{
		engine::getSong()->resumeFromPause();
		m_playButton->setIcon( embed::getIconPixmap( "pause" ) );
	}
	else
	{
		m_playButton->setIcon( embed::getIconPixmap( "pause" ) );
		engine::getSong()->playBB();
	}

}




void bbEditor::stop( void )
{
	engine::getSong()->stop();
	m_playButton->setIcon( embed::getIconPixmap( "play" ) );
	m_playButton->update();
}




void bbEditor::updatePosition( void )
{
	//realignTracks();
	emit positionChanged( m_currentPosition );
}




void bbEditor::keyPressEvent( QKeyEvent * _ke )
{
	if ( _ke->key() == Qt::Key_Space )
	{
		if( engine::getSong()->playing() )
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
		if( m_bbtc->currentBB()+ 1 < m_bbtc->numOfBBs() )
		{
			m_bbtc->setCurrentBB( m_bbtc->currentBB() + 1 );
		}
	}
	else if ( _ke->key() == Qt::Key_Minus )
	{
		if( m_bbtc->currentBB() > 0 )
		{
			m_bbtc->setCurrentBB( m_bbtc->currentBB() - 1 );
		}
	}
	else
	{
		// ignore event and pass to parent-widget
		_ke->ignore();
	}

}




#include "bb_editor.moc"


#endif
