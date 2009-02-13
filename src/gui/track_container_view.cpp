/*
 * track_container_view.cpp - view-component for trackContainer
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


#include <QtGui/QApplication>
#include <QtGui/QLayout>
#include <QtGui/QMdiArea>
#include <QtGui/QProgressDialog>
#include <QtGui/QScrollBar>
#include <QtGui/QWheelEvent>


#include "track_container_view.h"
#include "track_container.h"
#include "bb_track.h"
#include "main_window.h"
#include "debug.h"
#include "file_browser.h"
#include "import_filter.h"
#include "instrument.h"
#include "instrument_track.h"
#include "mmp.h"
#include "project_journal.h"
#include "rubberband.h"
#include "song.h"
#include "string_pair_drag.h"
#include "track.h"

#include "fluiq/widget_container.h"


trackContainerView::trackContainerView( trackContainer * _tc ) :
	FLUIQ::CollapsibleWidget( Qt::Vertical ),
	modelView( NULL, this ),
	journallingObject(),
	serializingObjectHook(),
	m_currentPosition( 0, 0 ),
	m_tc( _tc ),
	m_trackViews(),
	m_scrollArea( new scrollArea( this ) ),
	m_ppt( DEFAULT_PIXELS_PER_TACT ),
	m_rubberBand( new rubberBand( m_scrollArea ) ),
	m_origin()
{
	engine::getMainWindow()->centralWidgetContainer()->addWidget( this );

	m_tc->setHook( this );

	addWidget( m_scrollArea );

	QWidget * scrollContent = new QWidget;
	m_scrollLayout = new QVBoxLayout( scrollContent );
	m_scrollLayout->setMargin( 0 );
	m_scrollLayout->setSpacing( 0 );
	m_scrollLayout->setSizeConstraint( QLayout::SetMinAndMaxSize );

	m_scrollArea->setWidget( scrollContent );

	m_scrollArea->show();
	m_rubberBand->hide();

	setAcceptDrops( true );

	connect( engine::getSong(), SIGNAL( timeSignatureChanged( int, int ) ),
						this, SLOT( realignTracks() ) );
	connect( m_tc, SIGNAL( trackAdded( track * ) ),
			this, SLOT( createTrackView( track * ) ),
			Qt::QueuedConnection );
}




trackContainerView::~trackContainerView()
{
	while( !m_trackViews.empty() )
	{
		delete m_trackViews.takeLast();
	}
}





void trackContainerView::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	mainWindow::saveWidgetState( this, _this );
}




void trackContainerView::loadSettings( const QDomElement & _this )
{
	mainWindow::restoreWidgetState( this, _this );
}




trackView * trackContainerView::addTrackView( trackView * _tv )
{
/*	QMap<QString, QVariant> map;
	map["id"] = _tv->getTrack()->id();
	addJournalEntry( journalEntry( AddTrack, map ) );*/

	m_trackViews.push_back( _tv );
	m_scrollLayout->addWidget( _tv );
	connect( this, SIGNAL( positionChanged( const midiTime & ) ),
				_tv->getTrackContentWidget(),
				SLOT( changePosition( const midiTime & ) ) );
	realignTracks();
	return( _tv );
}




void trackContainerView::removeTrackView( trackView * _tv )
{
	int index = m_trackViews.indexOf( _tv );
	if( index != -1 )
	{
/*		QMap<QString, QVariant> map;
		multimediaProject mmp( multimediaProject::JournalData );
		_tv->getTrack()->saveState( mmp, mmp.content() );
		map["id"] = _tv->getTrack()->id();
		map["state"] = mmp.toString();
		addJournalEntry( journalEntry( RemoveTrack, map ) );*/

		m_trackViews.removeAt( index );

		disconnect( _tv );
		m_scrollLayout->removeWidget( _tv );

		realignTracks();
		if( engine::getSong() )
		{
			engine::getSong()->setModified();
		}
	}
}




void trackContainerView::moveTrackViewUp( trackView * _tv )
{
	for( int i = 1; i < m_trackViews.size(); ++i )
	{
		trackView * t = m_trackViews[i];
		if( t == _tv )
		{
			bbTrack::swapBBTracks( t->getTrack(),
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




void trackContainerView::moveTrackViewDown( trackView * _tv )
{
	for( int i = 0; i < m_trackViews.size()-1; ++i )
	{
		trackView * t = m_trackViews[i];
		if( t == _tv )
		{
			bbTrack::swapBBTracks( t->getTrack(),
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





void trackContainerView::realignTracks( void )
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
        ( *it )->getTrackContentWidget()->updateBackground();
	}
}




void trackContainerView::createTrackView( track * _t )
{
	_t->createView( this );
}




void trackContainerView::deleteTrackView( trackView * _tv )
{
	track * t = _tv->getTrack();
	removeTrackView( _tv );
	delete _tv;

	engine::getMixer()->lock();
	delete t;
	engine::getMixer()->unlock();
}




const trackView * trackContainerView::trackViewAt( const int _y ) const
{
	const int abs_y = _y + m_scrollArea->viewport()->y();
	int y_cnt = 0;
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




bool trackContainerView::allowRubberband( void ) const
{
	return( false );
}




void trackContainerView::setPixelsPerTact( int _ppt )
{
	m_ppt = _ppt;
}




void trackContainerView::clearAllTracks( void )
{
	while( !m_trackViews.empty() )
	{
		trackView * tv = m_trackViews.takeLast();
		track * t = tv->getTrack();
		delete tv;
		delete t;
	}
}




void trackContainerView::undoStep( journalEntry & _je )
{
#if 0
	saveJournallingState( false );
	switch( _je.actionID() )
	{
		case AddTrack:
		{
			QMap<QString, QVariant> map = _je.data().toMap();
			track * t =
				dynamic_cast<track *>(
			engine::getProjectJournal()->getJournallingObject(
							map["id"].toInt() ) );
			assert( t != NULL );
			multimediaProject mmp( multimediaProject::JournalData );
			t->saveState( mmp, mmp.content() );
			map["state"] = mmp.toString();
			_je.data() = map;
			t->deleteLater();
			break;
		}

		case RemoveTrack:
		{
			multimediaProject mmp(
				_je.data().toMap()["state"].toString(), false );
			track::create( mmp.content().firstChild().toElement(),
									m_tc );
			break;
		}
	}
	restoreJournallingState();
#endif
}




void trackContainerView::redoStep( journalEntry & _je )
{
#if 0
	switch( _je.actionID() )
	{
		case AddTrack:
		case RemoveTrack:
			_je.actionID() = ( _je.actionID() == AddTrack ) ?
						RemoveTrack : AddTrack;
			undoStep( _je );
			_je.actionID() = ( _je.actionID() == AddTrack ) ?
						RemoveTrack : AddTrack;
			break;
	}
#endif
}




void trackContainerView::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee,
		QString( "presetfile,sampledata,samplefile,instrument,"
				"importedproject,track_%1,track_%2" ).
						arg( track::InstrumentTrack ).
						arg( track::SampleTrack ) );
}




void trackContainerView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	engine::getMixer()->lock();
	if( type == "instrument" )
	{
		instrumentTrack * it = dynamic_cast<instrumentTrack *>(
				track::create( track::InstrumentTrack,
								m_tc ) );
		it->loadInstrument( value );
		//it->toggledInstrumentTrackButton( true );
		_de->accept();
	}
	else if( /*type == "sampledata" || */type == "samplefile" )
	{
		instrumentTrack * it = dynamic_cast<instrumentTrack *>(
				track::create( track::InstrumentTrack,
								m_tc ) );
		const QString iname = /*( type == "sampledata" ) ?
			"audiofileprocessor" :*/
			engine::pluginFileHandling()[fileItem::extension(
								value )];
		instrument * i = it->loadInstrument( iname );
		i->loadFile( value );
		//it->toggledInstrumentTrackButton( true );
		_de->accept();
	}
	else if( type == "presetfile" )
	{
		multimediaProject mmp( value );
		instrumentTrack * it = dynamic_cast<instrumentTrack *>(
				track::create( track::InstrumentTrack,
								m_tc ) );
		it->loadTrackSpecificSettings( mmp.content().firstChild().
								toElement() );
		//it->toggledInstrumentTrackButton( true );
		_de->accept();
	}
	else if( type == "importedproject" )
	{
		importFilter::import( value, m_tc );
		_de->accept();
	}
	else if( type.left( 6 ) == "track_" )
	{
		multimediaProject mmp( value, false );
		track::create( mmp.content().firstChild().toElement(), m_tc );
		_de->accept();
	}
	engine::getMixer()->unlock();
}




void trackContainerView::mousePressEvent( QMouseEvent * _me )
{
	if( allowRubberband() == true )
	{
		m_origin = m_scrollArea->mapFromParent( _me->pos() );
		m_rubberBand->setGeometry( QRect( m_origin, QSize() ) );
		m_rubberBand->show();
	}
	QWidget::mousePressEvent( _me );
}




void trackContainerView::mouseMoveEvent( QMouseEvent * _me )
{
	if( rubberBandActive() == true )
	{
		m_rubberBand->setGeometry( QRect( m_origin,
				m_scrollArea->mapFromParent( _me->pos() ) ).
								normalized() );
	}
	QWidget::mouseMoveEvent( _me );
}




void trackContainerView::mouseReleaseEvent( QMouseEvent * _me )
{
	m_rubberBand->hide();
	QWidget::mouseReleaseEvent( _me );
}





void trackContainerView::resizeEvent( QResizeEvent * _re )
{
	realignTracks();
	QWidget::resizeEvent( _re );
}




trackContainerView::scrollArea::scrollArea( trackContainerView * _parent ) :
	QScrollArea( NULL ),
	m_trackContainerView( _parent )
{
	setFrameStyle( QFrame::NoFrame );
	setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
}




trackContainerView::scrollArea::~scrollArea()
{
}




void trackContainerView::scrollArea::wheelEvent( QWheelEvent * _we )
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





#include "moc_track_container_view.cxx"

