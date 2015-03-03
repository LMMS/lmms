/*
 * TrackContainerView.cpp - view-component for TrackContainer
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QApplication>
#include <QLayout>
#include <QMdiArea>
#include <QProgressDialog>
#include <QScrollBar>
#include <QWheelEvent>


#include "TrackContainerView.h"
#include "TrackContainer.h"
#include "BBTrack.h"
#include "MainWindow.h"
#include "debug.h"
#include "FileBrowser.h"
#include "ImportFilter.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "DataFile.h"
#include "Rubberband.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "Track.h"
#include "GuiApplication.h"


TrackContainerView::TrackContainerView( TrackContainer * _tc ) :
	QWidget(),
	ModelView( NULL, this ),
	JournallingObject(),
	SerializingObjectHook(),
	m_currentPosition( 0, 0 ),
	m_tc( _tc ),
	m_trackViews(),
	m_scrollArea( new scrollArea( this ) ),
	m_ppt( DEFAULT_PIXELS_PER_TACT ),
	m_rubberBand( new RubberBand( m_scrollArea ) ),
	m_origin()
{
	m_tc->setHook( this );

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
	connect( this, SIGNAL( positionChanged( const MidiTime & ) ),
				_tv->getTrackContentWidget(),
				SLOT( changePosition( const MidiTime & ) ) );
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




void TrackContainerView::moveTrackViewUp( TrackView * _tv )
{
	for( int i = 1; i < m_trackViews.size(); ++i )
	{
		TrackView * t = m_trackViews[i];
		if( t == _tv )
		{
			BBTrack::swapBBTracks( t->getTrack(),
					m_trackViews[i - 1]->getTrack() );
			m_scrollLayout->removeWidget( t );
			m_scrollLayout->insertWidget( i - 1, t );
			qSwap( m_tc->m_tracks[i-1], m_tc->m_tracks[i] );
			m_trackViews.swap( i - 1, i );
			realignTracks();
			break;
		}
	}
}




void TrackContainerView::moveTrackViewDown( TrackView * _tv )
{
	for( int i = 0; i < m_trackViews.size()-1; ++i )
	{
		TrackView * t = m_trackViews[i];
		if( t == _tv )
		{
			BBTrack::swapBBTracks( t->getTrack(),
					m_trackViews[i + 1]->getTrack() );
			m_scrollLayout->removeWidget( t );
			m_scrollLayout->insertWidget( i + 1, t );
			qSwap( m_tc->m_tracks[i], m_tc->m_tracks[i+1] );
			m_trackViews.swap( i, i + 1 );
			realignTracks();
			break;
		}
	}
}





void TrackContainerView::realignTracks()
{
	QWidget * content = m_scrollArea->widget();
	content->setFixedWidth( width()
				- m_scrollArea->verticalScrollBar()->width() );
	content->setFixedHeight( content->minimumSizeHint().height() );

	for( trackViewList::iterator it = m_trackViews.begin();
						it != m_trackViews.end(); ++it )
	{
		( *it )->show();
		( *it )->update();
	}
}




void TrackContainerView::createTrackView( Track * _t )
{
	//m_tc->addJournalCheckPoint();

	_t->createView( this );
}




void TrackContainerView::deleteTrackView( TrackView * _tv )
{
	//m_tc->addJournalCheckPoint();

	Track * t = _tv->getTrack();
	removeTrackView( _tv );
	delete _tv;

	t->deleteLater();
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




void TrackContainerView::setPixelsPerTact( int _ppt )
{
	m_ppt = _ppt;

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
				"importedproject,soundfontfile,vstpluginfile,projectfile,"
				"track_%1,track_%2" ).
						arg( Track::InstrumentTrack ).
						arg( Track::SampleTrack ) );
}

void TrackContainerView::selectRegionFromPixels(int xStart, int xEnd)
{
	m_rubberBand->setEnabled( true );
	m_rubberBand->show();
	m_rubberBand->setGeometry( min( xStart, xEnd ), 0, max( xStart, xEnd ) - min( xStart, xEnd ), std::numeric_limits<int>::max() );
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
		|| type == "soundfontfile" || type == "vstpluginfile")
	{
		InstrumentTrack * it = dynamic_cast<InstrumentTrack *>(
				Track::create( Track::InstrumentTrack,
								m_tc ) );
		Instrument * i = it->loadInstrument(
			Engine::pluginFileHandling()[FileItem::extension(
								value )]);
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
		if( gui->mainWindow()->mayChangeProject() )
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




void TrackContainerView::mousePressEvent( QMouseEvent * _me )
{
	if( allowRubberband() == true )
	{
		m_origin = m_scrollArea->mapFromParent( _me->pos() );
		m_rubberBand->setEnabled( true );
		m_rubberBand->setGeometry( QRect( m_origin, QSize() ) );
		m_rubberBand->show();
	}
	QWidget::mousePressEvent( _me );
}




void TrackContainerView::mouseMoveEvent( QMouseEvent * _me )
{
	if( rubberBandActive() == true )
	{
		m_rubberBand->setGeometry( QRect( m_origin,
				m_scrollArea->mapFromParent( _me->pos() ) ).
								normalized() );
	}
	QWidget::mouseMoveEvent( _me );
}




void TrackContainerView::mouseReleaseEvent( QMouseEvent * _me )
{
	m_rubberBand->hide();
	m_rubberBand->setEnabled( false );
	QWidget::mouseReleaseEvent( _me );
}





void TrackContainerView::resizeEvent( QResizeEvent * _re )
{
	realignTracks();
	QWidget::resizeEvent( _re );
}




TrackContainerView::scrollArea::scrollArea( TrackContainerView * _parent ) :
	QScrollArea( _parent ),
	m_trackContainerView( _parent )
{
	setFrameStyle( QFrame::NoFrame );
	setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
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
	Instrument *i = m_it->loadInstrument( m_name );
	QObject *parent = i->parent();
	i->setParent( 0 );
	i->moveToThread( m_containerThread );
	i->setParent( parent );
}
