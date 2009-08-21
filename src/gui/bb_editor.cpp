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
	tb_layout->setSpacing( 0 );
	tb_layout->setMargin( 0 );


	setWindowIcon( embed::getIconPixmap( "bb_track" ) );
	setWindowTitle( tr( "Beat+Bassline Editor" ) );
	// TODO: Use style sheet
	setMinimumWidth( TRACK_OP_WIDTH + DEFAULT_SETTINGS_WIDGET_WIDTH
						+ 2 * TCO_BORDER_WIDTH + 192 );


	m_playButton = new toolButton( embed::getIconPixmap( "play" ),
			tr( "Play/pause current beat/bassline (Space)" ),
					this, SLOT( play() ), m_toolBar );

	m_stopButton = new toolButton( embed::getIconPixmap( "stop" ),
			tr( "Stop playback of current beat/bassline (Space)" ),
					this, SLOT( stop() ), m_toolBar );

	toolButton * add_bb_track = new toolButton(
					embed::getIconPixmap( "add_bb_track" ),
						tr( "Add beat/bassline" ),
				engine::getSong(), SLOT( addBBTrack() ),
								m_toolBar );

	toolButton * add_automation_track = new toolButton(
				embed::getIconPixmap( "add_automation" ),
						tr( "Add automation-track" ),
				this, SLOT( addAutomationTrack() ), m_toolBar );



	m_playButton->setWhatsThis(
		tr( "Click here to play the current "
			"beat/bassline.  The beat/bassline is automatically "
			"looped when its end is reached." ) );
	m_stopButton->setWhatsThis(
		tr( "Click here to stop playing of current "
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
	tb_layout->addWidget( add_automation_track );
	tb_layout->addStretch();
	tb_layout->addWidget( l );
	tb_layout->addSpacing( 15 );

	engine::mainWindow()->workspace()->addSubWindow( this );
	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, FALSE );
	parentWidget()->layout()->setSizeConstraint( QLayout::SetMinimumSize );
	parentWidget()->resize( minimumWidth(), 300 );
	parentWidget()->move( 610, 5 );
	parentWidget()->show();


	setModel( _tc );
	connect( &_tc->m_bbComboBoxModel, SIGNAL( dataChanged() ),
			this, SLOT( updatePosition() ) );
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
	engine::mainWindow()->setPlaybackMode( PPM_BB );
	
	if( engine::getSong()->isPlaying() )
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
	else if( engine::getSong()->isPaused() )
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




void bbEditor::addAutomationTrack( void )
{
	(void) track::create( track::AutomationTrack, model() );
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

