/*
 * bb_editor.cpp - basic main-window for editing of beats and basslines
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


#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QMdiArea>

#include "bb_editor.h"
#include "bb_track_container.h"
#include "embed.h"
#include "MainWindow.h"
#include "song.h"
#include "tool_button.h"
#include "config_mgr.h"
#include "DataFile.h"
#include "string_pair_drag.h"

#include "TrackContainer.h"
#include "Pattern.h"



bbEditor::bbEditor( bbTrackContainer* tc ) :
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
	if( configManager::inst()->value( "ui",
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


	m_playButton = new toolButton( embed::getIconPixmap( "play" ),
			tr( "Play/pause current beat/bassline (Space)" ),
					this, SLOT( play() ), m_toolBar );

	m_stopButton = new toolButton( embed::getIconPixmap( "stop" ),
			tr( "Stop playback of current beat/bassline (Space)" ),
					this, SLOT( stop() ), m_toolBar );

	m_playButton->setObjectName( "playButton" );
	m_stopButton->setObjectName( "stopButton" );

	toolButton * add_bb_track = new toolButton(
					embed::getIconPixmap( "add_bb_track" ),
						tr( "Add beat/bassline" ),
				engine::getSong(), SLOT( addBBTrack() ),
								m_toolBar );

	toolButton * add_automation_track = new toolButton(
				embed::getIconPixmap( "add_automation" ),
						tr( "Add automation-track" ),
				this, SLOT( addAutomationTrack() ), m_toolBar );

	toolButton * remove_bar = new toolButton(
				embed::getIconPixmap( "step_btn_remove" ),
						tr( "Remove steps" ),
				this, SLOT( removeSteps() ), m_toolBar );

	toolButton * add_bar = new toolButton(
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

	m_bbComboBox = new comboBox( m_toolBar );
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

	engine::mainWindow()->workspace()->addSubWindow( this );
	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->layout()->setSizeConstraint( QLayout::SetMinimumSize );
	parentWidget()->resize( minimumWidth(), 300 );
	parentWidget()->move( 610, 5 );
	parentWidget()->show();


	setModel( tc );
	connect( &tc->m_bbComboBoxModel, SIGNAL( dataChanged() ),
			this, SLOT( updatePosition() ) );
}




bbEditor::~bbEditor()
{
}


void bbEditor::dropEvent( QDropEvent * de )
{
	QString type = stringPairDrag::decodeKey( de );
	QString value = stringPairDrag::decodeValue( de );
	
	if( type.left( 6 ) == "track_" )
	{
		DataFile dataFile( value.toUtf8() );
		track * t = track::create( dataFile.content().firstChild().toElement(), model() );
		
		t->deleteTCOs();
		m_bbtc->updateAfterTrackAdd();
		
		de->accept();
	}
	else
	{
		TrackContainerView::dropEvent( de );
	}
}


void bbEditor::removeBBView( int _bb )
{
	foreach( trackView* view, trackViews() )
	{
		view->getTrackContentWidget()->removeTCOView( _bb );
	}
}




void bbEditor::setPauseIcon( bool pause )
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




void bbEditor::play()
{
	if( engine::getSong()->playMode() != song::Mode_PlayBB )
	{
		engine::getSong()->playBB();
	}
	else
	{
		engine::getSong()->togglePause();
	}
}




void bbEditor::stop()
{
	engine::getSong()->stop();
}




void bbEditor::updatePosition()
{
	//realignTracks();
	emit positionChanged( m_currentPosition );
}




void bbEditor::addAutomationTrack()
{
	(void) track::create( track::AutomationTrack, model() );
}




void bbEditor::addSteps()
{
	TrackContainer::TrackList tl = model()->tracks();

	for( TrackContainer::TrackList::iterator it = tl.begin();
		it != tl.end(); ++it )
	{
		if( ( *it )->type() == track::InstrumentTrack )
		{
			Pattern* p = static_cast<Pattern *>( ( *it )->getTCO( m_bbtc->currentBB() ) );
			p->addSteps();
		}
	}
}




void bbEditor::removeSteps()
{
	TrackContainer::TrackList tl = model()->tracks();

	for( TrackContainer::TrackList::iterator it = tl.begin();
		it != tl.end(); ++it )
	{
		if( ( *it )->type() == track::InstrumentTrack )
		{
			Pattern* p = static_cast<Pattern *>( ( *it )->getTCO( m_bbtc->currentBB() ) );
			p->removeSteps();
		}
	}
}




void bbEditor::keyPressEvent( QKeyEvent * _ke )
{
	if ( _ke->key() == Qt::Key_Space )
	{
		if( engine::getSong()->isPlaying() )
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




#include "moc_bb_editor.cxx"

