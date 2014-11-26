/*
 * BBEditor.cpp - basic main-window for editing of beats and basslines
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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


#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMdiArea>

#include "BBEditor.h"
#include "BBTrackContainer.h"
#include "embed.h"
#include "MainWindow.h"
#include "Song.h"
#include "ToolButton.h"
#include "ConfigManager.h"
#include "DataFile.h"
#include "StringPairDrag.h"

#include "TrackContainer.h"
#include "Pattern.h"



BBEditor::BBEditor( BBTrackContainer* tc ) :
	TrackContainerView( tc ),
	m_bbtc( tc )
{
	// create toolbar
	m_toolBar = new QWidget;
	m_toolBar->setFixedHeight( 32 );
	m_toolBar->move( 0, 0 );
	m_toolBar->setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( m_toolBar->backgroundRole(),
					embed::getIconPixmap( "toolbar_bg" ) );
	m_toolBar->setPalette( pal );
	static_cast<QVBoxLayout *>( layout() )->insertWidget( 0, m_toolBar );

	QHBoxLayout * tb_layout = new QHBoxLayout( m_toolBar );
	tb_layout->setSpacing( 0 );
	tb_layout->setMargin( 0 );


	setWindowIcon( embed::getIconPixmap( "bb_track_btn" ) );
	setWindowTitle( tr( "Beat+Bassline Editor" ) );
	// TODO: Use style sheet
	if( ConfigManager::inst()->value( "ui",
					  "compacttrackbuttons" ).toInt() )
	{
		setMinimumWidth( TRACK_OP_WIDTH_COMPACT + DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT
			     + 2 * TCO_BORDER_WIDTH + 264 );
	}
	else
	{
		setMinimumWidth( TRACK_OP_WIDTH + DEFAULT_SETTINGS_WIDGET_WIDTH
			     + 2 * TCO_BORDER_WIDTH + 264 );
	}


	m_playButton = new ToolButton( embed::getIconPixmap( "play" ),
			tr( "Play/pause current beat/bassline (Space)" ),
					this, SLOT( play() ), m_toolBar );

	m_stopButton = new ToolButton( embed::getIconPixmap( "stop" ),
			tr( "Stop playback of current beat/bassline (Space)" ),
					this, SLOT( stop() ), m_toolBar );

	m_playButton->setObjectName( "playButton" );
	m_stopButton->setObjectName( "stopButton" );

	ToolButton * add_bb_track = new ToolButton(
					embed::getIconPixmap( "add_bb_track" ),
						tr( "Add beat/bassline" ),
				Engine::getSong(), SLOT( addBBTrack() ),
								m_toolBar );

	ToolButton * add_automation_track = new ToolButton(
				embed::getIconPixmap( "add_automation" ),
						tr( "Add automation-track" ),
				this, SLOT( addAutomationTrack() ), m_toolBar );

	ToolButton * remove_bar = new ToolButton(
				embed::getIconPixmap( "step_btn_remove" ),
						tr( "Remove steps" ),
				this, SLOT( removeSteps() ), m_toolBar );

	ToolButton * add_bar = new ToolButton(
				embed::getIconPixmap( "step_btn_add" ),
						tr( "Add steps" ),
				this, SLOT( addSteps() ), m_toolBar );



	m_playButton->setWhatsThis(
		tr( "Click here to play the current "
			"beat/bassline.  The beat/bassline is automatically "
			"looped when its end is reached." ) );
	m_stopButton->setWhatsThis(
		tr( "Click here to stop playing of current "
							"beat/bassline." ) );

	m_bbComboBox = new ComboBox( m_toolBar );
	m_bbComboBox->setFixedSize( 200, 22 );
	m_bbComboBox->setModel( &tc->m_bbComboBoxModel );

	tb_layout->addSpacing( 5 );
	tb_layout->addWidget( m_playButton );
	tb_layout->addWidget( m_stopButton );
	tb_layout->addSpacing( 20 );
	tb_layout->addWidget( m_bbComboBox );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( add_bb_track );
	tb_layout->addWidget( add_automation_track );
	tb_layout->addStretch();
	tb_layout->addWidget( remove_bar );
	tb_layout->addWidget( add_bar );
	tb_layout->addSpacing( 15 );

	Engine::mainWindow()->workspace()->addSubWindow( this );
	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->layout()->setSizeConstraint( QLayout::SetMinimumSize );
	parentWidget()->resize( minimumWidth(), 300 );
	parentWidget()->move( 610, 5 );
	parentWidget()->show();


	setModel( tc );
	connect( &tc->m_bbComboBoxModel, SIGNAL( dataChanged() ),
			this, SLOT( updatePosition() ) );
}




BBEditor::~BBEditor()
{
}


void BBEditor::dropEvent( QDropEvent * de )
{
	QString type = StringPairDrag::decodeKey( de );
	QString value = StringPairDrag::decodeValue( de );
	
	if( type.left( 6 ) == "track_" )
	{
		DataFile dataFile( value.toUtf8() );
		Track * t = Track::create( dataFile.content().firstChild().toElement(), model() );
		
		t->deleteTCOs();
		m_bbtc->updateAfterTrackAdd();
		
		de->accept();
	}
	else
	{
		TrackContainerView::dropEvent( de );
	}
}


void BBEditor::removeBBView( int _bb )
{
	foreach( TrackView* view, trackViews() )
	{
		view->getTrackContentWidget()->removeTCOView( _bb );
	}
}




void BBEditor::setPauseIcon( bool pause )
{
	if( pause == true )
	{
		m_playButton->setIcon( embed::getIconPixmap( "pause" ) );
	}
	else
	{
		m_playButton->setIcon( embed::getIconPixmap( "play" ) );
	}
}




void BBEditor::play()
{
	if( Engine::getSong()->playMode() != Song::Mode_PlayBB )
	{
		Engine::getSong()->playBB();
	}
	else
	{
		Engine::getSong()->togglePause();
	}
}




void BBEditor::stop()
{
	Engine::getSong()->stop();
}




void BBEditor::updatePosition()
{
	//realignTracks();
	emit positionChanged( m_currentPosition );
}




void BBEditor::addAutomationTrack()
{
	(void) Track::create( Track::AutomationTrack, model() );
}




void BBEditor::addSteps()
{
	TrackList tl = model()->tracks();

	for( TrackList::iterator it = tl.begin();
		it != tl.end(); ++it )
	{
		if( ( *it )->type() == Track::InstrumentTrack )
		{
			Pattern* p = static_cast<Pattern *>( ( *it )->getTCO( m_bbtc->currentBB() ) );
			p->addSteps();
		}
	}
}




void BBEditor::removeSteps()
{
	TrackList tl = model()->tracks();

	for( TrackList::iterator it = tl.begin();
		it != tl.end(); ++it )
	{
		if( ( *it )->type() == Track::InstrumentTrack )
		{
			Pattern* p = static_cast<Pattern *>( ( *it )->getTCO( m_bbtc->currentBB() ) );
			p->removeSteps();
		}
	}
}




void BBEditor::keyPressEvent( QKeyEvent * _ke )
{
	if ( _ke->key() == Qt::Key_Space )
	{
		if( Engine::getSong()->isPlaying() )
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






