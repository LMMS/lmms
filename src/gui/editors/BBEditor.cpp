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

#include "BBEditor.h"

#include <QAction>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMdiArea>

#include "ComboBox.h"
#include "BBTrackContainer.h"
#include "embed.h"
#include "MainWindow.h"
#include "Song.h"
#include "ConfigManager.h"
#include "DataFile.h"
#include "StringPairDrag.h"

#include "TrackContainer.h"
#include "Pattern.h"



BBEditor::BBEditor( BBTrackContainer* tc ) :
	Editor(false),
	m_trackContainerView( new BBTrackContainerView(tc) )
{
	setWindowIcon( embed::getIconPixmap( "bb_track_btn" ) );
	setWindowTitle( tr( "Beat+Bassline Editor" ) );
	setCentralWidget(m_trackContainerView);

	setAcceptDrops(true);
	m_toolBar->setAcceptDrops(true);
	connect(m_toolBar, SIGNAL(dragEntered(QDragEnterEvent*)), m_trackContainerView, SLOT(dragEnterEvent(QDragEnterEvent*)));
	connect(m_toolBar, SIGNAL(dropped(QDropEvent*)), m_trackContainerView, SLOT(dropEvent(QDropEvent*)));

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


	m_playAction->setToolTip(tr( "Play/pause current beat/bassline (Space)" ));
	m_stopAction->setToolTip(tr( "Stop playback of current beat/bassline (Space)" ));

	m_playAction->setWhatsThis(
		tr( "Click here to play the current "
			"beat/bassline.  The beat/bassline is automatically "
			"looped when its end is reached." ) );
	m_stopAction->setWhatsThis(
		tr( "Click here to stop playing of current "
							"beat/bassline." ) );

	m_bbComboBox = new ComboBox( m_toolBar );
	m_bbComboBox->setFixedSize( 200, 22 );
	m_bbComboBox->setModel( &tc->m_bbComboBoxModel );

	m_toolBar->addSeparator();
	m_toolBar->addWidget( m_bbComboBox );

	m_toolBar->addSeparator();
	m_toolBar->addAction(embed::getIconPixmap("add_bb_track"), tr("Add beat/bassline"),
						 Engine::getSong(), SLOT(addBBTrack()));
	m_toolBar->addAction(embed::getIconPixmap("add_automation"), tr("Add automation-track"),
						 m_trackContainerView, SLOT(addAutomationTrack()));

	QWidget* stretch = new QWidget(m_toolBar);
	stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_toolBar->addWidget(stretch);

	m_toolBar->addAction(embed::getIconPixmap("step_btn_remove"), tr("Remove steps"),
						 m_trackContainerView, SLOT(removeSteps()));
	m_toolBar->addAction(embed::getIconPixmap("step_btn_add"), tr("Add steps"),
						 m_trackContainerView, SLOT(addSteps()));
	m_toolBar->addSeparator();

	connect( &tc->m_bbComboBoxModel, SIGNAL( dataChanged() ),
			m_trackContainerView, SLOT( updatePosition() ) );

	QAction* viewNext = new QAction(this);
	connect(viewNext, SIGNAL(triggered()), m_bbComboBox, SLOT(selectNext()));
	viewNext->setShortcut(Qt::Key_Plus);
	addAction(viewNext);

	QAction* viewPrevious = new QAction(this);
	connect(viewPrevious, SIGNAL(triggered()), m_bbComboBox, SLOT(selectPrevious()));
	viewPrevious->setShortcut(Qt::Key_Minus);
	addAction(viewPrevious);
}


BBEditor::~BBEditor()
{
}


QSize BBEditor::sizeHint() const
{
	return {minimumWidth()+10, 300};
}


void BBEditor::removeBBView( int bb )
{
	m_trackContainerView->removeBBView(bb);
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




BBTrackContainerView::BBTrackContainerView(BBTrackContainer* tc) :
	TrackContainerView(tc),
	m_bbtc(tc)
{
	setModel( tc );
}




void BBTrackContainerView::addSteps()
{
	TrackContainer::TrackList tl = model()->tracks();

	for( TrackContainer::TrackList::iterator it = tl.begin();
		it != tl.end(); ++it )
	{
		if( ( *it )->type() == Track::InstrumentTrack )
		{
			Pattern* p = static_cast<Pattern *>( ( *it )->getTCO( m_bbtc->currentBB() ) );
			p->addSteps();
		}
	}
}




void BBTrackContainerView::removeSteps()
{
	TrackContainer::TrackList tl = model()->tracks();

	for( TrackContainer::TrackList::iterator it = tl.begin();
		it != tl.end(); ++it )
	{
		if( ( *it )->type() == Track::InstrumentTrack )
		{
			Pattern* p = static_cast<Pattern *>( ( *it )->getTCO( m_bbtc->currentBB() ) );
			p->removeSteps();
		}
	}
}




void BBTrackContainerView::addAutomationTrack()
{
	(void) Track::create( Track::AutomationTrack, model() );
}




void BBTrackContainerView::removeBBView(int bb)
{
	foreach( TrackView* view, trackViews() )
	{
		view->getTrackContentWidget()->removeTCOView( bb );
	}
}



void BBTrackContainerView::saveSettings(QDomDocument& doc, QDomElement& element)
{
	MainWindow::saveWidgetState(parentWidget(), element);
}

void BBTrackContainerView::loadSettings(const QDomElement& element)
{
	MainWindow::restoreWidgetState(parentWidget(), element);
}




void BBTrackContainerView::dropEvent(QDropEvent* de)
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




void BBTrackContainerView::updatePosition()
{
	//realignTracks();
	emit positionChanged( m_currentPosition );
}
