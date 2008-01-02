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


#include "track_container.h"


#include <QtGui/QApplication>
#include <QtGui/QMdiArea>
#include <QtGui/QProgressDialog>
#include <QtGui/QScrollBar>
#include <QtGui/QWheelEvent>


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
#include "track.h"


trackContainer::trackContainer( void ) :
	m_currentPosition( 0, 0 ),
	m_scrollArea( new scrollArea( this ) ),
	m_ppt( DEFAULT_PIXELS_PER_TACT ),
	m_rubberBand( new rubberBand( m_scrollArea ) ),
	m_origin()
{
	QVBoxLayout * layout = new QVBoxLayout( this );
	layout->setMargin( 0 );
	layout->setSpacing( 0 );
	layout->addWidget( m_scrollArea );

	QWidget * scrollContent = new QWidget;
	m_scrollLayout = new QVBoxLayout( scrollContent );
	m_scrollLayout->setMargin( 0 );
	m_scrollLayout->setSpacing( 0 );
	m_scrollLayout->setSizeConstraint( QLayout::SetMinimumSize );

	m_scrollArea->setWidget( scrollContent );

	m_scrollArea->show();
	m_rubberBand->hide();

	setAcceptDrops( TRUE );
}




trackContainer::~trackContainer()
{
	while( !m_tracks.empty() )
	{
		delete m_tracks.takeLast();
	}
}




void trackContainer::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setTagName( classNodeName() );
	_this.setAttribute( "type", nodeName() );
	mainWindow::saveWidgetState( this, _this );

	// save settings of each track
	for( int i = 0; i < m_tracks.size(); ++i )
	{
		m_tracks[i]->saveState( _doc, _this );
	}
}




void trackContainer::loadSettings( const QDomElement & _this )
{
	static QProgressDialog * pd = NULL;
	bool was_null = ( pd == NULL );
	int start_val = 0;
	if( pd == NULL )
	{
		pd = new QProgressDialog( tr( "Loading project..." ),
						tr( "Cancel" ), 0,
						_this.childNodes().count() );
		pd->setWindowModality( Qt::ApplicationModal );
		pd->setWindowTitle( tr( "Please wait..." ) );
		pd->show();
	}
	else
	{
		start_val = pd->value();
		pd->setMaximum( pd->maximum() + _this.childNodes().count() );
	}

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		pd->setValue( pd->value() + 1 );
		qApp->processEvents( QEventLoop::AllEvents, 100 );

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

	realignTracks();

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

	m_tracks.push_back( _track );
	m_scrollLayout->addWidget( _track->getTrackWidget() );
	connect( this, SIGNAL( positionChanged( const midiTime & ) ),
				_track->getTrackWidget(),
				SLOT( changePosition( const midiTime & ) ) );
	realignTracks();
}




void trackContainer::removeTrack( track * _track )
{
	int index = m_tracks.indexOf( _track );
	if( index != -1 )
	{
		QMap<QString, QVariant> map;
		multimediaProject mmp( multimediaProject::JOURNAL_DATA );
		_track->saveState( mmp, mmp.content() );
		map["id"] = _track->id();
		map["state"] = mmp.toString();
		addJournalEntry( journalEntry( REMOVE_TRACK, map ) );

		m_tracks.removeAt( index );

		disconnect( _track->getTrackWidget() );
		m_scrollLayout->removeWidget( _track->getTrackWidget() );

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
	for( int i = 1; i < m_tracks.size(); ++i )
	{
		if( m_tracks[i] == _track )
		{
			bbTrack::swapBBTracks( m_tracks[i], m_tracks[i - 1] );
			QWidget * tw = m_tracks[i]->getTrackWidget();
			m_scrollLayout->removeWidget( tw );
			m_scrollLayout->insertWidget( i - 1, tw );
			m_tracks.swap( i - 1, i );
			realignTracks();
			break;
		}
	}
}




void trackContainer::moveTrackDown( track * _track )
{
	for( int i = 0; i < m_tracks.size() - 1; ++i )
	{
		if( m_tracks[i] == _track )
		{
			bbTrack::swapBBTracks( m_tracks[i], m_tracks[i + 1] );
			QWidget * tw = m_tracks[i]->getTrackWidget();
			m_scrollLayout->removeWidget( tw );
			m_scrollLayout->insertWidget( i + 1, tw );
			m_tracks.swap( i, i + 1 );
			realignTracks();
			break;
		}
	}
}




void trackContainer::updateAfterTrackAdd( void )
{
}




void trackContainer::realignTracks( void )
{
	QWidget * content = m_scrollArea->widget();
	content->setFixedWidth( width()
				- m_scrollArea->verticalScrollBar()->width() );
	content->setFixedHeight( content->minimumSizeHint().height() );

	for( int i = 0; i < m_tracks.size(); ++i )
	{
		trackWidget * tw = m_tracks[i]->getTrackWidget();
		tw->show();
		tw->update();
	}
}




void trackContainer::clearAllTracks( void )
{
	while( !m_tracks.empty() )
	{
		removeTrack( m_tracks.last() );
	}
}




const trackWidget * trackContainer::trackWidgetAt( const int _y ) const
{
	const int abs_y = _y + m_scrollArea->viewport()->y();
	int y_cnt = 0;
	for( int i = 0; i < m_tracks.size(); ++i )
	{
		const int y_cnt1 = y_cnt;
		y_cnt += m_tracks[i]->getTrackWidget()->height();
		if( abs_y >= y_cnt1 && abs_y < y_cnt )
		{
			return( m_tracks[i]->getTrackWidget() );
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
	for( int i = 0; i < m_tracks.size(); ++i )
	{
		if( m_tracks[i]->type() == _tt ||
					_tt == track::TOTAL_TRACK_TYPES )
		{
			++cnt;
		}
	}
	return( cnt );
}




void trackContainer::setMutedOfAllTracks( bool _muted )
{
	for( int i = 0; i < m_tracks.size(); ++i )
	{
		m_tracks[i]->setMuted( _muted );
	}
}




const QList<track *> trackContainer::tracks( void ) const
{
	return( m_tracks );
}




QList<track *> trackContainer::tracks( void )
{
	return( m_tracks );
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




trackContainer::scrollArea::scrollArea( trackContainer * _parent ) :
	QScrollArea( _parent ),
	m_trackContainer( _parent )
{
	setFrameStyle( QFrame::NoFrame );
	setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
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


#endif
