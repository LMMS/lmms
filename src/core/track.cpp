#ifndef SINGLE_SOURCE_COMPILE

/*
 * track.cpp - implementation of classes concerning tracks -> neccessary for
 *             all track-like objects (beat/bassline, sample-track...)
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


#include <Qt/QtXml>
#include <QtGui/QApplication>
#include <QtGui/QLayout>
#include <QtGui/QMenu>
#include <QtGui/QMouseEvent>


#include "track.h"
#include "automation_pattern.h"
#include "track_container.h"
#include "automation_track.h"
#include "instrument_track.h"
#include "bb_editor.h"
#include "bb_track.h"
#include "sample_track.h"
#include "song_editor.h"
#include "templates.h"
#include "clipboard.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "pixmap_button.h"
#include "debug.h"
#include "tooltip.h"
#include "string_pair_drag.h"
#include "mmp.h"
#include "main_window.h"
#include "text_float.h"
#include "project_journal.h"


const Sint16 RESIZE_GRIP_WIDTH = 4;

const Uint16 TRACK_OP_BTN_WIDTH = 20;
const Uint16 TRACK_OP_BTN_HEIGHT = 14;

const Uint16 MINIMAL_TRACK_HEIGHT = 32;


textFloat * trackContentObject::s_textFloat = NULL;


// ===========================================================================
// trackContentObject
// ===========================================================================
trackContentObject::trackContentObject( track * _track ) :
	selectableObject( _track->getTrackContentWidget() ),
	m_track( _track ),
	m_startPosition(),
	m_length(),
	m_action( NONE ),
	m_autoResize( FALSE ),
	m_initialMouseX( 0 ),
	m_muted( FALSE ),
	m_hint( NULL )
{
	if( s_textFloat == NULL )
	{
		s_textFloat = new textFloat( this );
		s_textFloat->setPixmap( embed::getIconPixmap( "clock" ) );
	}

	setAttribute( Qt::WA_DeleteOnClose );
	setFocusPolicy( Qt::StrongFocus );
	show();

	setJournalling( FALSE );
	movePosition( 0 );
	changeLength( 0 );
	setJournalling( TRUE );

	setFixedHeight( parentWidget()->height() - 2 );
	setAcceptDrops( TRUE );
	setMouseTracking( TRUE );
}




trackContentObject::~trackContentObject()
{
	delete m_hint;
}




bool trackContentObject::fixedTCOs( void )
{
	return( m_track->getTrackContainer()->fixedTCOs() );
}




void trackContentObject::movePosition( const midiTime & _pos )
{
	if( m_startPosition != _pos )
	{
		//engine::getSongEditor()->setModified();
		addJournalEntry( journalEntry( MOVE, m_startPosition - _pos ) );
		m_startPosition = _pos;
	}
	m_track->getTrackWidget()->changePosition();
	// moving a TCO can result in change of song-length etc.,
	// therefore we update the track-container
	m_track->getTrackContainer()->update();
}




void trackContentObject::changeLength( const midiTime & _length )
{
	if( m_length != _length )
	{
		//engine::getSongEditor()->setModified();
		addJournalEntry( journalEntry( RESIZE, m_length - _length ) );
		m_length = _length;
	}
	setFixedWidth( static_cast<int>( m_length * pixelsPerTact() /
					64 ) + TCO_BORDER_WIDTH * 2 );
	// changing length of TCO can result in change of song-length etc.,
	// therefore we update the track-container
	m_track->getTrackContainer()->update();
}




void trackContentObject::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "tco_" +
					QString::number( m_track->type() ) );
}




void trackContentObject::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == ( "tco_" + QString::number( m_track->type() ) ) )
	{
		// value contains our XML-data so simply create a
		// multimediaProject which does the rest for us...
		multimediaProject mmp( value, FALSE );
		// at least save position before getting to moved to somewhere
		// the user doesn't expect...
		midiTime pos = startPosition();
		restoreState( mmp.content().firstChild().toElement() );
		movePosition( pos );
		_de->accept();
	}
}




void trackContentObject::leaveEvent( QEvent * _e )
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




void trackContentObject::mousePressEvent( QMouseEvent * _me )
{
	if( m_track->getTrackContainer()->allowRubberband() == TRUE &&
					_me->button() == Qt::LeftButton )
	{
		// if rubberband is active, we can be selected
		if( m_track->getTrackContainer()->rubberBandActive() == FALSE )
		{
			if( engine::getMainWindow()->isCtrlPressed() == TRUE )
			{
				setSelected( !isSelected() );
			}
			else if( isSelected() == TRUE )
			{
				m_action = MOVE_SELECTION;
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
		multimediaProject mmp( multimediaProject::DRAG_N_DROP_DATA );
		saveState( mmp, mmp.content() );
		QPixmap thumbnail = QPixmap::grabWidget( this ).scaled(
						128, 128,
						Qt::KeepAspectRatio,
						Qt::SmoothTransformation );
		new stringPairDrag( QString( "tco_%1" ).arg( m_track->type() ),
					mmp.toString(), thumbnail, this );
	}
	else if( _me->button() == Qt::LeftButton &&
		/*	engine::getMainWindow()->isShiftPressed() == FALSE &&*/
							fixedTCOs() == FALSE )
	{
		// move or resize
		setJournalling( FALSE );

		m_initialMouseX = _me->x();

		if( _me->x() < width() - RESIZE_GRIP_WIDTH )
		{
			m_action = MOVE;
			m_oldTime = m_startPosition;
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
			m_action = RESIZE;
			m_oldTime = m_length;
			QCursor c( Qt::SizeHorCursor );
			QApplication::setOverrideCursor( c );
			s_textFloat->setTitle( tr( "Current length" ) );
			delete m_hint;
			m_hint = textFloat::displayMessage( tr( "Hint" ),
					tr( "Press <Ctrl> for free "
							"resizing." ),
					embed::getIconPixmap( "hint" ), 0 );
		}
		s_textFloat->reparent( this );
		// setup text-float as if TCO was already moved/resized
		mouseMoveEvent( _me );
		s_textFloat->show();
	}
	else if( _me->button() == Qt::MidButton )
	{
		if( engine::getMainWindow()->isCtrlPressed() )
		{
			toggleMute();
		}
		else if( fixedTCOs() == FALSE )
		{
			// delete ourself
			close();
		}
	}
}




void trackContentObject::mouseMoveEvent( QMouseEvent * _me )
{
	if( engine::getMainWindow()->isCtrlPressed() == TRUE )
	{
		delete m_hint;
		m_hint = NULL;
	}

	const float ppt = m_track->getTrackContainer()->pixelsPerTact();
	if( m_action == MOVE )
	{
		const int x = mapToParent( _me->pos() ).x() - m_initialMouseX;
		midiTime t = tMax( 0, (Sint32) m_track->getTrackContainer()->
							currentPosition() +
					static_cast<int>( x * 64 / ppt ) );
		if( engine::getMainWindow()->isCtrlPressed() ==
					FALSE && _me->button() == Qt::NoButton )
		{
			t = t.toNearestTact();
		}
		movePosition( t );
		m_track->getTrackWidget()->changePosition();
		s_textFloat->setText( QString( "%1:%2" ).
					arg( m_startPosition.getTact() + 1 ).
					arg( m_startPosition.getTact64th() ) );
		s_textFloat->move( mapTo( topLevelWidget(), QPoint( 0, 0 ) ) +
				QPoint( -2 - s_textFloat->width(), 8 ) );
	}
	else if( m_action == MOVE_SELECTION )
	{
		const int dx = _me->x() - m_initialMouseX;
		QVector<selectableObject *> so =
				m_track->getTrackContainer()->selectedObjects();
		QVector<trackContentObject *> tcos;
		midiTime smallest_pos;
		// find out smallest position of all selected objects for not
		// moving an object before zero
		for( QVector<selectableObject *>::iterator it = so.begin();
							it != so.end(); ++it )
		{
			trackContentObject * tco =
				dynamic_cast<trackContentObject *>( *it );
			if( tco == NULL )
			{
				continue;
			}
			tcos.push_back( tco );
			smallest_pos = tMin<Sint32>( smallest_pos,
					(Sint32)tco->startPosition() +
					static_cast<int>( dx * 64 / ppt ) );
		}
		for( QVector<trackContentObject *>::iterator it = tcos.begin();
							it != tcos.end(); ++it )
		{
			( *it )->movePosition( ( *it )->startPosition() +
					static_cast<int>( dx * 64 / ppt ) -
								smallest_pos );
		}
	}
	else if( m_action == RESIZE )
	{
		midiTime t = tMax( 64,
				static_cast<int>( _me->x() * 64 / ppt ) );
		if( engine::getMainWindow()->isCtrlPressed() ==
					FALSE && _me->button() == Qt::NoButton )
		{
			t = t.toNearestTact();
		}
		changeLength( t );
		s_textFloat->setText( tr( "%1:%2 (%3:%4 to %5:%6)" ).
					arg( length().getTact() ).
					arg( length().getTact64th() ).
					arg( m_startPosition.getTact() + 1 ).
					arg( m_startPosition.getTact64th() ).
					arg( endPosition().getTact() + 1 ).
					arg( endPosition().getTact64th() ) );
		s_textFloat->move( mapTo( topLevelWidget(), QPoint( 0, 0 ) ) +
						QPoint( width() + 2, 8 ) );
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




void trackContentObject::mouseReleaseEvent( QMouseEvent * _me )
{
	if( m_action == MOVE || m_action == RESIZE )
	{
		setJournalling( TRUE );
		addJournalEntry( journalEntry( m_action, m_oldTime -
				( ( m_action == MOVE ) ?
					m_startPosition : m_length ) ) );
	}
	m_action = NONE;
	delete m_hint;
	m_hint = NULL;
	s_textFloat->hide();
	leaveEvent( NULL );
	selectableObject::mouseReleaseEvent( _me );
}




void trackContentObject::contextMenuEvent( QContextMenuEvent * _cme )
{
	QMenu contextMenu( this );
	if( fixedTCOs() == FALSE )
	{
		contextMenu.addAction( embed::getIconPixmap( "cancel" ),
					tr( "Delete (middle mousebutton)" ),
							this, SLOT( close() ) );
		contextMenu.addSeparator();
		contextMenu.addAction( embed::getIconPixmap( "edit_cut" ),
					tr( "Cut" ), this, SLOT( cut() ) );
	}
	contextMenu.addAction( embed::getIconPixmap( "edit_copy" ),
					tr( "Copy" ), this, SLOT( copy() ) );
	contextMenu.addAction( embed::getIconPixmap( "edit_paste" ),
					tr( "Paste" ), this, SLOT( paste() ) );
	contextMenu.addSeparator();
	contextMenu.addAction( embed::getIconPixmap( "muted" ),
				tr( "Mute/unmute (<Ctrl> + middle click)" ),
						this, SLOT( toggleMute() ) );
	constructContextMenu( &contextMenu );

	contextMenu.exec( QCursor::pos() );
}

#undef addSeparator



float trackContentObject::pixelsPerTact( void )
{
	if( fixedTCOs() )
	{
		return( ( getTrack()->getTrackContentWidget()->width() -
				2 * TCO_BORDER_WIDTH -
						DEFAULT_SCROLLBAR_SIZE ) /
				tMax<float>( length().getTact(), 1.0f ) );
	}
	return( getTrack()->getTrackContainer()->pixelsPerTact() );
}




void trackContentObject::undoStep( journalEntry & _je )
{
	saveJournallingState( FALSE );
	switch( _je.actionID() )
	{
		case MOVE:
			movePosition( startPosition() + _je.data().toInt() );
			break;
		case RESIZE:
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




void trackContentObject::close( void )
{
	m_track->getTrackContentWidget()->removeTCO( this, FALSE );
	// we have to give our track-container the focus because otherwise the
	// op-buttons of our track-widgets could become focus and when the user
	// presses space for playing song, just one of these buttons is pressed
	// which results in unwanted effects
	m_track->getTrackContainer()->setFocus();
	QWidget::close();
}




void trackContentObject::cut( void )
{
	copy();
	close();
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
	m_muted = !m_muted;
	update();
}




void trackContentObject::setAutoResizeEnabled( bool _e )
{
	m_autoResize = _e;
}




// ===========================================================================
// trackContentWidget
// ===========================================================================
trackContentWidget::trackContentWidget( trackWidget * _parent ) :
	QWidget( _parent ),
	m_trackWidget( _parent )
{
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setColor( backgroundRole(), QColor( 96, 96, 96 ) );
	setPalette( pal );
	setAcceptDrops( TRUE );
}




trackContentWidget::~trackContentWidget()
{
}




trackContentObject * trackContentWidget::getTCO( int _tco_num )
{
	if( _tco_num < m_trackContentObjects.size() )
	{
		return( m_trackContentObjects[_tco_num] );
	}
	printf( "called trackContentWidget::getTCO( %d, TRUE ), "
			"but TCO %d doesn't exist\n", _tco_num, _tco_num );
	return( getTrack()->addTCO( getTrack()->createTCO( _tco_num * 64 ) ) );
//	return( NULL );
}




int trackContentWidget::numOfTCOs( void )
{
	return( m_trackContentObjects.size() );
}




trackContentObject * trackContentWidget::addTCO( trackContentObject * _tco )
{
	QMap<QString, QVariant> map;
	map["id"] = _tco->id();
	addJournalEntry( journalEntry( ADD_TCO, map ) );

	m_trackContentObjects.push_back( _tco );
	_tco->move( 0, 1 );

	_tco->saveJournallingState( FALSE );
	m_trackWidget->changePosition();
	_tco->restoreJournallingState();

	//engine::getSongEditor()->setModified();
	return( _tco );		// just for convenience
}




void trackContentWidget::removeTCO( int _tco_num, bool _also_delete )
{
	removeTCO( getTCO( _tco_num ), _also_delete );
}




void trackContentWidget::removeTCO( trackContentObject * _tco,
							bool _also_delete )
{
	tcoVector::iterator it = qFind( m_trackContentObjects.begin(),
					m_trackContentObjects.end(),
					_tco );
	if( it != m_trackContentObjects.end() )
	{
		QMap<QString, QVariant> map;
		multimediaProject mmp( multimediaProject::JOURNAL_DATA );
		_tco->saveState( mmp, mmp.content() );
		map["id"] = _tco->id();
		map["state"] = mmp.toString();
		addJournalEntry( journalEntry( REMOVE_TCO, map ) );

		if( _also_delete )
		{
			delete _tco;
		}

		m_trackContentObjects.erase( it );
		engine::getSongEditor()->setModified();
	}
}




void trackContentWidget::removeAllTCOs( void )
{
	while( !m_trackContentObjects.empty() )
	{
		delete m_trackContentObjects.front();
		m_trackContentObjects.erase( m_trackContentObjects.begin() );
	}
}




void trackContentWidget::swapPositionOfTCOs( int _tco_num1, int _tco_num2 )
{
	// TODO: range-checking
	qSwap( m_trackContentObjects[_tco_num1],
					m_trackContentObjects[_tco_num2] );

	const midiTime pos = m_trackContentObjects[_tco_num1]->startPosition();

	m_trackContentObjects[_tco_num1]->movePosition(
			m_trackContentObjects[_tco_num2]->startPosition() );
	m_trackContentObjects[_tco_num2]->movePosition( pos );
}




tact trackContentWidget::length( void ) const
{
	// find last end-position
	midiTime last = 0;
	for( tcoVector::const_iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		last = tMax( ( *it )->endPosition(), last );
	}
	return( last.getTact() + ( ( last.getTact64th() != 0 )? 1 : 0 ) );
}




void trackContentWidget::insertTact( const midiTime & _pos )
{
	// we'll increase the position of every TCO, posated behind _pos, by
	// one tact
	for( tcoVector::iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		if( ( *it )->startPosition() >= _pos )
		{
			( *it )->movePosition( (*it)->startPosition() + 64 );
		}
	}
}




void trackContentWidget::removeTact( const midiTime & _pos )
{
	// we'll decrease the position of every TCO, posated behind _pos, by
	// one tact
	for( tcoVector::iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		if( ( *it )->startPosition() >= _pos )
		{
			( *it )->movePosition( tMax( ( *it )->startPosition() -
								    64, 0 ) );
		}
	}
}




void trackContentWidget::updateTCOs( void )
{
	for( tcoVector::iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		( *it )->setFixedHeight( height() - 2 );
		( *it )->update();
	}
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
			getTrack()->getTrackContainer()->fixedTCOs() == FALSE )
	{
		const midiTime pos = getPosition( _de->pos().x()
							).toNearestTact();
		trackContentObject * tco = addTCO( getTrack()->createTCO(
								pos ) );
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
	if( getTrack()->getTrackContainer()->allowRubberband() == TRUE )
	{
		QWidget::mousePressEvent( _me );
	}
	else if( engine::getMainWindow()->isShiftPressed() == TRUE )
	{
		QWidget::mousePressEvent( _me );
	}
	else if( _me->button() == Qt::LeftButton &&
			getTrack()->getTrackContainer()->fixedTCOs() == FALSE )
	{
		const midiTime pos = getPosition( _me->x() ).getTact() * 64;
		trackContentObject * tco = addTCO( getTrack()->createTCO(
									pos ) );
		tco->saveJournallingState( FALSE );
		tco->movePosition( pos );
		tco->restoreJournallingState();
	}
}




void trackContentWidget::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.fillRect( rect(), QColor( 96, 96, 96 ) );
	const trackContainer * tc = getTrack()->getTrackContainer();
	const int offset = (int)( ( tc->currentPosition() % 4 ) *
							tc->pixelsPerTact() );
	// draw vertical lines
	p.setPen( QColor( 128, 128, 128 ) );
	for( int x = -offset; x < width(); x += (int) tc->pixelsPerTact() )
	{
		p.drawLine( x, 0, x, height() );
	}
}




void trackContentWidget::resizeEvent( QResizeEvent * _re )
{
	updateTCOs();
}




void trackContentWidget::undoStep( journalEntry & _je )
{
	saveJournallingState( FALSE );
	switch( _je.actionID() )
	{
		case ADD_TCO:
		{
			QMap<QString, QVariant> map = _je.data().toMap();
			trackContentObject * tco =
				dynamic_cast<trackContentObject *>(
			engine::getProjectJournal()->getJournallingObject(
							map["id"].toInt() ) );
			assert( tco != NULL );
			multimediaProject mmp(
					multimediaProject::JOURNAL_DATA );
			tco->saveState( mmp, mmp.content() );
			map["state"] = mmp.toString();
			_je.data() = map;
			tco->close();
			break;
		}

		case REMOVE_TCO:
		{
			trackContentObject * tco = addTCO(
						getTrack()->createTCO(
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
		case ADD_TCO:
		case REMOVE_TCO:
			_je.actionID() = ( _je.actionID() == ADD_TCO ) ?
							REMOVE_TCO : ADD_TCO;
			undoStep( _je );
			_je.actionID() = ( _je.actionID() == ADD_TCO ) ?
							REMOVE_TCO : ADD_TCO;
			break;
	}
}




track * trackContentWidget::getTrack( void )
{
	return( m_trackWidget->getTrack() );
}




midiTime trackContentWidget::getPosition( int _mouse_x )
{
	const trackContainer * tc = getTrack()->getTrackContainer();
	return( midiTime( tc->currentPosition() + _mouse_x * 64 /
				static_cast<int>( tc->pixelsPerTact() ) ) );
}




// ===========================================================================
// trackOperationsWidget
// ===========================================================================


QPixmap * trackOperationsWidget::s_grip = NULL;


trackOperationsWidget::trackOperationsWidget( trackWidget * _parent ) :
	QWidget( _parent ),
	m_trackWidget( _parent )
{
	if( s_grip == NULL )
	{
		s_grip = new QPixmap( embed::getIconPixmap(
							"track_op_grip" ) );
	}

	toolTip::add( this, tr( "Press <Ctrl> while clicking on move-grip "
				"to begin a new drag'n'drop-action." ) );

	QMenu * to_menu = new QMenu( this );
	to_menu->setFont( pointSize<9>( to_menu->font() ) );
	connect( to_menu, SIGNAL( aboutToShow() ), this, SLOT( updateMenu() ) );


	m_trackOps = new QPushButton( embed::getIconPixmap( "track_op_menu" ),
								"", this );
	m_trackOps->setGeometry( 12, 1, 28, 28 );
	m_trackOps->setMenu( to_menu );
	toolTip::add( m_trackOps, tr( "Actions for this track" ) );


	m_muteBtn = new pixmapButton( this, tr( "Mute" ),
						m_trackWidget->getTrack() );
	m_muteBtn->setActiveGraphic( embed::getIconPixmap( "mute_on" ) );
	m_muteBtn->setInactiveGraphic( embed::getIconPixmap( "mute_off" ) );
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
	return( m_muteBtn->isChecked() );
}




void trackOperationsWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
		engine::getMainWindow()->isCtrlPressed() == TRUE &&
			m_trackWidget->getTrack()->type() != track::BB_TRACK )
	{
		multimediaProject mmp( multimediaProject::DRAG_N_DROP_DATA );
		m_trackWidget->getTrack()->saveState( mmp, mmp.content() );
		new stringPairDrag( QString( "track_%1" ).arg(
					m_trackWidget->getTrack()->type() ),
			mmp.toString(), QPixmap::grabWidget(
				&m_trackWidget->getTrackSettingsWidget() ),
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
	if( m_trackWidget->isMovingTrack() == FALSE )
	{
		p.drawPixmap( 2, 2, *s_grip );
		if( inBBEditor() )
		{
			const char * trackOps_icon;
			const char * mute_active_icon;
			const char * mute_inactive_icon;
			bbTrack * bb_track = currentBBTrack();
			if( !bb_track || bb_track->automationDisabled(
						m_trackWidget->getTrack() ) )
			{
				trackOps_icon = "track_op_menu_disabled";
				mute_active_icon = "mute_on_disabled";
				mute_inactive_icon = "mute_off_disabled";
			}
			else
			{
				trackOps_icon = "track_op_menu";
				mute_active_icon = "mute_on";
				mute_inactive_icon = "mute_off";
			}
			m_trackOps->setIcon( embed::getIconPixmap(
							trackOps_icon ) );
			m_muteBtn->setActiveGraphic( embed::getIconPixmap(
							mute_active_icon ) );
			m_muteBtn->setInactiveGraphic( embed::getIconPixmap(
							mute_inactive_icon ) );
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
	m_trackWidget->getTrack()->clone();
	engine::getMixer()->unlock();
}




void trackOperationsWidget::removeTrack( void )
{
	engine::getMixer()->lock();
	m_trackWidget->getTrack()->getTrackContainer()->removeTrack(
						m_trackWidget->getTrack() );
	engine::getMixer()->unlock();
}




void trackOperationsWidget::setMuted( bool _muted )
{
	m_muteBtn->setChecked( _muted );
	m_trackWidget->getTrackContentWidget().updateTCOs();
}




void trackOperationsWidget::muteBtnRightClicked( void )
{
	const bool m = muted();	// next function might modify our mute-state,
				// so save it now
	m_trackWidget->getTrack()->getTrackContainer()->
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
						m_trackWidget->getTrack() ) )
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
	currentBBTrack()->enableAutomation( m_trackWidget->getTrack() );
}




void trackOperationsWidget::disableAutomation( void )
{
	currentBBTrack()->disableAutomation( m_trackWidget->getTrack() );
}




bbTrack * trackOperationsWidget::currentBBTrack( void )
{
	return( bbTrack::findBBTrack( engine::getBBEditor()->currentBB() ) );
}




bool trackOperationsWidget::inBBEditor( void )
{
	return( m_trackWidget->getTrack()->getTrackContainer()
						== engine::getBBEditor() );
}






// ===========================================================================
// trackWidget
// ===========================================================================

trackWidget::trackWidget( track * _track, QWidget * _parent ) :
	QWidget( _parent ),
	m_track( _track ),
	m_trackOperationsWidget( this ),
	m_trackSettingsWidget( this ),
	m_trackContentWidget( this ),
	m_action( NONE )
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

	setAcceptDrops( TRUE );
}




trackWidget::~trackWidget()
{
}




void trackWidget::repaint( void )
{
	m_trackContentWidget.repaint();
	QWidget::repaint();
}




void trackWidget::update( void )
{
	m_trackContentWidget.update();
	QWidget::update();
}




// resposible for moving track-content-widgets to appropriate position after
// change of visible viewport
void trackWidget::changePosition( const midiTime & _new_pos )
{
	midiTime pos = _new_pos;
	if( pos < 0 )
	{
		pos = m_track->getTrackContainer()->currentPosition();
	}

	const Sint32 begin = pos;
	const Sint32 end = endPosition( pos );
	const float ppt = m_track->getTrackContainer()->pixelsPerTact();
	const int tcos = m_trackContentWidget.numOfTCOs();

	for( int i = 0; i < tcos; ++i )
	{
		trackContentObject * tco = m_trackContentWidget.getTCO( i );
		tco->changeLength( tco->length() );
		Sint32 ts = tco->startPosition();
		Sint32 te = tco->endPosition();
		if( ( ts >= begin && ts <= end ) ||
			( te >= begin && te <= end ) ||
			( ts <= begin && te >= end ) )
		{
			tco->move( static_cast<int>( ( ts - begin ) * ppt /
							64 ), tco->y() );
			tco->show();
		}
		else
		{
			tco->hide();
		}
	}
}




void trackWidget::undoStep( journalEntry & _je )
{
	saveJournallingState( FALSE );
	switch( _je.actionID() )
	{
		case MOVE_TRACK:
		{
			trackContainer * tc = m_track->getTrackContainer();
			if( _je.data().toInt() > 0 )
			{
				tc->moveTrackUp( m_track );
			}
			else
			{
				tc->moveTrackDown( m_track );
			}
			break;
		}
		case RESIZE_TRACK:
			setFixedHeight( tMax<int>( height() +
						_je.data().toInt(),
						MINIMAL_TRACK_HEIGHT ) );
			m_track->getTrackContainer()->realignTracks();
			break;
	}
	restoreJournallingState();
}




void trackWidget::redoStep( journalEntry & _je )
{
	journalEntry je( _je.actionID(), -_je.data().toInt() );
	undoStep( je );
}




void trackWidget::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "track_" +
					QString::number( m_track->type() ) );
}




void trackWidget::dropEvent( QDropEvent * _de )
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




void trackWidget::mousePressEvent( QMouseEvent * _me )
{
	if( m_track->getTrackContainer()->allowRubberband() == TRUE )
	{
		QWidget::mousePressEvent( _me );
	}
	else if( _me->button() == Qt::LeftButton )
	{
		if( engine::getMainWindow()->isShiftPressed() == TRUE )
		{
			m_action = RESIZE_TRACK;
			QCursor::setPos( mapToGlobal( QPoint( _me->x(),
								height() ) ) );
			QCursor c( Qt::SizeVerCursor);
			QApplication::setOverrideCursor( c );
		}
		else
		{
			m_action = MOVE_TRACK;

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




void trackWidget::mouseMoveEvent( QMouseEvent * _me )
{
	if( m_track->getTrackContainer()->allowRubberband() == TRUE )
	{
		QWidget::mouseMoveEvent( _me );
	}
	else if( m_action == MOVE_TRACK )
	{
		trackContainer * tc = m_track->getTrackContainer();
		// look which track-widget the mouse-cursor is over
		const trackWidget * track_at_y = tc->trackWidgetAt(
			mapTo( tc->containerWidget(), _me->pos() ).y() );
		// a track-widget not equal to ourself?
		if( track_at_y != NULL && track_at_y != this )
		{
			// then move us up/down there!
			if( _me->y() < 0 )
			{
				tc->moveTrackUp( m_track );
			}
			else
			{
				tc->moveTrackDown( m_track );
			}
			addJournalEntry( journalEntry( MOVE_TRACK, _me->y() ) );
		}
	}
	else if( m_action == RESIZE_TRACK )
	{
		setFixedHeight( tMax<int>( _me->y(), MINIMAL_TRACK_HEIGHT ) );
		m_track->getTrackContainer()->realignTracks();
	}
}




void trackWidget::mouseReleaseEvent( QMouseEvent * _me )
{
	m_action = NONE;
	while( QApplication::overrideCursor() != NULL )
	{
		QApplication::restoreOverrideCursor();
	}
	m_trackOperationsWidget.update();

	QWidget::mouseReleaseEvent( _me );
}




void trackWidget::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.setPen( QColor( 0, 0, 0 ) );
	p.drawLine( 0, height() - 1, width() - 1, height() - 1 );
}




void trackWidget::resizeEvent( QResizeEvent * _re )
{
	m_trackOperationsWidget.setFixedSize( TRACK_OP_WIDTH, height() - 1 );
	m_trackOperationsWidget.move( 0, 0 );
	m_trackSettingsWidget.setFixedSize( DEFAULT_SETTINGS_WIDGET_WIDTH,
								height() - 1 );
	m_trackSettingsWidget.move( TRACK_OP_WIDTH, 0 );
	m_trackContentWidget.resize( m_track->getTrackContainer()->width() -
								TRACK_OP_WIDTH -
						DEFAULT_SETTINGS_WIDGET_WIDTH,
								height() - 1 );
	m_trackContentWidget.move( m_trackOperationsWidget.width() +
					m_trackSettingsWidget.width(), 0 );
}




midiTime trackWidget::endPosition( const midiTime & _pos_start )
{
	const float ppt = m_track->getTrackContainer()->pixelsPerTact();
	const int cww = m_trackContentWidget.width();
	return( _pos_start + static_cast<int>( cww * 64 / ppt ) );
}






// ===========================================================================
// track
// ===========================================================================

track::track( trackContainer * _tc, bool _create_widget ) :
	m_trackContainer( _tc ),
	m_trackWidget( NULL )
{
	if( _create_widget )
	{
		m_trackWidget = new trackWidget( this,
					m_trackContainer->containerWidget() );

		m_trackContainer->addTrack( this );
	}
}




track::~track()
{
	if( m_trackContainer == engine::getBBEditor()
						&& engine::getSongEditor() )
	{
		trackVector tracks = engine::getSongEditor()->tracks();
		for( trackVector::iterator it = tracks.begin();
						it != tracks.end(); ++it )
		{
			if( ( *it )->type() == BB_TRACK )
			{
				bbTrack * bb_track = (bbTrack *)*it;
				if( bb_track->automationDisabled( this ) )
				{
					// Remove reference from bbTrack
					bb_track->enableAutomation( this );
				}
			}
		}
	}

	if( m_trackWidget != NULL )
	{
		m_trackContainer->removeTrack( this );

		delete m_trackWidget;
		m_trackWidget = NULL;
	}

	for( QList<automationPattern *>::iterator it =
						m_automation_patterns.begin();
					it != m_automation_patterns.end();
					++it )
	{
		( *it )->forgetTrack();
	}
}




track * track::create( trackTypes _tt, trackContainer * _tc )
{
	track * t = NULL;

	switch( _tt )
	{
		case INSTRUMENT_TRACK: t = new instrumentTrack( _tc ); break;
		case BB_TRACK: t = new bbTrack( _tc ); break;
		case SAMPLE_TRACK: t = new sampleTrack( _tc ); break;
//		case EVENT_TRACK:
//		case VIDEO_TRACK:
		case AUTOMATION_TRACK: t = new automationTrack( _tc ); break;
		default: break;
	}

#ifdef DEBUG_LMMS
	assert( t != NULL );
#endif

	return( t );
}




void track::create( const QDomElement & _this, trackContainer * _tc )
{
	create( static_cast<trackTypes>( _this.attribute( "type" ).toInt() ),
						_tc )->restoreState( _this );
}




void track::clone( void )
{
	QDomDocument doc;
	QDomElement parent = doc.createElement( "clone" );
	saveState( doc, parent );
	create( parent.firstChild().toElement(), m_trackContainer );
}




tact track::length( void ) const
{
	return( getTrackContentWidget()->length() );
}




void track::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	int num_of_tcos = getTrackContentWidget()->numOfTCOs();

	_this.setTagName( "track" );
	_this.setAttribute( "type", type() );
	_this.setAttribute( "muted", muted() );
	_this.setAttribute( "height", m_trackWidget->height() );

	QDomElement ts_de = _doc.createElement( nodeName() );
	// let actual track (instrumentTrack, bbTrack, sampleTrack etc.) save
	// its settings
	_this.appendChild( ts_de );
	saveTrackSpecificSettings( _doc, ts_de );

	// now save settings of all TCO's
	for( int i = 0; i < num_of_tcos; ++i )
	{
		trackContentObject * tco = getTCO( i );
		tco->saveState( _doc, _this );
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


	getTrackContentWidget()->removeAllTCOs();

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
				getTrackContentWidget()->saveJournallingState(
									FALSE );
				addTCO( tco );
				getTrackContentWidget()->
						restoreJournallingState();
			}
		}
		node = node.nextSibling();
        }

	if( _this.attribute( "height" ).toInt() >= MINIMAL_TRACK_HEIGHT )
	{
		m_trackWidget->setFixedHeight(
					_this.attribute( "height" ).toInt() );
	}
}




trackContentObject * track::addTCO( trackContentObject * _tco )
{
	return( getTrackContentWidget()->addTCO( _tco ) );
}




void track::removeTCO( int _tco_num )
{
	getTrackContentWidget()->removeTCO( _tco_num );
}




int track::numOfTCOs( void )
{
	return( getTrackContentWidget()->numOfTCOs() );
}




trackContentObject * track::getTCO( int _tco_num )
{
	return( getTrackContentWidget()->getTCO( _tco_num ) );
	
}




int track::getTCONum( trackContentObject * _tco )
{
	for( int i = 0; i < getTrackContentWidget()->numOfTCOs(); ++i )
	{
		if( getTCO( i ) == _tco )
		{
			return( i );
		}
	}
	qWarning( "track::getTCONum(...) -> _tco not found!\n" );
	return( 0 );
}




void track::getTCOsInRange( QList<trackContentObject *> & _tco_v,
							const midiTime & _start,
							const midiTime & _end )
{
	for( int i = 0; i < getTrackContentWidget()->numOfTCOs(); ++i )
	{
		trackContentObject * tco = getTCO( i );
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
	getTrackContentWidget()->swapPositionOfTCOs( _tco_num1, _tco_num2 );
}




void track::addAutomationPattern( automationPattern * _pattern )
{
	m_automation_patterns.append( _pattern );
}




void track::removeAutomationPattern( automationPattern * _pattern )
{
	m_automation_patterns.removeAll( _pattern );
}




void track::sendMidiTime( const midiTime & _time )
{
	for( QList<automationPattern *>::iterator it =
						m_automation_patterns.begin();
					it != m_automation_patterns.end();
					++it )
	{
		( *it )->processMidiTime( _time );
	}
}




#include "track.moc"


#endif
