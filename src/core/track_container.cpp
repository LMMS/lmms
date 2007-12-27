#ifndef SINGLE_SOURCE_COMPILE

/*
 * track_container.cpp - implementation of base-class for all track-containers
 *                       like Song-Editor, BB-Editor...
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QtGui/QApplication>
#include <QtGui/QProgressDialog>
#include <QtGui/QWheelEvent>

#else

#include <qdom.h>
#include <qapplication.h>
#include <qprogressdialog.h>

#define setValue setProgress
#define maximum totalSteps

#endif


#include "track_container.h"
#include "bb_track.h"
#include "config_mgr.h"
#include "debug.h"
#include "engine.h"
#include "file_browser.h"
#include "import_filter.h"
#include "instrument.h"
#include "instrument_track.h"
#include "main_window.h"
#include "mixer.h"
#include "mmp.h"
#include "project_journal.h"
#include "rubberband.h"
#include "song_editor.h"
#include "string_pair_drag.h"
#include "templates.h"
#include "track.h"


trackContainer::trackContainer( void ) :
	QMainWindow( engine::getMainWindow()->workspace()
#ifdef QT3
				, 0, Qt::WStyle_Title
#endif
			 ),
	m_currentPosition( 0, 0 ),
	m_scrollArea( new scrollArea( this ) ),
	m_ppt( DEFAULT_PIXELS_PER_TACT ),
	m_rubberBand( new rubberBand( m_scrollArea ) ),
	m_origin()
{
#ifdef QT4
	if( engine::getMainWindow()->workspace() != NULL )
	{
		engine::getMainWindow()->workspace()->addWindow( this );
	}
#endif

	m_scrollArea->show();
	m_rubberBand->hide();

	setAcceptDrops( TRUE );
}




trackContainer::~trackContainer()
{
	engine::getProjectJournal()->setJournalling( FALSE );

	while( m_trackWidgets.size() )
	{
		removeTrack( m_trackWidgets.last()->getTrack() );
	}

	engine::getProjectJournal()->setJournalling( TRUE );
}




void trackContainer::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setTagName( classNodeName() );
	_this.setAttribute( "type", nodeName() );
	mainWindow::saveWidgetState( this, _this );

	// save settings of each track
	for( trackWidgetVector::iterator it = m_trackWidgets.begin();
					it != m_trackWidgets.end(); ++it )
	{
		( *it )->getTrack()->saveState( _doc, _this );
	}
}




void trackContainer::loadSettings( const QDomElement & _this )
{
	static QProgressDialog * pd = NULL;
	bool was_null = ( pd == NULL );
	int start_val = 0;
	if( pd == NULL )
	{
#ifdef QT4
		pd = new QProgressDialog( tr( "Loading project..." ),
						tr( "Cancel" ), 0,
						_this.childNodes().count() );
#else
		pd = new QProgressDialog( tr( "Loading project..." ),
						tr( "Cancel" ),
						_this.childNodes().count(),
								0, 0, TRUE );
#endif
		pd->setWindowTitle( tr( "Please wait..." ) );
		pd->show();
	}
	else
	{
#ifdef QT4
		start_val = pd->value();
		pd->setMaximum( pd->maximum() + _this.childNodes().count() );
#else
		start_val = pd->progress();
		pd->setTotalSteps( pd->maximum() + _this.childNodes().count() );
#endif
	}

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
#ifdef QT4
		pd->setValue( pd->value() + 1 );
		qApp->processEvents( QEventLoop::AllEvents, 100 );
#else
		pd->setValue( pd->progress() + 1 );
		qApp->processEvents( 100 );
#endif

		if( pd->wasCanceled() )
		{
			break;
		}

		if( node.isElement() &&
			!node.toElement().attribute( "metadata" ).toInt() )
		{
			track::create( node.toElement(), this );
		}
		node = node.nextSibling();
	}

	mainWindow::restoreWidgetState( this, _this );

	pd->setValue( start_val + _this.childNodes().count() );

	if( was_null )
	{
		delete pd;
		pd = NULL;
	}
}




void trackContainer::addTrack( track * _track )
{
	QMap<QString, QVariant> map;
	map["id"] = _track->id();
	addJournalEntry( journalEntry( ADD_TRACK, map ) );

	m_trackWidgets.push_back( _track->getTrackWidget() );
#ifndef QT4
	m_scrollArea->addChild( _track->getTrackWidget() );
#endif
	connect( this, SIGNAL( positionChanged( const midiTime & ) ),
				_track->getTrackWidget(),
				SLOT( changePosition( const midiTime & ) ) );
	realignTracks();
}




void trackContainer::removeTrack( track * _track )
{
	trackWidgetVector::iterator it = qFind( m_trackWidgets.begin(),
			m_trackWidgets.end(), _track->getTrackWidget() );
	if( it != m_trackWidgets.end() )
	{
		QMap<QString, QVariant> map;
		multimediaProject mmp( multimediaProject::JOURNAL_DATA );
		_track->saveState( mmp, mmp.content() );
		map["id"] = _track->id();
		map["state"] = mmp.toString();
		addJournalEntry( journalEntry( REMOVE_TRACK, map ) );

#ifndef QT4
		m_scrollArea->removeChild( _track->getTrackWidget() );
#endif
		m_trackWidgets.erase( it );

		delete _track;

		realignTracks();
		if( engine::getSongEditor() )
		{
			engine::getSongEditor()->setModified();
		}
	}
}




void trackContainer::moveTrackUp( track * _track )
{
	for( trackWidgetVector::iterator it = m_trackWidgets.begin();
					it != m_trackWidgets.end(); ++it )
	{
		if( *it == _track->getTrackWidget() &&
						it > m_trackWidgets.begin() )
		{
			bbTrack::swapBBTracks( ( *it )->getTrack(),
						( *( it - 1 ) )->getTrack() );
			qSwap( *it, *( it - 1 ) );
			realignTracks();
			break;
		}
	}
}




void trackContainer::moveTrackDown( track * _track )
{
	for( trackWidgetVector::iterator it = m_trackWidgets.begin();
					it != m_trackWidgets.end(); ++it )
	{
		if( *it == _track->getTrackWidget() &&
						it + 1 < m_trackWidgets.end() )
		{
			bbTrack::swapBBTracks( ( *it )->getTrack(),
						( *( it + 1 ) )->getTrack() );
			qSwap( *it, *( it + 1 ) );
			realignTracks();
			break;
		}
	}
}




void trackContainer::updateAfterTrackAdd( void )
{
}




void trackContainer::realignTracks( bool _complete_update )
{
	int y = 0;
	for( trackWidgetVector::iterator it = m_trackWidgets.begin();
					it != m_trackWidgets.end(); ++it )
	{
		( *it )->show();
		( *it )->update();
#ifdef QT4
		( *it )->move( 0, y );
#else
		m_scrollArea->moveChild( *it, 0, y );
#endif
		( *it )->resize( width() - DEFAULT_SCROLLBAR_SIZE,
				 			( *it )->height() );
		( *it )->changePosition( m_currentPosition );
		y += ( *it )->height();
	}
#ifdef QT3
	m_scrollArea->resizeContents( width() - DEFAULT_SCROLLBAR_SIZE, y );
#endif
	updateScrollArea();
}




void trackContainer::clearAllTracks( void )
{
	while( !m_trackWidgets.empty() )
	{
		removeTrack( m_trackWidgets.last()->getTrack() );
	}
}




const trackWidget * trackContainer::trackWidgetAt( const int _y ) const
{
	const int abs_y = _y +
#ifndef QT3
				m_scrollArea->viewport()->y();
#else
				m_scrollArea->contentsY();
#endif
	int y_cnt = 0;
	for( trackWidgetVector::const_iterator it = m_trackWidgets.begin();
					it != m_trackWidgets.end(); ++it )
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




bool trackContainer::allowRubberband( void ) const
{
	return( FALSE );
}




Uint16 trackContainer::countTracks( track::trackTypes _tt ) const
{
	Uint16 cnt = 0;
	for( trackWidgetVector::const_iterator it = m_trackWidgets.begin();
					it != m_trackWidgets.end(); ++it )
	{
		if( ( *it )->getTrack()->type() == _tt ||
					_tt == track::TOTAL_TRACK_TYPES )
		{
			++cnt;
		}
	}
	return( cnt );
}




void trackContainer::setMutedOfAllTracks( bool _muted )
{
	for( trackWidgetVector::iterator it = m_trackWidgets.begin();
					it != m_trackWidgets.end(); ++it )
	{
		( *it )->getTrack()->setMuted( _muted );
	}
}




constTrackVector trackContainer::tracks( void ) const
{
	constTrackVector tracks;
	for( trackWidgetVector::const_iterator it = m_trackWidgets.begin();
					it != m_trackWidgets.end(); ++it )
	{
		tracks.push_back( ( *it )->getTrack() );
	}
	return( tracks );
}




trackVector trackContainer::tracks( void )
{
	trackVector tracks;
	for( trackWidgetVector::iterator it = m_trackWidgets.begin();
					it != m_trackWidgets.end(); ++it )
	{
		tracks.push_back( ( *it )->getTrack() );
	}
	return( tracks );
}




void trackContainer::setPixelsPerTact( Uint16 _ppt )
{
	m_ppt = _ppt;
}




void trackContainer::undoStep( journalEntry & _je )
{
	saveJournallingState( FALSE );
	switch( _je.actionID() )
	{
		case ADD_TRACK:
		{
			QMap<QString, QVariant> map = _je.data().toMap();
			track * tr =
				dynamic_cast<track *>(
			engine::getProjectJournal()->getJournallingObject(
							map["id"].toInt() ) );
			assert( tr != NULL );
			multimediaProject mmp(
					multimediaProject::JOURNAL_DATA );
			tr->saveState( mmp, mmp.content() );
			map["state"] = mmp.toString();
			_je.data() = map;
			removeTrack( tr );
			break;
		}

		case REMOVE_TRACK:
		{
			multimediaProject mmp(
				_je.data().toMap()["state"].toString(), FALSE );
			track::create( mmp.content().firstChild().toElement(),
									this );
			break;
		}
	}
	restoreJournallingState();
}




void trackContainer::redoStep( journalEntry & _je )
{
	switch( _je.actionID() )
	{
		case ADD_TRACK:
		case REMOVE_TRACK:
			_je.actionID() = ( _je.actionID() == ADD_TRACK ) ?
						REMOVE_TRACK : ADD_TRACK;
			undoStep( _je );
			_je.actionID() = ( _je.actionID() == ADD_TRACK ) ?
						REMOVE_TRACK : ADD_TRACK;
			break;
	}
}




void trackContainer::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee,
		QString( "presetfile,sampledata,samplefile,instrument,midifile,"
					"track_%1,track_%2" ).
						arg( track::INSTRUMENT_TRACK ).
						arg( track::SAMPLE_TRACK ) );
}




void trackContainer::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	engine::getMixer()->lock();
	if( type == "instrument" )
	{
		instrumentTrack * it = dynamic_cast<instrumentTrack *>(
				track::create( track::INSTRUMENT_TRACK,
								this ) );
		it->loadInstrument( value );
		it->toggledInstrumentTrackButton( TRUE );
		_de->accept();
	}
	else if( type == "sampledata" || type == "samplefile" )
	{
		instrumentTrack * it = dynamic_cast<instrumentTrack *>(
				track::create( track::INSTRUMENT_TRACK,
								this ) );
		QString iname = type == "sampledata" ? "audiofileprocessor" :
			engine::sampleExtensions()[fileItem::extension(
								value )];
		instrument * i = it->loadInstrument( iname );
		i->setParameter( type, value );
		it->toggledInstrumentTrackButton( TRUE );
		_de->accept();
	}
	else if( type == "presetfile" )
	{
		multimediaProject mmp( value );
		instrumentTrack * it = dynamic_cast<instrumentTrack *>(
				track::create( track::INSTRUMENT_TRACK,
								this ) );
		it->loadTrackSpecificSettings( mmp.content().firstChild().
								toElement() );
		it->toggledInstrumentTrackButton( TRUE );
		_de->accept();
	}
	else if( type == "midifile" )
	{
		importFilter::import( value, this );
		_de->accept();
	}
	else if( type.left( 6 ) == "track_" )
	{
		multimediaProject mmp( value, FALSE );
		track::create( mmp.content().firstChild().toElement(), this );
		// after adding a track, we have to make sure, actual editor
		// can setup new track (e.g. adding TCO's (bbEditor does so))
		updateAfterTrackAdd();
		_de->accept();
	}
	engine::getMixer()->unlock();
}




void trackContainer::mousePressEvent( QMouseEvent * _me )
{
	if( allowRubberband() == TRUE )
	{
		m_origin = m_scrollArea->mapFromParent( _me->pos() );
		m_rubberBand->setGeometry( QRect( m_origin, QSize() ) );
		m_rubberBand->show();
	}
}




void trackContainer::mouseMoveEvent( QMouseEvent * _me )
{
	if( rubberBandActive() == TRUE )
	{
		m_rubberBand->setGeometry( QRect( m_origin,
				m_scrollArea->mapFromParent( _me->pos() ) ).
								normalized() );
	}
}




void trackContainer::mouseReleaseEvent( QMouseEvent * _me )
{
	m_rubberBand->hide();
}





void trackContainer::resizeEvent( QResizeEvent * )
{
	realignTracks();
}




void trackContainer::updateScrollArea( void )
{
	m_scrollArea->resize( width(), scrollAreaRect().height() );
/*	m_scrollArea->resize( tMax( m_scrollArea->parentWidget()->width() - 
					m_scrollArea->x() - 2, 0 ),
				tMax( m_scrollArea->parentWidget()->height() -
					m_scrollArea->y() - 2, 0 ) );*/
}




trackContainer::scrollArea::scrollArea( trackContainer * _parent ) :
	QScrollArea( _parent ),
	m_trackContainer( _parent )
{
	setFrameStyle( QFrame::NoFrame );
	setHorizontalScrollBarPolicy( 
#ifdef QT4
					Qt::ScrollBarAlwaysOff
#else
					QScrollArea::AlwaysOff
#endif
					);
	setVerticalScrollBarPolicy( 
#ifdef QT4
					Qt::ScrollBarAlwaysOn
#else
					QScrollArea::AlwaysOn
#endif
					);
}




trackContainer::scrollArea::~scrollArea()
{
}




void trackContainer::scrollArea::wheelEvent( QWheelEvent * _we )
{
	// always pass wheel-event to parent-widget (song-editor
	// bb-editor etc.) because they might want to use it for zooming
	// or scrolling left/right if a modifier-key is pressed, otherwise
	// they do not accept it and we pass it up to QScrollArea
	m_trackContainer->wheelEvent( _we );
	if( !_we->isAccepted() )
	{
		QScrollArea::wheelEvent( _we );
	}
}




#include "track_container.moc"

#undef setValue
#undef maximum

#endif
