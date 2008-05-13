#ifndef SINGLE_SOURCE_COMPILE

/*
 * track.cpp - implementation of classes concerning tracks -> neccessary for
 *             all track-like objects (beat/bassline, sample-track...)
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


#include "track.h"

#include <assert.h>

#include <QtGui/QLayout>
#include <QtGui/QMenu>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QStyleOption>


#include "automation_pattern.h"
#include "automation_track.h"
#include "automatable_model_templates.h"
#include "bb_editor.h"
#include "bb_track.h"
#include "bb_track_container.h"
#include "clipboard.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "instrument_track.h"
#include "main_window.h"
#include "mmp.h"
#include "pixmap_button.h"
#include "project_journal.h"
#include "sample_track.h"
#include "song.h"
#include "string_pair_drag.h"
#include "templates.h"
#include "text_float.h"
#include "tooltip.h"
#include "track_container.h"


const Sint16 RESIZE_GRIP_WIDTH = 4;

const Uint16 TRACK_OP_BTN_WIDTH = 20;
const Uint16 TRACK_OP_BTN_HEIGHT = 14;

const Uint16 MINIMAL_TRACK_HEIGHT = 32;


textFloat * trackContentObjectView::s_textFloat = NULL;


// ===========================================================================
// trackContentObject
// ===========================================================================
trackContentObject::trackContentObject( track * _track ) :
	model( _track ),
	m_track( _track ),
	m_startPosition(),
	m_length(),
	m_mutedModel( FALSE, this )
{
	m_mutedModel.setTrack( _track );
	setJournalling( FALSE );
	movePosition( 0 );
	changeLength( 0 );
	setJournalling( TRUE );
}




trackContentObject::~trackContentObject()
{
	m_track->removeTCO( this );
}




void trackContentObject::movePosition( const midiTime & _pos )
{
	if( m_startPosition != _pos )
	{
		addJournalEntry( journalEntry( Move, m_startPosition - _pos ) );
		m_startPosition = _pos;
		engine::getSong()->updateLength();
	}
	emit positionChanged();
}




void trackContentObject::changeLength( const midiTime & _length )
{
	if( m_length != _length )
	{
		addJournalEntry( journalEntry( Resize, m_length - _length ) );
		m_length = _length;
		engine::getSong()->updateLength();
	}
	emit lengthChanged();
}




void trackContentObject::undoStep( journalEntry & _je )
{
	saveJournallingState( FALSE );
	switch( _je.actionID() )
	{
		case Move:
			movePosition( startPosition() + _je.data().toInt() );
			break;
		case Resize:
			changeLength( length() + _je.data().toInt() );
			break;
	}
	restoreJournallingState();
}




void trackContentObject::redoStep( journalEntry & _je )
{
	journalEntry je( _je.actionID(), -_je.data().toInt() );
	undoStep( je );
}




void trackContentObject::cut( void )
{
	copy();
	deleteLater();
}




void trackContentObject::copy( void )
{
	clipboard::copy( this );
}




void trackContentObject::paste( void )
{
	if( clipboard::getContent( nodeName() ) != NULL )
	{
		restoreState( *( clipboard::getContent( nodeName() ) ) );
	}
}




void trackContentObject::toggleMute( void )
{
	m_mutedModel.setValue( !m_mutedModel.value() );
	emit dataChanged();
}







trackContentObjectView::trackContentObjectView( trackContentObject * _tco,
							trackView * _tv ) :
	selectableObject( _tv->getTrackContentWidget() ),
	modelView( NULL ),
	m_tco( _tco ),
	m_trackView( _tv ),
	m_action( NoAction ),
	m_autoResize( FALSE ),
	m_initialMouseX( 0 ),
	m_hint( NULL )
{
	if( s_textFloat == NULL )
	{
		s_textFloat = new textFloat;
		s_textFloat->setPixmap( embed::getIconPixmap( "clock" ) );
	}

	setAttribute( Qt::WA_DeleteOnClose );
	setFocusPolicy( Qt::StrongFocus );
	move( 0, 1 );
	show();

	setFixedHeight( _tv->getTrackContentWidget()->height() - 2 );
	setAcceptDrops( TRUE );
	setMouseTracking( TRUE );

	connect( m_tco, SIGNAL( lengthChanged() ),
			this, SLOT( updateLength() ) );
	connect( m_tco, SIGNAL( positionChanged() ),
			this, SLOT( updatePosition() ) );
	connect( m_tco, SIGNAL( destroyed( QObject * ) ),
			this, SLOT( close() ), Qt::QueuedConnection );
	setModel( m_tco );

	m_trackView->getTrackContentWidget()->addTCOView( this );
}




trackContentObjectView::~trackContentObjectView()
{
	delete m_hint;
	// we have to give our track-container the focus because otherwise the
	// op-buttons of our track-widgets could become focus and when the user
	// presses space for playing song, just one of these buttons is pressed
	// which results in unwanted effects
	m_trackView->getTrackContainerView()->setFocus();
}




bool trackContentObjectView::fixedTCOs( void )
{
	return( m_trackView->getTrackContainerView()->fixedTCOs() );
}




bool trackContentObjectView::close( void )
{
	m_trackView->getTrackContentWidget()->removeTCOView( this );
	return( QWidget::close() );
}




void trackContentObjectView::remove( void )
{
	// delete ourself
	close();
	m_tco->deleteLater();
}




void trackContentObjectView::updateLength( void )
{
	if( fixedTCOs() )
	{
		setFixedWidth( parentWidget()->width() );
	}
	else
	{
		setFixedWidth( static_cast<int>( m_tco->length() *
						pixelsPerTact() /
						DefaultTicksPerTact ) +
							TCO_BORDER_WIDTH * 2 );
	}
	m_trackView->getTrackContainerView()->update();
}




void trackContentObjectView::updatePosition( void )
{
	m_trackView->getTrackContentWidget()->changePosition();
	// moving a TCO can result in change of song-length etc.,
	// therefore we update the track-container
	m_trackView->getTrackContainerView()->update();
}



void trackContentObjectView::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "tco_" +
				QString::number( m_tco->getTrack()->type() ) );
}




void trackContentObjectView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == ( "tco_" + QString::number( m_tco->getTrack()->type() ) ) )
	{
		// value contains our XML-data so simply create a
		// multimediaProject which does the rest for us...
		multimediaProject mmp( value, FALSE );
		// at least save position before getting to moved to somewhere
		// the user doesn't expect...
		midiTime pos = m_tco->startPosition();
		m_tco->restoreState( mmp.content().firstChild().toElement() );
		m_tco->movePosition( pos );
		_de->accept();
	}
}




void trackContentObjectView::leaveEvent( QEvent * _e )
{
	while( QApplication::overrideCursor() != NULL )
	{
		QApplication::restoreOverrideCursor();
	}
	if( _e != NULL )
	{
		QWidget::leaveEvent( _e );
	}
}




void trackContentObjectView::mousePressEvent( QMouseEvent * _me )
{
	if( m_trackView->getTrackContainerView()->allowRubberband() == TRUE &&
					_me->button() == Qt::LeftButton )
	{
		// if rubberband is active, we can be selected
		if( !m_trackView->getTrackContainerView()->rubberBandActive() )
		{
			if( engine::getMainWindow()->isCtrlPressed() == TRUE )
			{
				setSelected( !isSelected() );
			}
			else if( isSelected() == TRUE )
			{
				m_action = MoveSelection;
				m_initialMouseX = _me->x();
			}
		}
		else
		{
			selectableObject::mousePressEvent( _me );
		}
		return;
	}
	else if( engine::getMainWindow()->isShiftPressed() == TRUE )
	{
		// add/remove object to/from selection
		selectableObject::mousePressEvent( _me );
	}
	else if( _me->button() == Qt::LeftButton &&
			engine::getMainWindow()->isCtrlPressed() == TRUE )
	{
		// start drag-action
		multimediaProject mmp( multimediaProject::DragNDropData );
		m_tco->saveState( mmp, mmp.content() );
		QPixmap thumbnail = QPixmap::grabWidget( this ).scaled(
						128, 128,
						Qt::KeepAspectRatio,
						Qt::SmoothTransformation );
		new stringPairDrag( QString( "tco_%1" ).arg(
						m_tco->getTrack()->type() ),
					mmp.toString(), thumbnail, this );
	}
	else if( _me->button() == Qt::LeftButton &&
		/*	engine::getMainWindow()->isShiftPressed() == FALSE &&*/
							fixedTCOs() == FALSE )
	{
		// move or resize
		m_tco->setJournalling( FALSE );

		m_initialMouseX = _me->x();

		if( _me->x() < width() - RESIZE_GRIP_WIDTH )
		{
			m_action = Move;
			m_oldTime = m_tco->startPosition();
			QCursor c( Qt::SizeAllCursor );
			QApplication::setOverrideCursor( c );
			s_textFloat->setTitle( tr( "Current position" ) );
			delete m_hint;
			m_hint = textFloat::displayMessage( tr( "Hint" ),
					tr( "Press <Ctrl> for free "
							"positioning." ),
					embed::getIconPixmap( "hint" ), 0 );
		}
		else if( m_autoResize == FALSE )
		{
			m_action = Resize;
			m_oldTime = m_tco->length();
			QCursor c( Qt::SizeHorCursor );
			QApplication::setOverrideCursor( c );
			s_textFloat->setTitle( tr( "Current length" ) );
			delete m_hint;
			m_hint = textFloat::displayMessage( tr( "Hint" ),
					tr( "Press <Ctrl> for free "
							"resizing." ),
					embed::getIconPixmap( "hint" ), 0 );
		}
//		s_textFloat->reparent( this );
		// setup text-float as if TCO was already moved/resized
		mouseMoveEvent( _me );
		s_textFloat->show();
	}
	else if( _me->button() == Qt::MidButton )
	{
		if( engine::getMainWindow()->isCtrlPressed() )
		{
			m_tco->toggleMute();
		}
		else if( fixedTCOs() == FALSE )
		{
			remove();
		}
	}
}




void trackContentObjectView::mouseMoveEvent( QMouseEvent * _me )
{
	if( engine::getMainWindow()->isCtrlPressed() == TRUE )
	{
		delete m_hint;
		m_hint = NULL;
	}

	const float ppt = m_trackView->getTrackContainerView()->pixelsPerTact();
	if( m_action == Move )
	{
		const int x = mapToParent( _me->pos() ).x() - m_initialMouseX;
		midiTime t = tMax( 0, (Sint32)
			m_trackView->getTrackContainerView()->currentPosition()+
				static_cast<int>( x * DefaultTicksPerTact /
									ppt ) );
		if( engine::getMainWindow()->isCtrlPressed() ==
					FALSE && _me->button() == Qt::NoButton )
		{
			t = t.toNearestTact();
		}
		m_tco->movePosition( t );
		m_trackView->getTrackContentWidget()->changePosition();
		s_textFloat->setText( QString( "%1:%2" ).
				arg( m_tco->startPosition().getTact() + 1 ).
				arg( m_tco->startPosition().getTicks() ) );
		s_textFloat->moveGlobal( this, QPoint( width() + 2, 8 ) );
	}
	else if( m_action == MoveSelection )
	{
		const int dx = _me->x() - m_initialMouseX;
		QVector<selectableObject *> so =
			m_trackView->getTrackContainerView()->selectedObjects();
		QVector<trackContentObject *> tcos;
		midiTime smallest_pos;
		// find out smallest position of all selected objects for not
		// moving an object before zero
		for( QVector<selectableObject *>::iterator it = so.begin();
							it != so.end(); ++it )
		{
			trackContentObjectView * tcov =
				dynamic_cast<trackContentObjectView *>( *it );
			if( tcov == NULL )
			{
				continue;
			}
			trackContentObject * tco = tcov->m_tco;
			tcos.push_back( tco );
			smallest_pos = tMin<Sint32>( smallest_pos,
					(Sint32)tco->startPosition() +
					static_cast<int>( dx *
						DefaultTicksPerTact / ppt ) );
		}
		for( QVector<trackContentObject *>::iterator it = tcos.begin();
							it != tcos.end(); ++it )
		{
			( *it )->movePosition( ( *it )->startPosition() +
					static_cast<int>( dx *
						DefaultTicksPerTact / ppt ) -
								smallest_pos );
		}
	}
	else if( m_action == Resize )
	{
		midiTime t = tMax( DefaultTicksPerTact,
				static_cast<int>( _me->x() *
						DefaultTicksPerTact / ppt ) );
		if( engine::getMainWindow()->isCtrlPressed() ==
					FALSE && _me->button() == Qt::NoButton )
		{
			t = t.toNearestTact();
		}
		m_tco->changeLength( t );
		s_textFloat->setText( tr( "%1:%2 (%3:%4 to %5:%6)" ).
				arg( m_tco->length().getTact() ).
				arg( m_tco->length().getTicks() ).
				arg( m_tco->startPosition().getTact() + 1 ).
				arg( m_tco->startPosition().getTicks() ).
				arg( m_tco->endPosition().getTact() + 1 ).
				arg( m_tco->endPosition().getTicks() ) );
		s_textFloat->moveGlobal( this, QPoint( width() + 2, 8 ) );
	}
	else
	{
		if( _me->x() > width() - RESIZE_GRIP_WIDTH )
		{
			if( QApplication::overrideCursor() != NULL &&
				QApplication::overrideCursor()->shape() !=
							Qt::SizeHorCursor )
			{
				while( QApplication::overrideCursor() != NULL )
				{
					QApplication::restoreOverrideCursor();
				}
			}
			QCursor c( Qt::SizeHorCursor );
			QApplication::setOverrideCursor( c );
		}
		else
		{
			leaveEvent( NULL );
		}
	}
}




void trackContentObjectView::mouseReleaseEvent( QMouseEvent * _me )
{
	if( m_action == Move || m_action == Resize )
	{
		m_tco->setJournalling( TRUE );
		m_tco->addJournalEntry( journalEntry( m_action, m_oldTime -
			( ( m_action == Move ) ?
				m_tco->startPosition() : m_tco->length() ) ) );
	}
	m_action = NoAction;
	delete m_hint;
	m_hint = NULL;
	s_textFloat->hide();
	leaveEvent( NULL );
	selectableObject::mouseReleaseEvent( _me );
}




void trackContentObjectView::contextMenuEvent( QContextMenuEvent * _cme )
{
	QMenu contextMenu( this );
	if( fixedTCOs() == FALSE )
	{
		contextMenu.addAction( embed::getIconPixmap( "cancel" ),
					tr( "Delete (middle mousebutton)" ),
						this, SLOT( remove() ) );
		contextMenu.addSeparator();
		contextMenu.addAction( embed::getIconPixmap( "edit_cut" ),
					tr( "Cut" ), m_tco, SLOT( cut() ) );
	}
	contextMenu.addAction( embed::getIconPixmap( "edit_copy" ),
					tr( "Copy" ), m_tco, SLOT( copy() ) );
	contextMenu.addAction( embed::getIconPixmap( "edit_paste" ),
					tr( "Paste" ), m_tco, SLOT( paste() ) );
	contextMenu.addSeparator();
	contextMenu.addAction( embed::getIconPixmap( "muted" ),
				tr( "Mute/unmute (<Ctrl> + middle click)" ),
						m_tco, SLOT( toggleMute() ) );
	constructContextMenu( &contextMenu );

	contextMenu.exec( QCursor::pos() );
}




float trackContentObjectView::pixelsPerTact( void )
{
	return( m_trackView->getTrackContainerView()->pixelsPerTact() );
}




void trackContentObjectView::setAutoResizeEnabled( bool _e )
{
	m_autoResize = _e;
}





// ===========================================================================
// trackContentWidget
// ===========================================================================
trackContentWidget::trackContentWidget( trackView * _parent ) :
	QWidget( _parent ),
	m_trackView( _parent )
{
	//setAutoFillBackground( TRUE );
	//QPalette pal;
	//pal.setColor( backgroundRole(), QColor( 96, 96, 96 ) );
	//setPalette( pal );
	setAcceptDrops( TRUE );

	connect( _parent->getTrackContainerView(),
			SIGNAL( positionChanged( const midiTime & ) ),
			this, SLOT( changePosition( const midiTime & ) ) );
}




trackContentWidget::~trackContentWidget()
{
}




void trackContentWidget::addTCOView( trackContentObjectView * _tcov )
{
	trackContentObject * tco = _tcov->getTrackContentObject();
	QMap<QString, QVariant> map;
	map["id"] = tco->id();
	addJournalEntry( journalEntry( AddTrackContentObject, map ) );

	m_tcoViews.push_back( _tcov );

	tco->saveJournallingState( FALSE );
	changePosition();
	tco->restoreJournallingState();

}




void trackContentWidget::removeTCOView( trackContentObjectView * _tcov )
{
	tcoViewVector::iterator it = qFind( m_tcoViews.begin(),
						m_tcoViews.end(),
						_tcov );
	if( it != m_tcoViews.end() )
	{
		QMap<QString, QVariant> map;
		multimediaProject mmp( multimediaProject::JournalData );
		_tcov->getTrackContentObject()->saveState( mmp, mmp.content() );
		map["id"] = _tcov->getTrackContentObject()->id();
		map["state"] = mmp.toString();
		addJournalEntry( journalEntry( RemoveTrackContentObject,
								map ) );

		m_tcoViews.erase( it );
		engine::getSong()->setModified();
	}
}




void trackContentWidget::update( void )
{
	for( tcoViewVector::iterator it = m_tcoViews.begin();
				it != m_tcoViews.end(); ++it )
	{
		( *it )->setFixedHeight( height() - 2 );
		( *it )->update();
	}
	QWidget::update();
}




// resposible for moving track-content-widgets to appropriate position after
// change of visible viewport
void trackContentWidget::changePosition( const midiTime & _new_pos )
{
//	const int tcos = numOfTCOs();


	if( m_trackView->getTrackContainerView() == engine::getBBEditor() )
	{
		const int cur_bb = engine::getBBTrackContainer()->currentBB();
		int i = 0;
		// first show TCO for current BB...
		for( tcoViewVector::iterator it = m_tcoViews.begin();
					it != m_tcoViews.end(); ++it, ++i )
		{
			if( i == cur_bb )
			{
				( *it )->move( 0, ( *it )->y() );
				( *it )->raise();
				( *it )->show();
			}
			else
			{
				( *it )->lower();
			}
		}
		// ...then hide others to avoid flickering
		i = 0;
		for( tcoViewVector::iterator it = m_tcoViews.begin();
					it != m_tcoViews.end(); ++it, ++i )
		{
			if( i != cur_bb )
			{
				( *it )->hide();
			}
		}
		return;
	}

	midiTime pos = _new_pos;
	if( pos < 0 )
	{
		pos = m_trackView->getTrackContainerView()->currentPosition();
	}

	const Sint32 begin = pos;
	const Sint32 end = endPosition( pos );
	const float ppt = m_trackView->getTrackContainerView()->pixelsPerTact();

	for( tcoViewVector::iterator it = m_tcoViews.begin();
						it != m_tcoViews.end(); ++it )
	{
		trackContentObjectView * tcov = *it;
		trackContentObject * tco = tcov->getTrackContentObject();
		tco->changeLength( tco->length() );
		Sint32 ts = tco->startPosition();
		Sint32 te = tco->endPosition();
		if( ( ts >= begin && ts <= end ) ||
			( te >= begin && te <= end ) ||
			( ts <= begin && te >= end ) )
		{
			tcov->move( static_cast<int>( ( ts - begin ) * ppt /
							DefaultTicksPerTact ),
								tcov->y() );
			tcov->show();
		}
		else
		{
			tcov->hide();
		}
	}

	// redraw backgroun
	update();
}




void trackContentWidget::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "tco_" +
					QString::number( getTrack()->type() ) );
}




void trackContentWidget::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == ( "tco_" + QString::number( getTrack()->type() ) ) &&
		m_trackView->getTrackContainerView()->fixedTCOs() == FALSE )
	{
		const midiTime pos = getPosition( _de->pos().x()
							).toNearestTact();
		trackContentObject * tco = getTrack()->addTCO(
						getTrack()->createTCO( pos ) );

		// value contains our XML-data so simply create a
		// multimediaProject which does the rest for us...
		multimediaProject mmp( value, FALSE );
		// at least save position before getting moved to somewhere
		// the user doesn't expect...
		tco->restoreState( mmp.content().firstChild().toElement() );
		tco->movePosition( pos );


		_de->accept();
	}
}






void trackContentWidget::mousePressEvent( QMouseEvent * _me )
{
	if( m_trackView->getTrackContainerView()->allowRubberband() == TRUE )
	{
		QWidget::mousePressEvent( _me );
	}
	else if( engine::getMainWindow()->isShiftPressed() == TRUE )
	{
		QWidget::mousePressEvent( _me );
	}
	else if( _me->button() == Qt::LeftButton &&
			!m_trackView->getTrackContainerView()->fixedTCOs() )
	{
		const midiTime pos = getPosition( _me->x() ).getTact() *
							DefaultTicksPerTact;
		trackContentObject * tco = getTrack()->addTCO(
						getTrack()->createTCO( pos ) );

		tco->saveJournallingState( FALSE );
		tco->movePosition( pos );
		tco->restoreJournallingState();

	}
}




void trackContentWidget::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	//p.fillRect( rect(), QColor( 23, 34, 37 ) );

	const trackContainerView * tcv = m_trackView->getTrackContainerView();
	bool flip = TRUE;
	if( !tcv->fixedTCOs() )
	{
		const int offset = (int)( ( tcv->currentPosition() % 4 ) *
							tcv->pixelsPerTact() );

		flip = tcv->currentPosition() % 256 < 128;
		int flipper = (tcv->currentPosition()/DefaultTicksPerTact) % 8;

		for( int x = 0; x < width(); x+= (int) tcv->pixelsPerTact() ) {
			p.fillRect( QRect(x, 0, 
			(int) tcv->pixelsPerTact(), height()),
				(flipper<4) ?
				QColor( 56, 80, 88 ) :
				QColor( 49, 71, 77 ));
			flipper = (flipper+1)%8;
		}

		// draw vertical lines
		p.setPen( QColor( 54, 65, 69 ) );
		for( int x = -offset; x < width();
					x += (int) tcv->pixelsPerTact() )
		{
			p.drawLine( x, 0, x, height() );
		}
	}
}




void trackContentWidget::resizeEvent( QResizeEvent * _re )
{
	update();
}




void trackContentWidget::undoStep( journalEntry & _je )
{
	saveJournallingState( FALSE );
	switch( _je.actionID() )
	{
		case AddTrackContentObject:
		{
			QMap<QString, QVariant> map = _je.data().toMap();
			trackContentObject * tco =
				dynamic_cast<trackContentObject *>(
			engine::getProjectJournal()->getJournallingObject(
							map["id"].toInt() ) );
			multimediaProject mmp(
					multimediaProject::JournalData );
			tco->saveState( mmp, mmp.content() );
			map["state"] = mmp.toString();
			_je.data() = map;
			tco->deleteLater();
			break;
		}

		case RemoveTrackContentObject:
		{
			trackContentObject * tco =
				getTrack()->addTCO( getTrack()->createTCO(
							midiTime( 0 ) ) );
			multimediaProject mmp(
				_je.data().toMap()["state"].toString(), FALSE );
			tco->restoreState(
				mmp.content().firstChild().toElement() );
			break;
		}
	}
	restoreJournallingState();
}




void trackContentWidget::redoStep( journalEntry & _je )
{
	switch( _je.actionID() )
	{
		case AddTrackContentObject:
		case RemoveTrackContentObject:
			_je.actionID() = ( _je.actionID() ==
						AddTrackContentObject ) ?
				RemoveTrackContentObject :
						AddTrackContentObject;
			undoStep( _je );
			_je.actionID() = ( _je.actionID() ==
						AddTrackContentObject ) ?
				RemoveTrackContentObject :
						AddTrackContentObject;
			break;
	}
}




track * trackContentWidget::getTrack( void )
{
	return( m_trackView->getTrack() );
}




midiTime trackContentWidget::getPosition( int _mouse_x )
{
	return( midiTime( m_trackView->getTrackContainerView()->
					currentPosition() + _mouse_x *
							DefaultTicksPerTact /
			static_cast<int>( m_trackView->
				getTrackContainerView()->pixelsPerTact() ) ) );
}



midiTime trackContentWidget::endPosition( const midiTime & _pos_start )
{
	const float ppt = m_trackView->getTrackContainerView()->pixelsPerTact();
	const int w = width();
	return( _pos_start + static_cast<int>( w * DefaultTicksPerTact /
									ppt ) );
}






// ===========================================================================
// trackOperationsWidget
// ===========================================================================


QPixmap * trackOperationsWidget::s_grip = NULL;
QPixmap * trackOperationsWidget::s_muteOffDisabled;
QPixmap * trackOperationsWidget::s_muteOffEnabled;
QPixmap * trackOperationsWidget::s_muteOnDisabled;
QPixmap * trackOperationsWidget::s_muteOnEnabled;


trackOperationsWidget::trackOperationsWidget( trackView * _parent ) :
	QWidget( _parent ),
	m_trackView( _parent ),
	m_automationDisabled( FALSE )
{
	if( s_grip == NULL )
	{
		s_grip = new QPixmap( embed::getIconPixmap(
							"track_op_grip" ) );
		s_muteOffDisabled = new QPixmap( embed::getIconPixmap(
							"mute_off_disabled" ) );
		s_muteOffEnabled = new QPixmap( embed::getIconPixmap(
							"mute_off" ) );
		s_muteOnDisabled = new QPixmap( embed::getIconPixmap(
							"mute_on_disabled" ) );
		s_muteOnEnabled = new QPixmap( embed::getIconPixmap(
							"mute_on" ) );
	}

	toolTip::add( this, tr( "Press <Ctrl> while clicking on move-grip "
				"to begin a new drag'n'drop-action." ) );

	QMenu * to_menu = new QMenu( this );
	to_menu->setFont( pointSize<9>( to_menu->font() ) );
	connect( to_menu, SIGNAL( aboutToShow() ), this, SLOT( updateMenu() ) );


	setObjectName( "automationEnabled" );


	m_trackOps = new QPushButton( this );
	m_trackOps->move( 12, 1 );
	m_trackOps->setMenu( to_menu );
	toolTip::add( m_trackOps, tr( "Actions for this track" ) );


	m_muteBtn = new pixmapButton( this, tr( "Mute" ) );
	m_muteBtn->setActiveGraphic( *s_muteOnEnabled );
	m_muteBtn->setInactiveGraphic( *s_muteOffEnabled );
	m_muteBtn->setCheckable( TRUE );
	m_muteBtn->move( 44, 4 );
	m_muteBtn->show();
	connect( m_muteBtn, SIGNAL( toggled( bool ) ), this,
						SLOT( setMuted( bool ) ) );
	connect( m_muteBtn, SIGNAL( clickedRight() ), this,
					SLOT( muteBtnRightClicked() ) );
	m_muteBtn->setWhatsThis(
		tr( "With this switch you can either mute this track or mute "
			"all other tracks.\nBy clicking left, this track is "
			"muted. This is useful, if you only want to listen to "
			"the other tracks without changing this track "
			"and loosing information.\nWhen you click right on "
			"this switch, all other tracks will be "
			"muted. This is useful, if you only want to listen to "
			"this track." ) );
	toolTip::add( m_muteBtn, tr( "left click = mute this track\n"
			"right click = mute all other tracks (solo)" ) );

	if( inBBEditor() )
	{
		connect( engine::getBBEditor(),
				SIGNAL( positionChanged( const midiTime & ) ),
				this, SLOT( update() ) );
	}
}




trackOperationsWidget::~trackOperationsWidget()
{
}




bool trackOperationsWidget::muted( void ) const
{
	return( m_muteBtn->model()->value() );
}




void trackOperationsWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
		engine::getMainWindow()->isCtrlPressed() == TRUE &&
			m_trackView->getTrack()->type() != track::BBTrack )
	{
		multimediaProject mmp( multimediaProject::DragNDropData );
		m_trackView->getTrack()->saveState( mmp, mmp.content() );
		new stringPairDrag( QString( "track_%1" ).arg(
					m_trackView->getTrack()->type() ),
			mmp.toString(), QPixmap::grabWidget(
				m_trackView->getTrackSettingsWidget() ),
									this );
	}
	else if( _me->button() == Qt::LeftButton )
	{
		// track-widget (parent-widget) initiates track-move
		_me->ignore();
	}
}





void trackOperationsWidget::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.fillRect( rect(), QColor( 128, 128, 128 ) );
	if( m_trackView->isMovingTrack() == FALSE )
	{
		p.drawPixmap( 2, 2, *s_grip );
		if( inBBEditor() )
		{
			bbTrack * bb_track = currentBBTrack();
			if( !bb_track || bb_track->automationDisabled(
						m_trackView->getTrack() ) )
			{
				if( !m_automationDisabled )
				{
					m_automationDisabled = TRUE;
					setObjectName( "automationDisabled" );
					setStyle( NULL );
					m_muteBtn->setActiveGraphic(
							*s_muteOnDisabled );
					m_muteBtn->setInactiveGraphic(
							*s_muteOffDisabled );
				}
			}
			else
			{
				if( m_automationDisabled )
				{
					m_automationDisabled = FALSE;
					setObjectName( "automationEnabled" );
					setStyle( NULL );
					m_muteBtn->setActiveGraphic(
							*s_muteOnEnabled );
					m_muteBtn->setInactiveGraphic(
							*s_muteOffEnabled );
				}
			}
		}
		m_trackOps->show();
		m_muteBtn->show();
	}
	else
	{
		m_trackOps->hide();
		m_muteBtn->hide();
	}
}




void trackOperationsWidget::cloneTrack( void )
{
	engine::getMixer()->lock();
	m_trackView->getTrack()->clone();
	engine::getMixer()->unlock();
}




void trackOperationsWidget::removeTrack( void )
{
	m_trackView->close();
	delete m_trackView;
	engine::getMixer()->lock();
	delete m_trackView->getTrack();
	engine::getMixer()->unlock();
}




void trackOperationsWidget::setMuted( bool _muted )
{
	m_muteBtn->setChecked( _muted );
	m_trackView->getTrackContentWidget()->update();
}




void trackOperationsWidget::muteBtnRightClicked( void )
{
	const bool m = muted();	// next function might modify our mute-state,
				// so save it now
	m_trackView->getTrack()->getTrackContainer()->
						setMutedOfAllTracks( m );
	setMuted( !m );
}




void trackOperationsWidget::updateMenu( void )
{
	QMenu * to_menu = m_trackOps->menu();
	to_menu->clear();
	if( inBBEditor() )
	{
		bbTrack * bb_track = currentBBTrack();
		if( bb_track )
		{
			if( bb_track->automationDisabled(
						m_trackView->getTrack() ) )
			{
				to_menu->addAction( embed::getIconPixmap(
							"led_off", 16, 16 ),
					tr( "Enable automation" ),
					this, SLOT( enableAutomation() ) );
			}
			else
			{
				to_menu->addAction( embed::getIconPixmap(
							"led_green", 16, 16 ),
					tr( "Disable automation" ),
					this, SLOT( disableAutomation() ) );
			}
		}
	}
	to_menu->addAction( embed::getIconPixmap( "edit_copy", 16, 16 ),
						tr( "Clone this track" ),
						this, SLOT( cloneTrack() ) );
	to_menu->addAction( embed::getIconPixmap( "cancel", 16, 16 ),
						tr( "Remove this track" ),
						this, SLOT( removeTrack() ) );
}




void trackOperationsWidget::enableAutomation( void )
{
	currentBBTrack()->enableAutomation( m_trackView->getTrack() );
}




void trackOperationsWidget::disableAutomation( void )
{
	currentBBTrack()->disableAutomation( m_trackView->getTrack() );
}




bbTrack * trackOperationsWidget::currentBBTrack( void )
{
	return( bbTrack::findBBTrack(
				engine::getBBTrackContainer()->currentBB() ) );
}




bool trackOperationsWidget::inBBEditor( void )
{
	return( m_trackView->getTrackContainerView()
						== engine::getBBEditor() );
}






// ===========================================================================
// track
// ===========================================================================

track::track( TrackTypes _type, trackContainer * _tc ) :
	model( _tc ),
	m_trackContainer( _tc ),
	m_type( _type ),
	m_name(),
	m_pixmap( NULL ),
	m_mutedModel( FALSE, this ),
	m_trackContentObjects(),
	m_automationPatterns()
{
	m_mutedModel.setTrack( this );
	m_trackContainer->addTrack( this );
}




track::~track()
{
	if( m_trackContainer == engine::getBBTrackContainer()
						&& engine::getSong() )
	{
		QList<track *> tracks = engine::getSong()->tracks();
		for( int i = 0; i < tracks.size(); ++i )
		{
			if( tracks[i]->type() == BBTrack )
			{
				bbTrack * bb_track = (bbTrack *)tracks[i];
				if( bb_track->automationDisabled( this ) )
				{
					// Remove reference from bbTrack
					bb_track->enableAutomation( this );
				}
			}
		}
	}

	while( !m_trackContentObjects.isEmpty() )
	{
		delete m_trackContentObjects.last();
	}

	m_trackContainer->removeTrack( this );

	for( QList<automationPattern *>::iterator it =
						m_automationPatterns.begin();
					it != m_automationPatterns.end();
					++it )
	{
		( *it )->forgetTrack();
	}
}




track * track::create( TrackTypes _tt, trackContainer * _tc )
{
	track * t = NULL;

	switch( _tt )
	{
		case InstrumentTrack: t = new instrumentTrack( _tc ); break;
		case BBTrack: t = new bbTrack( _tc ); break;
//		case SampleTrack: t = new sampleTrack( _tc ); break;
//		case EVENT_TRACK:
//		case VIDEO_TRACK:
		case AutomationTrack: t = new automationTrack( _tc ); break;
		default: break;
	}

	_tc->updateAfterTrackAdd();

	return( t );
}




track * track::create( const QDomElement & _this, trackContainer * _tc )
{
	track * t = create(
		static_cast<TrackTypes>( _this.attribute( "type" ).toInt() ),
									_tc );
	if( t != NULL )
	{
		t->restoreState( _this );
	}
	return( t );
}




void track::clone( void )
{
	QDomDocument doc;
	QDomElement parent = doc.createElement( "clone" );
	saveState( doc, parent );
	create( parent.firstChild().toElement(), m_trackContainer );
}






void track::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setTagName( "track" );
	_this.setAttribute( "type", type() );
	_this.setAttribute( "muted", muted() );
// ### TODO
//	_this.setAttribute( "height", m_trackView->height() );

	QDomElement ts_de = _doc.createElement( nodeName() );
	// let actual track (instrumentTrack, bbTrack, sampleTrack etc.) save
	// its settings
	_this.appendChild( ts_de );
	saveTrackSpecificSettings( _doc, ts_de );

	// now save settings of all TCO's
	for( tcoVector::const_iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		( *it )->saveState( _doc, _this );
	}
}




void track::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "type" ).toInt() != type() )
	{
		qWarning( "Current track-type does not match track-type of "
							"settings-node!\n" );
	}

	setMuted( _this.attribute( "muted" ).toInt() );

	while( !m_trackContentObjects.empty() )
	{
		delete m_trackContentObjects.front();
		m_trackContentObjects.erase( m_trackContentObjects.begin() );
	}

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.nodeName() == nodeName() )
			{
				loadTrackSpecificSettings( node.toElement() );
			}
			else if(
			!node.toElement().attribute( "metadata" ).toInt() )
			{
				trackContentObject * tco = createTCO(
								midiTime( 0 ) );
				tco->restoreState( node.toElement() );
				saveJournallingState( FALSE );
				addTCO( tco );
				restoreJournallingState();
			}
		}
		node = node.nextSibling();
        }
/*
	if( _this.attribute( "height" ).toInt() >= MINIMAL_TRACK_HEIGHT )
	{
		m_trackView->setFixedHeight(
					_this.attribute( "height" ).toInt() );
	}*/
}




trackContentObject * track::addTCO( trackContentObject * _tco )
{
	m_trackContentObjects.push_back( _tco );

//	emit dataChanged();
	emit trackContentObjectAdded( _tco );

	//engine::getSongEditor()->setModified();
	return( _tco );		// just for convenience
}




void track::removeTCO( trackContentObject * _tco )
{
	tcoVector::iterator it = qFind( m_trackContentObjects.begin(),
					m_trackContentObjects.end(),
					_tco );
	if( it != m_trackContentObjects.end() )
	{
		m_trackContentObjects.erase( it );
		engine::getSong()->setModified();
	}
}




int track::numOfTCOs( void )
{
	return( m_trackContentObjects.size() );
}




trackContentObject * track::getTCO( int _tco_num )
{
	if( _tco_num < m_trackContentObjects.size() )
	{
		return( m_trackContentObjects[_tco_num] );
	}
	printf( "called track::getTCO( %d ), "
			"but TCO %d doesn't exist\n", _tco_num, _tco_num );
	return( addTCO( createTCO( _tco_num * DefaultTicksPerTact ) ) );
	
}




int track::getTCONum( trackContentObject * _tco )
{
//	for( int i = 0; i < getTrackContentWidget()->numOfTCOs(); ++i )
	tcoVector::iterator it = qFind( m_trackContentObjects.begin(),
					m_trackContentObjects.end(),
					_tco );
	if( it != m_trackContentObjects.end() )
	{
/*		if( getTCO( i ) == _tco )
		{
			return( i );
		}*/
		return( it - m_trackContentObjects.begin() );
	}
	qWarning( "track::getTCONum(...) -> _tco not found!\n" );
	return( 0 );
}




void track::getTCOsInRange( QList<trackContentObject *> & _tco_v,
							const midiTime & _start,
							const midiTime & _end )
{
	for( tcoVector::iterator it_o = m_trackContentObjects.begin();
				it_o != m_trackContentObjects.end(); ++it_o )
	{
		trackContentObject * tco = ( *it_o );//getTCO( i );
		Sint32 s = tco->startPosition();
		Sint32 e = tco->endPosition();
		if( ( s <= _end ) && ( e >= _start ) )
		{
			// ok, TCO is posated within given range
			// now let's search according position for TCO in list
			// 	-> list is ordered by TCO's position afterwards
			bool inserted = FALSE;
			for( QList<trackContentObject *>::iterator it =
								_tco_v.begin();
						it != _tco_v.end(); ++it )
			{
				if( ( *it )->startPosition() >= s )
				{
					_tco_v.insert( it, tco );
					inserted = TRUE;
					break;
				}
			}
			if( inserted == FALSE )
			{
				// no TCOs found posated behind current TCO...
				_tco_v.push_back( tco );
			}
		}
	}
}




void track::swapPositionOfTCOs( int _tco_num1, int _tco_num2 )
{
	// TODO: range-checking
	qSwap( m_trackContentObjects[_tco_num1],
					m_trackContentObjects[_tco_num2] );

	const midiTime pos = m_trackContentObjects[_tco_num1]->startPosition();

	m_trackContentObjects[_tco_num1]->movePosition(
			m_trackContentObjects[_tco_num2]->startPosition() );
	m_trackContentObjects[_tco_num2]->movePosition( pos );
}




void track::insertTact( const midiTime & _pos )
{
	// we'll increase the position of every TCO, posated behind _pos, by
	// one tact
	for( tcoVector::iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		if( ( *it )->startPosition() >= _pos )
		{
			( *it )->movePosition( (*it)->startPosition() +
							DefaultTicksPerTact );
		}
	}
}




void track::removeTact( const midiTime & _pos )
{
	// we'll decrease the position of every TCO, posated behind _pos, by
	// one tact
	for( tcoVector::iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		if( ( *it )->startPosition() >= _pos )
		{
			( *it )->movePosition( tMax( ( *it )->startPosition() -
						DefaultTicksPerTact, 0 ) );
		}
	}
}




tact track::length( void ) const
{
	// find last end-position
	Sint32 last = 0;
	for( tcoVector::const_iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		const Sint32 cur = ( *it )->endPosition();
		if( cur > last )
		{
			last = cur;
		}
	}
	return( last/DefaultTicksPerTact + 1 );
}



void track::addAutomationPattern( automationPattern * _pattern )
{
	m_automationPatterns.append( _pattern );
}




void track::removeAutomationPattern( automationPattern * _pattern )
{
	m_automationPatterns.removeAll( _pattern );
}




void track::sendMidiTime( const midiTime & _time )
{
	for( QList<automationPattern *>::iterator it =
						m_automationPatterns.begin();
					it != m_automationPatterns.end();
					++it )
	{
		( *it )->processMidiTime( _time );
	}
}






// ===========================================================================
// trackView
// ===========================================================================

trackView::trackView( track * _track, trackContainerView * _tcv ) :
	QWidget( _tcv->contentWidget() ),
	modelView( NULL/*_track*/ ),
	m_track( _track ),
	m_trackContainerView( _tcv ),
	m_trackOperationsWidget( this ),
	m_trackSettingsWidget( this ),
	m_trackContentWidget( this ),
	m_action( NoAction )
{
	m_trackOperationsWidget.setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setColor( m_trackOperationsWidget.backgroundRole(),
						QColor( 128, 128, 128 ) );
	m_trackOperationsWidget.setPalette( pal );


	m_trackSettingsWidget.setAutoFillBackground( TRUE );
	pal.setColor( m_trackSettingsWidget.backgroundRole(),
							QColor( 64, 64, 64 ) );
	m_trackSettingsWidget.setPalette( pal );

	QHBoxLayout * layout = new QHBoxLayout( this );
	layout->setMargin( 0 );
	layout->setSpacing( 0 );
	layout->addWidget( &m_trackOperationsWidget );
	layout->addWidget( &m_trackSettingsWidget );
	layout->addWidget( &m_trackContentWidget, 1 );

	resizeEvent( NULL );

	setAcceptDrops( TRUE );
	setAttribute( Qt::WA_DeleteOnClose );


	connect( m_track, SIGNAL( destroyed( QObject * ) ),
			this, SLOT( close() ), Qt::QueuedConnection );
	connect( m_track,
		SIGNAL( trackContentObjectAdded( trackContentObject * ) ),
			this, SLOT( createTCOView( trackContentObject * ) ),
			Qt::QueuedConnection );

	// create views for already existing TCOs
	for( track::tcoVector::iterator it =
					m_track->m_trackContentObjects.begin();
			it != m_track->m_trackContentObjects.end(); ++it )
	{
		createTCOView( *it );
	}
//	setModel( m_track );

	m_trackContainerView->addTrackView( this );
}




trackView::~trackView()
{
}




void trackView::resizeEvent( QResizeEvent * _re )
{
	m_trackOperationsWidget.setFixedSize( TRACK_OP_WIDTH, height() - 1 );
	m_trackSettingsWidget.setFixedSize( DEFAULT_SETTINGS_WIDGET_WIDTH,
								height() - 1 );
	m_trackContentWidget.setFixedHeight( height() - 1 );
}




void trackView::update( void )
{
	m_trackContentWidget.update();
	if( !m_trackContainerView->fixedTCOs() )
	{
		m_trackContentWidget.changePosition();
	}
	QWidget::update();
}




bool trackView::close( void )
{
	m_trackContainerView->removeTrackView( this );
	return( QWidget::close() );
}




void trackView::modelChanged( void )
{
	m_track = castModel<track>();
	assert( m_track != NULL );
	connect( m_track, SIGNAL( destroyed( QObject * ) ),
			this, SLOT( close() ), Qt::QueuedConnection );
	m_trackOperationsWidget.m_muteBtn->setModel( &m_track->m_mutedModel );
	modelView::modelChanged();
}




void trackView::undoStep( journalEntry & _je )
{
	saveJournallingState( FALSE );
	switch( _je.actionID() )
	{
		case MoveTrack:
			if( _je.data().toInt() > 0 )
			{
				m_trackContainerView->moveTrackViewUp( this );
			}
			else
			{
				m_trackContainerView->moveTrackViewDown( this );
			}
			break;
		case ResizeTrack:
			setFixedHeight( tMax<int>( height() +
						_je.data().toInt(),
						MINIMAL_TRACK_HEIGHT ) );
			m_trackContainerView->realignTracks();
			break;
	}
	restoreJournallingState();
}




void trackView::redoStep( journalEntry & _je )
{
	journalEntry je( _je.actionID(), -_je.data().toInt() );
	undoStep( je );
}




void trackView::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "track_" +
					QString::number( m_track->type() ) );
}




void trackView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == ( "track_" + QString::number( m_track->type() ) ) )
	{
		// value contains our XML-data so simply create a
		// multimediaProject which does the rest for us...
		multimediaProject mmp( value, FALSE );
		engine::getMixer()->lock();
		m_track->restoreState( mmp.content().firstChild().toElement() );
		engine::getMixer()->unlock();
		_de->accept();
	}
}




void trackView::mousePressEvent( QMouseEvent * _me )
{
	if( m_trackContainerView->allowRubberband() == TRUE )
	{
		QWidget::mousePressEvent( _me );
	}
	else if( _me->button() == Qt::LeftButton )
	{
		if( engine::getMainWindow()->isShiftPressed() == TRUE )
		{
			m_action = ResizeTrack;
			QCursor::setPos( mapToGlobal( QPoint( _me->x(),
								height() ) ) );
			QCursor c( Qt::SizeVerCursor);
			QApplication::setOverrideCursor( c );
		}
		else
		{
			m_action = MoveTrack;

			QCursor c( Qt::SizeAllCursor );
			QApplication::setOverrideCursor( c );
			// update because in move-mode, all elements in
			// track-op-widgets are hidden as a visual feedback
			m_trackOperationsWidget.update();
		}

		_me->accept();
	}
	else
	{
		QWidget::mousePressEvent( _me );
	}
}




void trackView::mouseMoveEvent( QMouseEvent * _me )
{
	if( m_trackContainerView->allowRubberband() == TRUE )
	{
		QWidget::mouseMoveEvent( _me );
	}
	else if( m_action == MoveTrack )
	{
		// look which track-widget the mouse-cursor is over
		const trackView * track_at_y =
			m_trackContainerView->trackViewAt(
				mapTo( m_trackContainerView->contentWidget(),
							_me->pos() ).y() );
		// a track-widget not equal to ourself?
		if( track_at_y != NULL && track_at_y != this )
		{
			// then move us up/down there!
			if( _me->y() < 0 )
			{
				m_trackContainerView->moveTrackViewUp( this );
			}
			else
			{
				m_trackContainerView->moveTrackViewDown( this );
			}
			addJournalEntry( journalEntry( MoveTrack, _me->y() ) );
		}
	}
	else if( m_action == ResizeTrack )
	{
		setFixedHeight( tMax<int>( _me->y(), MINIMAL_TRACK_HEIGHT ) );
		m_trackContainerView->realignTracks();
	}
}




void trackView::mouseReleaseEvent( QMouseEvent * _me )
{
	m_action = NoAction;
	while( QApplication::overrideCursor() != NULL )
	{
		QApplication::restoreOverrideCursor();
	}
	m_trackOperationsWidget.update();

	QWidget::mouseReleaseEvent( _me );
}




void trackView::paintEvent( QPaintEvent * _pe )
{
	QStyleOption opt;
	opt.initFrom( this );
	QPainter p( this );
	style()->drawPrimitive( QStyle::PE_Widget, &opt, &p, this );
}




void trackView::createTCOView( trackContentObject * _tco )
{
	_tco->createView( this );
}





#include "track.moc"


#endif
