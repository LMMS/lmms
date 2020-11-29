/*
 * TrackContainerView.cpp - view-component for TrackContainer
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

#include "TrackContainerView.h"

#include <cmath>

#include <QApplication>
#include <QLayout>
#include <QMdiArea>
#include <QWheelEvent>

#include "TrackContainer.h"
#include "BBTrack.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "FileBrowser.h"
#include "ImportFilter.h"
#include "Instrument.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "GuiApplication.h"
#include "PluginFactory.h"

using namespace std;

TrackContainerView::TrackContainerView( TrackContainer * _tc ) :
	QWidget(),
	ModelView( NULL, this ),
	JournallingObject(),
	SerializingObjectHook(),
	m_currentPosition( 0, 0 ),
	m_tc( _tc ),
	m_trackViews(),
	m_scrollArea( new scrollArea( this ) ),
	m_ppb( DEFAULT_PIXELS_PER_BAR ),
	m_rubberBand( new RubberBand( m_scrollArea ) )
{
	m_tc->setHook( this );
	//keeps the direction of the widget, undepended on the locale
	setLayoutDirection( Qt::LeftToRight );
	QVBoxLayout * layout = new QVBoxLayout( this );
	layout->setMargin( 0 );
	layout->setSpacing( 0 );
	layout->addWidget( m_scrollArea );

	QWidget * scrollContent = new QWidget;
	m_scrollLayout = new QVBoxLayout( scrollContent );
	m_scrollLayout->setMargin( 0 );
	m_scrollLayout->setSpacing( 0 );
	m_scrollLayout->setSizeConstraint( QLayout::SetMinAndMaxSize );

	m_scrollArea->setWidget( scrollContent );

	m_scrollArea->show();
	m_rubberBand->hide();
	m_rubberBand->setEnabled( false );

	setAcceptDrops( true );

	connect( Engine::getSong(), SIGNAL( timeSignatureChanged( int, int ) ),
						this, SLOT( realignTracks() ) );
	connect( m_tc, SIGNAL( trackAdded( Track * ) ),
			this, SLOT( createTrackView( Track * ) ),
			Qt::QueuedConnection );
}




TrackContainerView::~TrackContainerView()
{
	while( !m_trackViews.empty() )
	{
		delete m_trackViews.takeLast();
	}
}





void TrackContainerView::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	MainWindow::saveWidgetState( this, _this );
}




void TrackContainerView::loadSettings( const QDomElement & _this )
{
	MainWindow::restoreWidgetState( this, _this );
}




TrackView * TrackContainerView::addTrackView( TrackView * _tv )
{
	m_trackViews.push_back( _tv );
	m_scrollLayout->addWidget( _tv );
	connect( this, SIGNAL( positionChanged( const TimePos & ) ),
				_tv->getTrackContentWidget(),
				SLOT( changePosition( const TimePos & ) ) );
	realignTracks();
	return( _tv );
}




void TrackContainerView::removeTrackView( TrackView * _tv )
{
	int index = m_trackViews.indexOf( _tv );
	if( index != -1 )
	{
		m_trackViews.removeAt( index );

		disconnect( _tv );
		m_scrollLayout->removeWidget( _tv );

		realignTracks();
		if( Engine::getSong() )
		{
			Engine::getSong()->setModified();
		}
	}
}




void TrackContainerView::moveTrackView( TrackView * trackView, int indexTo )
{
	// Can't move out of bounds
	if ( indexTo >= m_trackViews.size() || indexTo < 0 ) { return; }

	// Does not need to move to itself
	int indexFrom = m_trackViews.indexOf( trackView );
	if ( indexFrom == indexTo ) { return; }

	BBTrack::swapBBTracks( trackView->getTrack(),
			m_trackViews[indexTo]->getTrack() );

	m_scrollLayout->removeWidget( trackView );
	m_scrollLayout->insertWidget( indexTo, trackView );

	Track * track = m_tc->m_tracks[indexFrom];

	m_tc->m_tracks.remove( indexFrom );
	m_tc->m_tracks.insert( indexTo, track );
	m_trackViews.move( indexFrom, indexTo );

	realignTracks();
}




void TrackContainerView::moveTrackViewUp( TrackView * trackView )
{
	int index = m_trackViews.indexOf( trackView );

	moveTrackView( trackView, index - 1 );
}




void TrackContainerView::moveTrackViewDown( TrackView * trackView )
{
	int index = m_trackViews.indexOf( trackView );

	moveTrackView( trackView, index + 1 );
}

void TrackContainerView::scrollToTrackView( TrackView * _tv )
{
	if (!m_trackViews.contains(_tv))
	{
		qWarning("TrackContainerView::scrollToTrackView: TrackView is not owned by this");
	}
	else
	{
		int currentScrollTop = m_scrollArea->verticalScrollBar()->value();
		int scrollAreaHeight = m_scrollArea->size().height();
		int trackViewTop = _tv->pos().y();
		int trackViewBottom = trackViewTop + _tv->size().height();

		// displayed_location = widget_location - currentScrollTop
		// want to make sure that the widget top has displayed location > 0,
		// and widget bottom < scrollAreaHeight
		// trackViewTop - scrollY > 0 && trackViewBottom - scrollY < scrollAreaHeight
		// therefore scrollY < trackViewTop && scrollY > trackViewBottom - scrollAreaHeight
		int newScroll = std::max( trackViewBottom-scrollAreaHeight, std::min(currentScrollTop, trackViewTop) );
		m_scrollArea->verticalScrollBar()->setValue(newScroll);
	}
}




void TrackContainerView::realignTracks()
{
	m_scrollArea->widget()->setFixedWidth(width());
	m_scrollArea->widget()->setFixedHeight(
				m_scrollArea->widget()->minimumSizeHint().height());

	for( trackViewList::iterator it = m_trackViews.begin();
						it != m_trackViews.end(); ++it )
	{
		( *it )->show();
		( *it )->update();
	}
}




TrackView * TrackContainerView::createTrackView( Track * _t )
{
	//m_tc->addJournalCheckPoint();

	// Avoid duplicating track views
	for( trackViewList::iterator it = m_trackViews.begin();
						it != m_trackViews.end(); ++it )
	{
		if ( ( *it )->getTrack() == _t ) { return ( *it ); }
	}

	return _t->createView( this );
}




void TrackContainerView::deleteTrackView( TrackView * _tv )
{
	//m_tc->addJournalCheckPoint();

	Track * t = _tv->getTrack();
	removeTrackView( _tv );
	delete _tv;

	Engine::mixer()->requestChangeInModel();
	delete t;
	Engine::mixer()->doneChangeInModel();
}




const TrackView * TrackContainerView::trackViewAt( const int _y ) const
{
	const int abs_y = _y + m_scrollArea->verticalScrollBar()->value();
	int y_cnt = 0;

//	debug code
//	qDebug( "abs_y %d", abs_y );

	for( trackViewList::const_iterator it = m_trackViews.begin();
						it != m_trackViews.end(); ++it )
	{
		const int y_cnt1 = y_cnt;
		y_cnt += ( *it )->height();
		if( abs_y >= y_cnt1 && abs_y < y_cnt )
		{
			return( *it );
		}
	}
	return( NULL );
}




bool TrackContainerView::allowRubberband() const
{
	return( false );
}




void TrackContainerView::setPixelsPerBar( int ppb )
{
	m_ppb = ppb;

	// tell all TrackContentWidgets to update their background tile pixmap
	for( trackViewList::Iterator it = m_trackViews.begin();
						it != m_trackViews.end(); ++it )
	{
		( *it )->getTrackContentWidget()->updateBackground();
	}
}




void TrackContainerView::clearAllTracks()
{
	while( !m_trackViews.empty() )
	{
		TrackView * tv = m_trackViews.takeLast();
		Track * t = tv->getTrack();
		delete tv;
		delete t;
	}
}




void TrackContainerView::dragEnterEvent( QDragEnterEvent * _dee )
{
	StringPairDrag::processDragEnterEvent( _dee,
		QString( "presetfile,pluginpresetfile,samplefile,instrument,"
				"importedproject,soundfontfile,patchfile,vstpluginfile,projectfile,"
				"track_%1,track_%2" ).
						arg( Track::InstrumentTrack ).
						arg( Track::SampleTrack ) );
}




void TrackContainerView::stopRubberBand()
{
	m_rubberBand->hide();
	m_rubberBand->setEnabled( false );
}




void TrackContainerView::dropEvent( QDropEvent * _de )
{
	QString type = StringPairDrag::decodeKey( _de );
	QString value = StringPairDrag::decodeValue( _de );
	if( type == "instrument" )
	{
		InstrumentTrack * it = dynamic_cast<InstrumentTrack *>(
				Track::create( Track::InstrumentTrack,
								m_tc ) );
		InstrumentLoaderThread *ilt = new InstrumentLoaderThread(
					this, it, value );
		ilt->start();
		//it->toggledInstrumentTrackButton( true );
		_de->accept();
	}
	else if( type == "samplefile" || type == "pluginpresetfile" 
		|| type == "soundfontfile" || type == "vstpluginfile"
		|| type == "patchfile" )
	{
		InstrumentTrack * it = dynamic_cast<InstrumentTrack *>(
				Track::create( Track::InstrumentTrack,
								m_tc ) );
		PluginFactory::PluginInfoAndKey piakn =
			pluginFactory->pluginSupportingExtension(FileItem::extension(value));
		Instrument * i = it->loadInstrument(piakn.info.name(), &piakn.key);
		i->loadFile( value );
		//it->toggledInstrumentTrackButton( true );
		_de->accept();
	}
	else if( type == "presetfile" )
	{
		DataFile dataFile( value );
		InstrumentTrack * it = dynamic_cast<InstrumentTrack *>(
				Track::create( Track::InstrumentTrack,
								m_tc ) );
		it->setSimpleSerializing();
		it->loadSettings( dataFile.content().toElement() );
		//it->toggledInstrumentTrackButton( true );
		_de->accept();
	}
	else if( type == "importedproject" )
	{
		ImportFilter::import( value, m_tc );
		_de->accept();
	}

	else if( type == "projectfile")
	{
		if( gui->mainWindow()->mayChangeProject(true) )
		{
			Engine::getSong()->loadProject( value );
		}
		_de->accept();
	}

	else if( type.left( 6 ) == "track_" )
	{
		DataFile dataFile( value.toUtf8() );
		Track::create( dataFile.content().firstChild().toElement(), m_tc );
		_de->accept();
	}
}




void TrackContainerView::resizeEvent( QResizeEvent * _re )
{
	realignTracks();
	QWidget::resizeEvent( _re );
}




RubberBand *TrackContainerView::rubberBand() const
{
	return m_rubberBand;
}




TrackContainerView::scrollArea::scrollArea( TrackContainerView * _parent ) :
	QScrollArea( _parent ),
	m_trackContainerView( _parent )
{
	setFrameStyle( QFrame::NoFrame );
	setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
}




TrackContainerView::scrollArea::~scrollArea()
{
}




void TrackContainerView::scrollArea::wheelEvent( QWheelEvent * _we )
{
	// always pass wheel-event to parent-widget (song-editor
	// bb-editor etc.) because they might want to use it for zooming
	// or scrolling left/right if a modifier-key is pressed, otherwise
	// they do not accept it and we pass it up to QScrollArea
	m_trackContainerView->wheelEvent( _we );
	if( !_we->isAccepted() )
	{
		QScrollArea::wheelEvent( _we );
	}
}




InstrumentLoaderThread::InstrumentLoaderThread( QObject *parent, InstrumentTrack *it, QString name ) :
	QThread( parent ),
	m_it( it ),
	m_name( name )
{
	m_containerThread = thread();
}




void InstrumentLoaderThread::run()
{
	Instrument *i = m_it->loadInstrument(m_name, nullptr,
				true /*always DnD*/);
	QObject *parent = i->parent();
	i->setParent( 0 );
	i->moveToThread( m_containerThread );
	i->setParent( parent );
}
