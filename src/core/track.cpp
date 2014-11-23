/*
 * track.cpp - implementation of classes concerning tracks -> necessary for
 *             all track-like objects (beat/bassline, sample-track...)
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

/** \file track.cpp
 *  \brief All classes concerning tracks and track-like objects
 */

/*
 * \mainpage Track classes
 *
 * \section introduction Introduction
 *
 * \todo fill this out
 */

#include "track.h"

#include <assert.h>
#include <cstdio>

#include <QtGui/QLayout>
#include <QtGui/QMenu>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QStyleOption>


#include "AutomationPattern.h"
#include "AutomationTrack.h"
#include "bb_editor.h"
#include "bb_track.h"
#include "bb_track_container.h"
#include "config_mgr.h"
#include "Clipboard.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "InstrumentTrack.h"
#include "MainWindow.h"
#include "DataFile.h"
#include "pixmap_button.h"
#include "ProjectJournal.h"
#include "SampleTrack.h"
#include "song.h"
#include "string_pair_drag.h"
#include "templates.h"
#include "text_float.h"
#include "tooltip.h"
#include "TrackContainer.h"


/*! The width of the resize grip in pixels
 */
const int RESIZE_GRIP_WIDTH = 4;

/*! The size of the track buttons in pixels
 */
const int TRACK_OP_BTN_WIDTH = 20;
const int TRACK_OP_BTN_HEIGHT = 14;


/*! A pointer for that text bubble used when moving segments, etc.
 *
 * In a number of situations, LMMS displays a floating text bubble
 * beside the cursor as you move or resize elements of a track about.
 * This pointer keeps track of it, as you only ever need one at a time.
 */
textFloat * trackContentObjectView::s_textFloat = NULL;


// ===========================================================================
// trackContentObject
// ===========================================================================
/*! \brief Create a new trackContentObject
 *
 *  Creates a new track content object for the given track.
 *
 * \param _track The track that will contain the new object
 */
trackContentObject::trackContentObject( track * _track ) :
	Model( _track ),
	m_track( _track ),
	m_name( QString::null ),
	m_startPosition(),
	m_length(),
	m_mutedModel( false, this, tr( "Muted" ) )
{
	if( getTrack() )
	{
		getTrack()->addTCO( this );
	}
	setJournalling( false );
	movePosition( 0 );
	changeLength( 0 );
	setJournalling( true );
}




/*! \brief Destroy a trackContentObject
 *
 *  Destroys the given track content object.
 *
 */
trackContentObject::~trackContentObject()
{
	emit destroyedTCO();

	if( getTrack() )
	{
		getTrack()->removeTCO( this );
	}
}




/*! \brief Move this trackContentObject's position in time
 *
 *  If the track content object has moved, update its position.  We
 *  also add a journal entry for undo and update the display.
 *
 * \param _pos The new position of the track content object.
 */
void trackContentObject::movePosition( const MidiTime & _pos )
{
	if( m_startPosition != _pos )
	{
		m_startPosition = _pos;
		engine::getSong()->updateLength();
	}
	emit positionChanged();
}




/*! \brief Change the length of this trackContentObject
 *
 *  If the track content object's length has chaanged, update it.  We
 *  also add a journal entry for undo and update the display.
 *
 * \param _length The new length of the track content object.
 */
void trackContentObject::changeLength( const MidiTime & _length )
{
	if( m_length != _length )
	{
		m_length = _length;
		engine::getSong()->updateLength();
	}
	emit lengthChanged();
}




/*! \brief Copy this trackContentObject to the clipboard.
 *
 *  Copies this track content object to the clipboard.
 */
void trackContentObject::copy()
{
	Clipboard::copy( this );
}




/*! \brief Pastes this trackContentObject into a track.
 *
 *  Pastes this track content object into a track.
 *
 * \param _je The journal entry to undo
 */
void trackContentObject::paste()
{
	if( Clipboard::getContent( nodeName() ) != NULL )
	{
		const MidiTime pos = startPosition();
		restoreState( *( Clipboard::getContent( nodeName() ) ) );
		movePosition( pos );
	}
}




/*! \brief Mutes this trackContentObject
 *
 *  Restore the previous state of this track content object.  This will
 *  restore the position or the length of the track content object
 *  depending on what was changed.
 *
 * \param _je The journal entry to undo
 */
void trackContentObject::toggleMute()
{
	m_mutedModel.setValue( !m_mutedModel.value() );
	emit dataChanged();
}







// ===========================================================================
// trackContentObjectView
// ===========================================================================
/*! \brief Create a new trackContentObjectView
 *
 *  Creates a new track content object view for the given
 *  track content object in the given track view.
 *
 * \param _tco The track content object to be displayed
 * \param _tv  The track view that will contain the new object
 */
trackContentObjectView::trackContentObjectView( trackContentObject * _tco,
							trackView * _tv ) :
	selectableObject( _tv->getTrackContentWidget() ),
	ModelView( NULL, this ),
	m_tco( _tco ),
	m_trackView( _tv ),
	m_action( NoAction ),
	m_autoResize( false ),
	m_initialMouseX( 0 ),
	m_hint( NULL ),
	m_fgColor( 0, 0, 0 ),
	m_textColor( 0, 0, 0 )
{
	if( s_textFloat == NULL )
	{
		s_textFloat = new textFloat;
		s_textFloat->setPixmap( embed::getIconPixmap( "clock" ) );
	}

	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setAttribute( Qt::WA_DeleteOnClose, true );
	setFocusPolicy( Qt::StrongFocus );
	setCursor( QCursor( embed::getIconPixmap( "hand" ), 3, 3 ) );
	move( 0, 1 );
	show();

	setFixedHeight( _tv->getTrackContentWidget()->height() - 2 );
	setAcceptDrops( true );
	setMouseTracking( true );

	connect( m_tco, SIGNAL( lengthChanged() ),
			this, SLOT( updateLength() ) );
	connect( m_tco, SIGNAL( positionChanged() ),
			this, SLOT( updatePosition() ) );
	connect( m_tco, SIGNAL( destroyedTCO() ), this, SLOT( close() ) );
	setModel( m_tco );

	m_trackView->getTrackContentWidget()->addTCOView( this );
}




/*! \brief Destroy a trackContentObjectView
 *
 *  Destroys the given track content object view.
 *
 */
trackContentObjectView::~trackContentObjectView()
{
	delete m_hint;
	// we have to give our track-container the focus because otherwise the
	// op-buttons of our track-widgets could become focus and when the user
	// presses space for playing song, just one of these buttons is pressed
	// which results in unwanted effects
	m_trackView->trackContainerView()->setFocus();
}




/*! \brief Does this trackContentObjectView have a fixed TCO?
 *
 *  Returns whether the containing trackView has fixed
 *  TCOs.
 *
 * \todo What the hell is a TCO here - track content object?  And in
 *  what circumstance are they fixed?
 */
bool trackContentObjectView::fixedTCOs()
{
	return m_trackView->trackContainerView()->fixedTCOs();
}



// qproperty access functions, to be inherited & used by TCOviews
//! \brief CSS theming qproperty access method
QColor trackContentObjectView::fgColor() const
{ return m_fgColor; }

//! \brief CSS theming qproperty access method
QColor trackContentObjectView::textColor() const
{ return m_textColor; }

//! \brief CSS theming qproperty access method
void trackContentObjectView::setFgColor( const QColor & _c )
{ m_fgColor = QColor( _c ); }

//! \brief CSS theming qproperty access method
void trackContentObjectView::setTextColor( const QColor & _c )
{ m_textColor = QColor( _c ); }


/*! \brief Close a trackContentObjectView
 *
 *  Closes a track content object view by asking the track
 *  view to remove us and then asking the QWidget to close us.
 *
 * \return Boolean state of whether the QWidget was able to close.
 */
bool trackContentObjectView::close()
{
	m_trackView->getTrackContentWidget()->removeTCOView( this );
	return QWidget::close();
}




/*! \brief Removes a trackContentObjectView from its track view.
 *
 *  Like the close() method, this asks the track view to remove this
 *  track content object view.  However, the track content object is
 *  scheduled for later deletion rather than closed immediately.
 *
 */
void trackContentObjectView::remove()
{
	m_trackView->getTrack()->addJournalCheckPoint();

	// delete ourself
	close();
	m_tco->deleteLater();
}




/*! \brief Cut this trackContentObjectView from its track to the clipboard.
 *
 *  Perform the 'cut' action of the clipboard - copies the track content
 *  object to the clipboard and then removes it from the track.
 */
void trackContentObjectView::cut()
{
	m_tco->copy();
	remove();
}




/*! \brief Updates a trackContentObjectView's length
 *
 *  If this track content object view has a fixed TCO, then we must
 *  keep the width of our parent.  Otherwise, calculate our width from
 *  the track content object's length in pixels adding in the border.
 *
 */
void trackContentObjectView::updateLength()
{
	if( fixedTCOs() )
	{
		setFixedWidth( parentWidget()->width() );
	}
	else
	{
		setFixedWidth(
		static_cast<int>( m_tco->length() * pixelsPerTact() /
					MidiTime::ticksPerTact() ) + 1 /*+
						TCO_BORDER_WIDTH * 2-1*/ );
	}
	m_trackView->trackContainerView()->update();
}




/*! \brief Updates a trackContentObjectView's position.
 *
 *  Ask our track view to change our position.  Then make sure that the
 *  track view is updated in case this position has changed the track
 *  view's length.
 *
 */
void trackContentObjectView::updatePosition()
{
	m_trackView->getTrackContentWidget()->changePosition();
	// moving a TCO can result in change of song-length etc.,
	// therefore we update the track-container
	m_trackView->trackContainerView()->update();
}



/*! \brief Change the trackContentObjectView's display when something
 *  being dragged enters it.
 *
 *  We need to notify Qt to change our display if something being
 *  dragged has entered our 'airspace'.
 *
 * \param _dee The QDragEnterEvent to watch.
 */
void trackContentObjectView::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "tco_" +
				QString::number( m_tco->getTrack()->type() ) );
}




/*! \brief Handle something being dropped on this trackContentObjectView.
 *
 *  When something has been dropped on this trackContentObjectView, and
 *  it's a track content object, then use an instance of our dataFile reader
 *  to take the xml of the track content object and turn it into something
 *  we can write over our current state.
 *
 * \param _de The QDropEvent to handle.
 */
void trackContentObjectView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == ( "tco_" + QString::number( m_tco->getTrack()->type() ) ) )
	{
		// value contains our XML-data so simply create a
		// DataFile which does the rest for us...
		DataFile dataFile( value.toUtf8() );
		// at least save position before getting to moved to somewhere
		// the user doesn't expect...
		MidiTime pos = m_tco->startPosition();
		m_tco->restoreState( dataFile.content().firstChild().toElement() );
 		m_tco->movePosition( pos );
		AutomationPattern::resolveAllIDs();
		_de->accept();
	}
}




/*! \brief Handle a dragged selection leaving our 'airspace'.
 *
 * \param _e The QEvent to watch.
 */
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




/*! \brief Handle a mouse press on this trackContentObjectView.
 *
 *  Handles the various ways in which a trackContentObjectView can be
 *  used with a click of a mouse button.
 *
 *  * If our container supports rubber band selection then handle
 *    selection events.
 *  * or if shift-left button, add this object to the selection
 *  * or if ctrl-left button, start a drag-copy event
 *  * or if just plain left button, resize if we're resizeable
 *  * or if ctrl-middle button, mute the track content object
 *  * or if middle button, maybe delete the track content object.
 *
 * \param _me The QMouseEvent to handle.
 */
void trackContentObjectView::mousePressEvent( QMouseEvent * _me )
{
	if( m_trackView->trackContainerView()->allowRubberband() == true &&
					_me->button() == Qt::LeftButton )
	{
		// if rubberband is active, we can be selected
		if( !m_trackView->trackContainerView()->rubberBandActive() )
		{
			if( _me->modifiers() & Qt::ControlModifier )
			{
				setSelected( !isSelected() );
			}
			else if( isSelected() == true )
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
	else if( _me->button() == Qt::LeftButton &&
			_me->modifiers() & Qt::ControlModifier )
	{
		// start drag-action
		DataFile dataFile( DataFile::DragNDropData );
		m_tco->saveState( dataFile, dataFile.content() );
		QPixmap thumbnail = QPixmap::grabWidget( this ).scaled(
						128, 128,
						Qt::KeepAspectRatio,
						Qt::SmoothTransformation );
		new stringPairDrag( QString( "tco_%1" ).arg(
						m_tco->getTrack()->type() ),
					dataFile.toString(), thumbnail, this );
	}
	else if( _me->button() == Qt::LeftButton &&
		/*	engine::mainWindow()->isShiftPressed() == false &&*/
							fixedTCOs() == false )
	{
		m_tco->addJournalCheckPoint();

		// move or resize
		m_tco->setJournalling( false );

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
					tr( "Press <Ctrl> and drag to make "
							"a copy." ),
					embed::getIconPixmap( "hint" ), 0 );
		}
		else if( m_autoResize == false )
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
	else if( _me->button() == Qt::RightButton )
	{
		if( _me->modifiers() & Qt::ControlModifier )
		{
			m_tco->toggleMute();
		}
		else if( _me->modifiers() & Qt::ShiftModifier && fixedTCOs() == false )
		{
			remove();
		}
	}
	else if( _me->button() == Qt::MidButton )
	{
		if( _me->modifiers() & Qt::ControlModifier )
		{
			m_tco->toggleMute();
		}
		else if( fixedTCOs() == false )
		{
			remove();
		}
	}
}




/*! \brief Handle a mouse movement (drag) on this trackContentObjectView.
 *
 *  Handles the various ways in which a trackContentObjectView can be
 *  used with a mouse drag.
 *
 *  * If in move mode, move ourselves in the track,
 *  * or if in move-selection mode, move the entire selection,
 *  * or if in resize mode, resize ourselves,
 *  * otherwise ???
 *
 * \param _me The QMouseEvent to handle.
 * \todo what does the final else case do here?
 */
void trackContentObjectView::mouseMoveEvent( QMouseEvent * _me )
{
	if( _me->modifiers() & Qt::ControlModifier )
	{
		delete m_hint;
		m_hint = NULL;
	}

	const float ppt = m_trackView->trackContainerView()->pixelsPerTact();
	if( m_action == Move )
	{
		const int x = mapToParent( _me->pos() ).x() - m_initialMouseX;
		MidiTime t = qMax( 0, (int)
			m_trackView->trackContainerView()->currentPosition()+
				static_cast<int>( x * MidiTime::ticksPerTact() /
									ppt ) );
		if( ! ( _me->modifiers() & Qt::ControlModifier )
		   && _me->button() == Qt::NoButton )
		{
			t = t.toNearestTact();
		}
		m_tco->movePosition( t );
		m_trackView->getTrackContentWidget()->changePosition();
		s_textFloat->setText( QString( "%1:%2" ).
				arg( m_tco->startPosition().getTact() + 1 ).
				arg( m_tco->startPosition().getTicks() %
						MidiTime::ticksPerTact() ) );
		s_textFloat->moveGlobal( this, QPoint( width() + 2,
		                                        height() + 2 ) );
	}
	else if( m_action == MoveSelection )
	{
		const int dx = _me->x() - m_initialMouseX;
		QVector<selectableObject *> so =
			m_trackView->trackContainerView()->selectedObjects();
		QVector<trackContentObject *> tcos;
		MidiTime smallest_pos, t;
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
			smallest_pos = qMin<int>( smallest_pos,
					(int)tco->startPosition() +
				static_cast<int>( dx *
					MidiTime::ticksPerTact() / ppt ) );
		}
		for( QVector<trackContentObject *>::iterator it = tcos.begin();
							it != tcos.end(); ++it )
		{
			t = ( *it )->startPosition() +
				static_cast<int>( dx *MidiTime::ticksPerTact() /
					 ppt )-smallest_pos;
			if( ! ( _me->modifiers() & Qt::AltModifier )
					   && _me->button() == Qt::NoButton )
			{
				t = t.toNearestTact();
			}
			( *it )->movePosition( t );
		}
	}
	else if( m_action == Resize )
	{
		MidiTime t = qMax( MidiTime::ticksPerTact() / 16, static_cast<int>( _me->x() * MidiTime::ticksPerTact() / ppt ) );
		if( ! ( _me->modifiers() & Qt::ControlModifier ) && _me->button() == Qt::NoButton )
		{
			t = qMax<int>( MidiTime::ticksPerTact(), t.toNearestTact() );
		}
		m_tco->changeLength( t );
		s_textFloat->setText( tr( "%1:%2 (%3:%4 to %5:%6)" ).
				arg( m_tco->length().getTact() ).
				arg( m_tco->length().getTicks() %
						MidiTime::ticksPerTact() ).
				arg( m_tco->startPosition().getTact() + 1 ).
				arg( m_tco->startPosition().getTicks() %
						MidiTime::ticksPerTact() ).
				arg( m_tco->endPosition().getTact() + 1 ).
				arg( m_tco->endPosition().getTicks() %
						MidiTime::ticksPerTact() ) );
		s_textFloat->moveGlobal( this, QPoint( width() + 2,
					height() + 2) );
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




/*! \brief Handle a mouse release on this trackContentObjectView.
 *
 *  If we're in move or resize mode, journal the change as appropriate.
 *  Then tidy up.
 *
 * \param _me The QMouseEvent to handle.
 */
void trackContentObjectView::mouseReleaseEvent( QMouseEvent * _me )
{
	if( m_action == Move || m_action == Resize )
	{
		m_tco->setJournalling( true );
	}
	m_action = NoAction;
	delete m_hint;
	m_hint = NULL;
	s_textFloat->hide();
	leaveEvent( NULL );
	selectableObject::mouseReleaseEvent( _me );
}




/*! \brief Set up the context menu for this trackContentObjectView.
 *
 *  Set up the various context menu events that can apply to a
 *  track content object view.
 *
 * \param _cme The QContextMenuEvent to add the actions to.
 */
void trackContentObjectView::contextMenuEvent( QContextMenuEvent * _cme )
{
	if( _cme->modifiers() )
	{
		return;
	}

	QMenu contextMenu( this );
	if( fixedTCOs() == false )
	{
		contextMenu.addAction( embed::getIconPixmap( "cancel" ),
					tr( "Delete (middle mousebutton)" ),
						this, SLOT( remove() ) );
		contextMenu.addSeparator();
		contextMenu.addAction( embed::getIconPixmap( "edit_cut" ),
					tr( "Cut" ), this, SLOT( cut() ) );
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





/*! \brief How many pixels a tact (bar) takes for this trackContentObjectView.
 *
 * \return the number of pixels per tact (bar).
 */
float trackContentObjectView::pixelsPerTact()
{
	return m_trackView->trackContainerView()->pixelsPerTact();
}




/*! \brief Set whether this trackContentObjectView can resize.
 *
 * \param _e The boolean state of whether this track content object view
 *  is allowed to resize.
 */
void trackContentObjectView::setAutoResizeEnabled( bool _e )
{
	m_autoResize = _e;
}




// ===========================================================================
// trackContentWidget
// ===========================================================================
/*! \brief Create a new trackContentWidget
 *
 *  Creates a new track content widget for the given track.
 *  The content widget comprises the 'grip bar' and the 'tools' button
 *  for the track's context menu.
 *
 * \param _track The parent track.
 */
trackContentWidget::trackContentWidget( trackView * _parent ) :
	QWidget( _parent ),
	m_trackView( _parent )
{
	setAcceptDrops( true );

	connect( _parent->trackContainerView(),
			SIGNAL( positionChanged( const MidiTime & ) ),
			this, SLOT( changePosition( const MidiTime & ) ) );

	setStyle( QApplication::style() );

	updateBackground();
}




/*! \brief Destroy this trackContentWidget
 *
 *  Destroys the trackContentWidget.
 */
trackContentWidget::~trackContentWidget()
{
}




void trackContentWidget::updateBackground()
{
	const int tactsPerBar = 4;
	const TrackContainerView * tcv = m_trackView->trackContainerView();

	// Assume even-pixels-per-tact. Makes sense, should be like this anyways
	int ppt = static_cast<int>( tcv->pixelsPerTact() );

	int w = ppt * tactsPerBar;
	int h = height();
	m_background = QPixmap( w * 2, height() );
	QPainter pmp( &m_background );

	pmp.fillRect( 0, 0, w, h, darkerColor() );
	pmp.fillRect( w, 0, w , h, lighterColor() );

	// draw lines
	pmp.setPen( QPen( QColor( 0, 0, 0, 160 ), 1 ) );
	// horizontal line
	pmp.drawLine( 0, h-1, w*2, h-1 );

	// vertical lines
	for( float x = 0; x < w * 2; x += ppt )
	{
		pmp.drawLine( QLineF( x, 0.0, x, h ) );
	}

	pmp.setPen( QPen( QColor( 140, 140, 140, 64 ), 1 ) );
	for( float x = 1.0; x < w * 2; x += ppt )
	{
		pmp.drawLine( QLineF( x, 0.0, x, h ) );
	}

	pmp.end();

	// Force redraw
	update();
}




/*! \brief Adds a trackContentObjectView to this widget.
 *
 *  Adds a(nother) trackContentObjectView to our list of views.  We also
 *  check that our position is up-to-date.
 *
 * \param _tcov The trackContentObjectView to add.
 */
void trackContentWidget::addTCOView( trackContentObjectView * _tcov )
{
	trackContentObject * tco = _tcov->getTrackContentObject();

	m_tcoViews.push_back( _tcov );

	tco->saveJournallingState( false );
	changePosition();
	tco->restoreJournallingState();
}




/*! \brief Removes the given trackContentObjectView to this widget.
 *
 *  Removes the given trackContentObjectView from our list of views.
 *
 * \param _tcov The trackContentObjectView to add.
 */
void trackContentWidget::removeTCOView( trackContentObjectView * _tcov )
{
	tcoViewVector::iterator it = qFind( m_tcoViews.begin(),
						m_tcoViews.end(),
						_tcov );
	if( it != m_tcoViews.end() )
	{
		m_tcoViews.erase( it );
		engine::getSong()->setModified();
	}
}




/*! \brief Update ourselves by updating all the tCOViews attached.
 *
 */
void trackContentWidget::update()
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
/*! \brief Move the trackContentWidget to a new place in time
 *
 * \param _new_pos The MIDI time to move to.
 */
void trackContentWidget::changePosition( const MidiTime & _new_pos )
{
	if( m_trackView->trackContainerView() == engine::getBBEditor() )
	{
		const int cur_bb = engine::getBBTrackContainer()->currentBB();
		setUpdatesEnabled( false );

		// first show TCO for current BB...
		for( tcoViewVector::iterator it = m_tcoViews.begin();
						it != m_tcoViews.end(); ++it )
		{
		if( ( *it )->getTrackContentObject()->
                            startPosition().getTact() == cur_bb )
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
		for( tcoViewVector::iterator it = m_tcoViews.begin();
					it != m_tcoViews.end(); ++it )
		{
			if( ( *it )->getTrackContentObject()->
	                            startPosition().getTact() != cur_bb )
			{
				( *it )->hide();
			}
		}
		setUpdatesEnabled( true );
		return;
	}

	MidiTime pos = _new_pos;
	if( pos < 0 )
	{
		pos = m_trackView->trackContainerView()->currentPosition();
	}

	const int begin = pos;
	const int end = endPosition( pos );
	const float ppt = m_trackView->trackContainerView()->pixelsPerTact();

	setUpdatesEnabled( false );
	for( tcoViewVector::iterator it = m_tcoViews.begin();
						it != m_tcoViews.end(); ++it )
	{
		trackContentObjectView * tcov = *it;
		trackContentObject * tco = tcov->getTrackContentObject();

		tco->changeLength( tco->length() );

		const int ts = tco->startPosition();
		const int te = tco->endPosition()-3;
		if( ( ts >= begin && ts <= end ) ||
			( te >= begin && te <= end ) ||
			( ts <= begin && te >= end ) )
		{
			tcov->move( static_cast<int>( ( ts - begin ) * ppt /
						MidiTime::ticksPerTact() ),
								tcov->y() );
			if( !tcov->isVisible() )
			{
				tcov->show();
			}
		}
		else
		{
			tcov->move( -tcov->width()-10, tcov->y() );
		}
	}
	setUpdatesEnabled( true );

	// redraw background
//	update();
}




/*! \brief Respond to a drag enter event on the trackContentWidget
 *
 * \param _dee the Drag Enter Event to respond to
 */
void trackContentWidget::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "tco_" +
					QString::number( getTrack()->type() ) );
}




/*! \brief Respond to a drop event on the trackContentWidget
 *
 * \param _de the Drop Event to respond to
 */
void trackContentWidget::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == ( "tco_" + QString::number( getTrack()->type() ) ) &&
		m_trackView->trackContainerView()->fixedTCOs() == false )
	{
		const MidiTime pos = getPosition( _de->pos().x()
                            ).getTact() * MidiTime::ticksPerTact();
		getTrack()->addJournalCheckPoint();
		trackContentObject * tco = getTrack()->createTCO( pos );

		// value contains our XML-data so simply create a
		// DataFile which does the rest for us...
		DataFile dataFile( value.toUtf8() );
		// at least save position before getting moved to somewhere
		// the user doesn't expect...
		tco->restoreState( dataFile.content().firstChild().toElement() );
		tco->movePosition( pos );

		AutomationPattern::resolveAllIDs();

		_de->accept();
	}
}






/*! \brief Respond to a mouse press on the trackContentWidget
 *
 * \param _me the mouse press event to respond to
 */
void trackContentWidget::mousePressEvent( QMouseEvent * _me )
{
	if( m_trackView->trackContainerView()->allowRubberband() == true )
	{
		QWidget::mousePressEvent( _me );
	}
	else if( _me->modifiers() & Qt::ShiftModifier )
	{
		QWidget::mousePressEvent( _me );
	}
	else if( _me->button() == Qt::LeftButton &&
			!m_trackView->trackContainerView()->fixedTCOs() )
	{
		const MidiTime pos = getPosition( _me->x() ).getTact() *
						MidiTime::ticksPerTact();
		trackContentObject * tco = getTrack()->createTCO( pos );

		tco->saveJournallingState( false );
		tco->movePosition( pos );
		tco->restoreJournallingState();

	}
}




/*! \brief Repaint the trackContentWidget on command
 *
 * \param _pe the Paint Event to respond to
 */
void trackContentWidget::paintEvent( QPaintEvent * _pe )
{
	// Assume even-pixels-per-tact. Makes sense, should be like this anyways
	const TrackContainerView * tcv = m_trackView->trackContainerView();
	int ppt = static_cast<int>( tcv->pixelsPerTact() );
	QPainter p( this );
	// Don't draw background on BB-Editor
	if( m_trackView->trackContainerView() != engine::getBBEditor() )
	{
		p.drawTiledPixmap( rect(), m_background, QPoint(
				tcv->currentPosition().getTact() * ppt, 0 ) );
	}
}




/*! \brief Updates the background tile pixmap on size changes.
 *
 * \param resizeEvent the resize event to pass to base class
 */
void trackContentWidget::resizeEvent( QResizeEvent * resizeEvent )
{
	// Update backgroud
	updateBackground();
	// Force redraw
	QWidget::resizeEvent( resizeEvent );
}




/*! \brief Return the track shown by the trackContentWidget
 *
 */
track * trackContentWidget::getTrack()
{
	return m_trackView->getTrack();
}




/*! \brief Return the position of the trackContentWidget in Tacts.
 *
 * \param _mouse_x the mouse's current X position in pixels.
 */
MidiTime trackContentWidget::getPosition( int _mouse_x )
{
	return MidiTime( m_trackView->trackContainerView()->
					currentPosition() + _mouse_x *
						MidiTime::ticksPerTact() /
			static_cast<int>( m_trackView->
				trackContainerView()->pixelsPerTact() ) );
}



/*! \brief Return the end position of the trackContentWidget in Tacts.
 *
 * \param _pos_start the starting position of the Widget (from getPosition())
 */
MidiTime trackContentWidget::endPosition( const MidiTime & _pos_start )
{
	const float ppt = m_trackView->trackContainerView()->pixelsPerTact();
	const int w = width();
	return _pos_start + static_cast<int>( w * MidiTime::ticksPerTact() / ppt );
}


// qproperty access methods
//! \brief CSS theming qproperty access method
QBrush trackContentWidget::darkerColor() const
{ return m_darkerColor; }

//! \brief CSS theming qproperty access method
QBrush trackContentWidget::lighterColor() const
{ return m_lighterColor; }

//! \brief CSS theming qproperty access method
void trackContentWidget::setDarkerColor( const QBrush & c )
{ m_darkerColor = c; }

//! \brief CSS theming qproperty access method
void trackContentWidget::setLighterColor( const QBrush & c )
{ m_lighterColor = c; }



// ===========================================================================
// trackOperationsWidget
// ===========================================================================


QPixmap * trackOperationsWidget::s_grip = NULL;     /*!< grip pixmap */


/*! \brief Create a new trackOperationsWidget
 *
 * The trackOperationsWidget is the grip and the mute button of a track.
 *
 * \param _parent the trackView to contain this widget
 */
trackOperationsWidget::trackOperationsWidget( trackView * _parent ) :
	QWidget( _parent ),             /*!< The parent widget */
	m_trackView( _parent )          /*!< The parent track view */
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


	setObjectName( "automationEnabled" );


	m_trackOps = new QPushButton( this );
	m_trackOps->move( 12, 1 );
	m_trackOps->setFocusPolicy( Qt::NoFocus );
	m_trackOps->setMenu( to_menu );
	toolTip::add( m_trackOps, tr( "Actions for this track" ) );


	m_muteBtn = new pixmapButton( this, tr( "Mute" ) );
	m_muteBtn->setActiveGraphic( embed::getIconPixmap( "led_off" ) );
	m_muteBtn->setInactiveGraphic( embed::getIconPixmap( "led_green" ) );
	m_muteBtn->setCheckable( true );

	m_soloBtn = new pixmapButton( this, tr( "Solo" ) );
	m_soloBtn->setActiveGraphic( embed::getIconPixmap( "led_red" ) );
	m_soloBtn->setInactiveGraphic( embed::getIconPixmap( "led_off" ) );
	m_soloBtn->setCheckable( true );

	if( configManager::inst()->value( "ui",
					  "compacttrackbuttons" ).toInt() )
	{
		m_muteBtn->move( 46, 0 );
		m_soloBtn->move( 46, 16 );
	}
	else
	{
		m_muteBtn->move( 46, 8 );
		m_soloBtn->move( 62, 8 );
	}

	m_muteBtn->show();
	toolTip::add( m_muteBtn, tr( "Mute this track" ) );

	m_soloBtn->show();
	toolTip::add( m_soloBtn, tr( "Solo" ) );

	connect( this, SIGNAL( trackRemovalScheduled( trackView * ) ),
			m_trackView->trackContainerView(),
				SLOT( deleteTrackView( trackView * ) ),
							Qt::QueuedConnection );
}




/*! \brief Destroy an existing trackOperationsWidget
 *
 */
trackOperationsWidget::~trackOperationsWidget()
{
}




/*! \brief Respond to trackOperationsWidget mouse events
 *
 *  If it's the left mouse button, and Ctrl is held down, and we're
 *  not a Beat+Bassline Editor track, then start a new drag event to
 *  copy this track.
 *
 *  Otherwise, ignore all other events.
 *
 *  \param _me The mouse event to respond to.
 */
void trackOperationsWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
		_me->modifiers() & Qt::ControlModifier &&
			m_trackView->getTrack()->type() != track::BBTrack )
	{
		DataFile dataFile( DataFile::DragNDropData );
		m_trackView->getTrack()->saveState( dataFile, dataFile.content() );
		new stringPairDrag( QString( "track_%1" ).arg(
					m_trackView->getTrack()->type() ),
			dataFile.toString(), QPixmap::grabWidget(
				m_trackView->getTrackSettingsWidget() ),
									this );
	}
	else if( _me->button() == Qt::LeftButton )
	{
		// track-widget (parent-widget) initiates track-move
		_me->ignore();
	}
}





/*! \brief Repaint the trackOperationsWidget
 *
 *  If we're not moving, and in the Beat+Bassline Editor, then turn
 *  automation on or off depending on its previous state and show
 *  ourselves.
 *
 *  Otherwise, hide ourselves.
 *
 *  \todo Flesh this out a bit - is it correct?
 *  \param _pe The paint event to respond to
 */
void trackOperationsWidget::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.fillRect( rect(), palette().brush(QPalette::Background) );

	if( m_trackView->isMovingTrack() == false )
	{
		p.drawPixmap( 2, 2, *s_grip );
		m_trackOps->show();
		m_muteBtn->show();
	}
	else
	{
		m_trackOps->hide();
		m_muteBtn->hide();
	}
}





/*! \brief Clone this track
 *
 */
void trackOperationsWidget::cloneTrack()
{
	engine::mixer()->lock();
	m_trackView->getTrack()->clone();
	engine::mixer()->unlock();
}


/*! \brief Clear this track - clears all TCOs from the track */
void trackOperationsWidget::clearTrack()
{
	engine::mixer()->lock();
	m_trackView->getTrack()->deleteTCOs();
	engine::mixer()->unlock();
}



/*! \brief Remove this track from the track list
 *
 */
void trackOperationsWidget::removeTrack()
{
	emit trackRemovalScheduled( m_trackView );
}




/*! \brief Update the trackOperationsWidget context menu
 *
 *  For all track types, we have the Clone and Remove options.
 *  For instrument-tracks we also offer the MIDI-control-menu
 *  For automation tracks, extra options: turn on/off recording
 *  on all TCOs (same should be added for sample tracks when
 *  sampletrack recording is implemented)
 */
void trackOperationsWidget::updateMenu()
{
	QMenu * to_menu = m_trackOps->menu();
	to_menu->clear();
	to_menu->addAction( embed::getIconPixmap( "edit_copy", 16, 16 ),
						tr( "Clone this track" ),
						this, SLOT( cloneTrack() ) );
	to_menu->addAction( embed::getIconPixmap( "cancel", 16, 16 ),
						tr( "Remove this track" ),
						this, SLOT( removeTrack() ) );
						
	if( ! m_trackView->trackContainerView()->fixedTCOs() )
	{
		to_menu->addAction( tr( "Clear this track" ), this, SLOT( clearTrack() ) );
	}

	if( dynamic_cast<InstrumentTrackView *>( m_trackView ) )
	{
		to_menu->addSeparator();
		to_menu->addMenu( dynamic_cast<InstrumentTrackView *>(
						m_trackView )->midiMenu() );
	}
	if( dynamic_cast<AutomationTrackView *>( m_trackView ) )
	{
		to_menu->addAction( tr( "Turn all recording on" ), this, SLOT( recordingOn() ) );
		to_menu->addAction( tr( "Turn all recording off" ), this, SLOT( recordingOff() ) );
	}
}


void trackOperationsWidget::recordingOn()
{
	AutomationTrackView * atv = dynamic_cast<AutomationTrackView *>( m_trackView );
	if( atv )
	{
		const track::tcoVector & tcov = atv->getTrack()->getTCOs();
		for( track::tcoVector::const_iterator it = tcov.begin(); it != tcov.end(); it++ )
		{
			AutomationPattern * ap = dynamic_cast<AutomationPattern *>( *it );
			if( ap ) { ap->setRecording( true ); }
		}
		atv->update();
	}
}


void trackOperationsWidget::recordingOff()
{
	AutomationTrackView * atv = dynamic_cast<AutomationTrackView *>( m_trackView );
	if( atv )
	{
		const track::tcoVector & tcov = atv->getTrack()->getTCOs();
		for( track::tcoVector::const_iterator it = tcov.begin(); it != tcov.end(); it++ )
		{
			AutomationPattern * ap = dynamic_cast<AutomationPattern *>( *it );
			if( ap ) { ap->setRecording( false ); }
		}
		atv->update();
	}	
}


// ===========================================================================
// track
// ===========================================================================

/*! \brief Create a new (empty) track object
 *
 *  The track object is the whole track, linking its contents, its
 *  automation, name, type, and so forth.
 *
 * \param _type The type of track (Song Editor or Beat+Bassline Editor)
 * \param _tc The track Container object to encapsulate in this track.
 *
 * \todo check the definitions of all the properties - are they OK?
 */
track::track( TrackTypes _type, TrackContainer * _tc ) :
	Model( _tc ),                   /*!< The track Model */
	m_trackContainer( _tc ),        /*!< The track container object */
	m_type( _type ),                /*!< The track type */
	m_name(),                       /*!< The track's name */
	m_mutedModel( false, this, tr( "Muted" ) ),
					 /*!< For controlling track muting */
	m_soloModel( false, this, tr( "Solo" ) ),
					/*!< For controlling track soloing */
	m_simpleSerializingMode( false ),
	m_trackContentObjects()         /*!< The track content objects (segments) */
{
	m_trackContainer->addTrack( this );
	m_height = -1;
}




/*! \brief Destroy this track
 *
 *  If the track container is a Beat+Bassline container, step through
 *  its list of tracks and remove us.
 *
 *  Then delete the trackContentObject's contents, remove this track from
 *  the track container.
 *
 *  Finally step through this track's automation and forget all of them.
 */
track::~track()
{
	emit destroyedTrack();

	while( !m_trackContentObjects.isEmpty() )
	{
		delete m_trackContentObjects.last();
	}

	m_trackContainer->removeTrack( this );
}




/*! \brief Create a track based on the given track type and container.
 *
 *  \param _tt The type of track to create
 *  \param _tc The track container to attach to
 */
track * track::create( TrackTypes _tt, TrackContainer * _tc )
{
	track * t = NULL;

	switch( _tt )
	{
		case InstrumentTrack: t = new ::InstrumentTrack( _tc ); break;
		case BBTrack: t = new bbTrack( _tc ); break;
		case SampleTrack: t = new ::SampleTrack( _tc ); break;
//		case EVENT_TRACK:
//		case VIDEO_TRACK:
		case AutomationTrack: t = new ::AutomationTrack( _tc ); break;
		case HiddenAutomationTrack:
						t = new ::AutomationTrack( _tc, true ); break;
		default: break;
	}

	_tc->updateAfterTrackAdd();

	return t;
}




/*! \brief Create a track inside TrackContainer from track type in a QDomElement and restore state from XML
 *
 *  \param _this The QDomElement containing the type of track to create
 *  \param _tc The track container to attach to
 */
track * track::create( const QDomElement & _this, TrackContainer * _tc )
{
	track * t = create(
		static_cast<TrackTypes>( _this.attribute( "type" ).toInt() ),
									_tc );
	if( t != NULL )
	{
		t->restoreState( _this );
	}
	return t;
}




/*! \brief Clone a track from this track
 *
 */
void track::clone()
{
	QDomDocument doc;
	QDomElement parent = doc.createElement( "clone" );
	saveState( doc, parent );
	create( parent.firstChild().toElement(), m_trackContainer );
}






/*! \brief Save this track's settings to file
 *
 *  We save the track type and its muted state and solo state, then append the track-
 *  specific settings.  Then we iterate through the trackContentObjects
 *  and save all their states in turn.
 *
 *  \param _doc The QDomDocument to use to save
 *  \param _this The The QDomElement to save into
 *  \todo Does this accurately describe the parameters?  I think not!?
 *  \todo Save the track height
 */
void track::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	if( !m_simpleSerializingMode )
	{
		_this.setTagName( "track" );
	}
	_this.setAttribute( "type", type() );
	_this.setAttribute( "name", name() );
	_this.setAttribute( "muted", isMuted() );
	_this.setAttribute( "solo", isSolo() );
	if( m_height >= MINIMAL_TRACK_HEIGHT )
	{
		_this.setAttribute( "height", m_height );
	}

	QDomElement ts_de = _doc.createElement( nodeName() );
	// let actual track (InstrumentTrack, bbTrack, sampleTrack etc.) save
	// its settings
	_this.appendChild( ts_de );
	saveTrackSpecificSettings( _doc, ts_de );

	if( m_simpleSerializingMode )
	{
		m_simpleSerializingMode = false;
		return;
	}

	// now save settings of all TCO's
	for( tcoVector::const_iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		( *it )->saveState( _doc, _this );
	}
}




/*! \brief Load the settings from a file
 *
 *  We load the track's type and muted state and solo state, then clear out our
 *  current trackContentObject.
 *
 *  Then we step through the QDomElement's children and load the
 *  track-specific settings and trackContentObjects states from it
 *  one at a time.
 *
 *  \param _this the QDomElement to load track settings from
 *  \todo Load the track height.
 */
void track::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "type" ).toInt() != type() )
	{
		qWarning( "Current track-type does not match track-type of "
							"settings-node!\n" );
	}

	setName( _this.hasAttribute( "name" ) ? _this.attribute( "name" ) :
			_this.firstChild().toElement().attribute( "name" ) );

	setMuted( _this.attribute( "muted" ).toInt() );
	setSolo( _this.attribute( "solo" ).toInt() );

	if( m_simpleSerializingMode )
	{
		QDomNode node = _this.firstChild();
		while( !node.isNull() )
		{
			if( node.isElement() && node.nodeName() == nodeName() )
			{
				loadTrackSpecificSettings( node.toElement() );
				break;
			}
			node = node.nextSibling();
		}
		m_simpleSerializingMode = false;
		return;
	}

	while( !m_trackContentObjects.empty() )
	{
		delete m_trackContentObjects.front();
//		m_trackContentObjects.erase( m_trackContentObjects.begin() );
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
								MidiTime( 0 ) );
				tco->restoreState( node.toElement() );
				saveJournallingState( false );
				restoreJournallingState();
			}
		}
		node = node.nextSibling();
	}

	if( _this.attribute( "height" ).toInt() >= MINIMAL_TRACK_HEIGHT &&
		_this.attribute( "height" ).toInt() <= DEFAULT_TRACK_HEIGHT )	// workaround for #3585927, tobydox/2012-11-11
	{
		m_height = _this.attribute( "height" ).toInt();
	}
}




/*! \brief Add another trackContentObject into this track
 *
 *  \param _tco The trackContentObject to attach to this track.
 */
trackContentObject * track::addTCO( trackContentObject * _tco )
{
	m_trackContentObjects.push_back( _tco );

	emit trackContentObjectAdded( _tco );

	return _tco;		// just for convenience
}




/*! \brief Remove a given trackContentObject from this track
 *
 *  \param _tco The trackContentObject to remove from this track.
 */
void track::removeTCO( trackContentObject * _tco )
{
	tcoVector::iterator it = qFind( m_trackContentObjects.begin(),
					m_trackContentObjects.end(),
					_tco );
	if( it != m_trackContentObjects.end() )
	{
		m_trackContentObjects.erase( it );
		if( engine::getSong() )
		{
			engine::getSong()->updateLength();
			engine::getSong()->setModified();
		}
	}
}


/*! \brief Remove all TCOs from this track */
void track::deleteTCOs()
{
	while( ! m_trackContentObjects.isEmpty() )
	{
		delete m_trackContentObjects.first();
	}
}


/*! \brief Return the number of trackContentObjects we contain
 *
 *  \return the number of trackContentObjects we currently contain.
 */
int track::numOfTCOs()
{
	return m_trackContentObjects.size();
}




/*! \brief Get a trackContentObject by number
 *
 *  If the TCO number is less than our TCO array size then fetch that
 *  numbered object from the array.  Otherwise we warn the user that
 *  we've somehow requested a TCO that is too large, and create a new
 *  TCO for them.
 *  \param _tco_number The number of the trackContentObject to fetch.
 *  \return the given trackContentObject or a new one if out of range.
 *  \todo reject TCO numbers less than zero.
 *  \todo if we create a TCO here, should we somehow attach it to the
 *     track?
 */
trackContentObject * track::getTCO( int _tco_num )
{
	if( _tco_num < m_trackContentObjects.size() )
	{
		return m_trackContentObjects[_tco_num];
	}
	printf( "called track::getTCO( %d ), "
			"but TCO %d doesn't exist\n", _tco_num, _tco_num );
	return createTCO( _tco_num * MidiTime::ticksPerTact() );

}




/*! \brief Determine the given trackContentObject's number in our array.
 *
 *  \param _tco The trackContentObject to search for.
 *  \return its number in our array.
 */
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
			return i;
		}*/
		return it - m_trackContentObjects.begin();
	}
	qWarning( "track::getTCONum(...) -> _tco not found!\n" );
	return 0;
}




/*! \brief Retrieve a list of trackContentObjects that fall within a period.
 *
 *  Here we're interested in a range of trackContentObjects that fall
 *  completely within a given time period - their start must be no earlier
 *  than the given start time and their end must be no later than the given
 *  end time.
 *
 *  We return the TCOs we find in order by time, earliest TCOs first.
 *
 *  \param _tco_c The list to contain the found trackContentObjects.
 *  \param _start The MIDI start time of the range.
 *  \param _end   The MIDI endi time of the range.
 */
void track::getTCOsInRange( tcoVector & _tco_v, const MidiTime & _start,
							const MidiTime & _end )
{
	for( tcoVector::iterator it_o = m_trackContentObjects.begin();
				it_o != m_trackContentObjects.end(); ++it_o )
	{
		trackContentObject * tco = ( *it_o );
		int s = tco->startPosition();
		int e = tco->endPosition();
		if( ( s <= _end ) && ( e >= _start ) )
		{
			// ok, TCO is posated within given range
			// now let's search according position for TCO in list
			// 	-> list is ordered by TCO's position afterwards
			bool inserted = false;
			for( tcoVector::iterator it = _tco_v.begin();
						it != _tco_v.end(); ++it )
			{
				if( ( *it )->startPosition() >= s )
				{
					_tco_v.insert( it, tco );
					inserted = true;
					break;
				}
			}
			if( inserted == false )
			{
				// no TCOs found posated behind current TCO...
				_tco_v.push_back( tco );
			}
		}
	}
}




/*! \brief Swap the position of two trackContentObjects.
 *
 *  First, we arrange to swap the positions of the two TCOs in the
 *  trackContentObjects list.  Then we swap their start times as well.
 *
 *  \param _tco_num1 The first trackContentObject to swap.
 *  \param _tco_num2 The second trackContentObject to swap.
 */
void track::swapPositionOfTCOs( int _tco_num1, int _tco_num2 )
{
	qSwap( m_trackContentObjects[_tco_num1],
					m_trackContentObjects[_tco_num2] );

	const MidiTime pos = m_trackContentObjects[_tco_num1]->startPosition();

	m_trackContentObjects[_tco_num1]->movePosition(
			m_trackContentObjects[_tco_num2]->startPosition() );
	m_trackContentObjects[_tco_num2]->movePosition( pos );
}




/*! \brief Move all the trackContentObjects after a certain time later by one bar.
 *
 *  \param _pos The time at which we want to insert the bar.
 *  \todo if we stepped through this list last to first, and the list was
 *    in ascending order by TCO time, once we hit a TCO that was earlier
 *    than the insert time, we could fall out of the loop early.
 */
void track::insertTact( const MidiTime & _pos )
{
	// we'll increase the position of every TCO, positioned behind _pos, by
	// one tact
	for( tcoVector::iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		if( ( *it )->startPosition() >= _pos )
		{
			( *it )->movePosition( (*it)->startPosition() +
						MidiTime::ticksPerTact() );
		}
	}
}




/*! \brief Move all the trackContentObjects after a certain time earlier by one bar.
 *
 *  \param _pos The time at which we want to remove the bar.
 */
void track::removeTact( const MidiTime & _pos )
{
	// we'll decrease the position of every TCO, positioned behind _pos, by
	// one tact
	for( tcoVector::iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		if( ( *it )->startPosition() >= _pos )
		{
			( *it )->movePosition( qMax( ( *it )->startPosition() -
						MidiTime::ticksPerTact(), 0 ) );
		}
	}
}




/*! \brief Return the length of the entire track in bars
 *
 *  We step through our list of TCOs and determine their end position,
 *  keeping track of the latest time found in ticks.  Then we return
 *  that in bars by dividing by the number of ticks per bar.
 */
tact_t track::length() const
{
	// find last end-position
	tick_t last = 0;
	for( tcoVector::const_iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		const tick_t cur = ( *it )->endPosition();
		if( cur > last )
		{
			last = cur;
		}
	}

	return last / MidiTime::ticksPerTact();
}



/*! \brief Invert the track's solo state.
 *
 *  We have to go through all the tracks determining if any other track
 *  is already soloed.  Then we have to save the mute state of all tracks,
 *  and set our mute state to on and all the others to off.
 */
void track::toggleSolo()
{
	const TrackContainer::TrackList & tl = m_trackContainer->tracks();

	bool solo_before = false;
	for( TrackContainer::TrackList::const_iterator it = tl.begin();
							it != tl.end(); ++it )
	{
		if( *it != this )
		{
			if( ( *it )->m_soloModel.value() )
			{
				solo_before = true;
				break;
			}
		}
	}

	const bool solo = m_soloModel.value();
	for( TrackContainer::TrackList::const_iterator it = tl.begin();
							it != tl.end(); ++it )
	{
		if( solo )
		{
			// save mute-state in case no track was solo before
			if( !solo_before )
			{
				( *it )->m_mutedBeforeSolo = ( *it )->isMuted();
			}
			( *it )->setMuted( *it == this ? false : true );
			if( *it != this )
			{
				( *it )->m_soloModel.setValue( false );
			}
		}
		else if( !solo_before )
		{
			( *it )->setMuted( ( *it )->m_mutedBeforeSolo );
		}
	}
}






// ===========================================================================
// trackView
// ===========================================================================

/*! \brief Create a new track View.
 *
 *  The track View is handles the actual display of the track, including
 *  displaying its various widgets and the track segments.
 *
 *  \param _track The track to display.
 *  \param _tcv The track Container View for us to be displayed in.
 *  \todo Is my description of these properties correct?
 */
trackView::trackView( track * _track, TrackContainerView * _tcv ) :
	QWidget( _tcv->contentWidget() ),   /*!< The Track Container View's content widget. */
	ModelView( NULL, this ),            /*!< The model view of this track */
	m_track( _track ),                  /*!< The track we're displaying */
	m_trackContainerView( _tcv ),       /*!< The track Container View we're displayed in */
	m_trackOperationsWidget( this ),    /*!< Our trackOperationsWidget */
	m_trackSettingsWidget( this ),      /*!< Our trackSettingsWidget */
	m_trackContentWidget( this ),       /*!< Our trackContentWidget */
	m_action( NoAction )                /*!< The action we're currently performing */
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setColor( backgroundRole(), QColor( 32, 36, 40 ) );
	setPalette( pal );

	m_trackSettingsWidget.setAutoFillBackground( true );

	QHBoxLayout * layout = new QHBoxLayout( this );
	layout->setMargin( 0 );
	layout->setSpacing( 0 );
	layout->addWidget( &m_trackOperationsWidget );
	layout->addWidget( &m_trackSettingsWidget );
	layout->addWidget( &m_trackContentWidget, 1 );
	setFixedHeight( m_track->getHeight() );

	resizeEvent( NULL );

	setAcceptDrops( true );
	setAttribute( Qt::WA_DeleteOnClose, true );


	connect( m_track, SIGNAL( destroyedTrack() ), this, SLOT( close() ) );
	connect( m_track,
		SIGNAL( trackContentObjectAdded( trackContentObject * ) ),
			this, SLOT( createTCOView( trackContentObject * ) ),
			Qt::QueuedConnection );

	connect( &m_track->m_mutedModel, SIGNAL( dataChanged() ),
			&m_trackContentWidget, SLOT( update() ) );

	connect( &m_track->m_soloModel, SIGNAL( dataChanged() ),
			m_track, SLOT( toggleSolo() ) );
	// create views for already existing TCOs
	for( track::tcoVector::iterator it =
					m_track->m_trackContentObjects.begin();
			it != m_track->m_trackContentObjects.end(); ++it )
	{
		createTCOView( *it );
	}

	m_trackContainerView->addTrackView( this );
}




/*! \brief Destroy this track View.
 *
 */
trackView::~trackView()
{
}




/*! \brief Resize this track View.
 *
 *  \param _re the Resize Event to handle.
 */
void trackView::resizeEvent( QResizeEvent * _re )
{
	if( configManager::inst()->value( "ui",
					  "compacttrackbuttons" ).toInt() )
	{
		m_trackOperationsWidget.setFixedSize( TRACK_OP_WIDTH_COMPACT, height() - 1 );
		m_trackSettingsWidget.setFixedSize( DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT, height() - 1 );
	}
	else
	{
		m_trackOperationsWidget.setFixedSize( TRACK_OP_WIDTH, height() - 1 );
		m_trackSettingsWidget.setFixedSize( DEFAULT_SETTINGS_WIDGET_WIDTH, height() - 1 );
	}
	m_trackContentWidget.setFixedHeight( height() );
}




/*! \brief Update this track View and all its content objects.
 *
 */
void trackView::update()
{
	m_trackContentWidget.update();
	if( !m_trackContainerView->fixedTCOs() )
	{
		m_trackContentWidget.changePosition();
	}
	QWidget::update();
}




/*! \brief Close this track View.
 *
 */
bool trackView::close()
{
	m_trackContainerView->removeTrackView( this );
	return QWidget::close();
}




/*! \brief Register that the model of this track View has changed.
 *
 */
void trackView::modelChanged()
{
	m_track = castModel<track>();
	assert( m_track != NULL );
	connect( m_track, SIGNAL( destroyedTrack() ), this, SLOT( close() ) );
	m_trackOperationsWidget.m_muteBtn->setModel( &m_track->m_mutedModel );
	m_trackOperationsWidget.m_soloBtn->setModel( &m_track->m_soloModel );
	ModelView::modelChanged();
	setFixedHeight( m_track->getHeight() );
}




/*! \brief Start a drag event on this track View.
 *
 *  \param _dee the DragEnterEvent to start.
 */
void trackView::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "track_" +
					QString::number( m_track->type() ) );
}




/*! \brief Accept a drop event on this track View.
 *
 *  We only accept drop events that are of the same type as this track.
 *  If so, we decode the data from the drop event by just feeding it
 *  back into the engine as a state.
 *
 *  \param _de the DropEvent to handle.
 */
void trackView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == ( "track_" + QString::number( m_track->type() ) ) )
	{
		// value contains our XML-data so simply create a
		// DataFile which does the rest for us...
		DataFile dataFile( value.toUtf8() );
		engine::mixer()->lock();
		m_track->restoreState( dataFile.content().firstChild().toElement() );
		engine::mixer()->unlock();
		_de->accept();
	}
}




/*! \brief Handle a mouse press event on this track View.
 *
 *  If this track container supports rubber band selection, let the
 *  widget handle that and don't bother with any other handling.
 *
 *  If the left mouse button is pressed, we handle two things.  If
 *  SHIFT is pressed, then we resize vertically.  Otherwise we start
 *  the process of moving this track to a new position.
 *
 *  Otherwise we let the widget handle the mouse event as normal.
 *
 *  \param _me the MouseEvent to handle.
 */
void trackView::mousePressEvent( QMouseEvent * _me )
{
	// If previously dragged too small, restore on shift-leftclick
	if( height() < DEFAULT_TRACK_HEIGHT &&
		_me->modifiers() & Qt::ShiftModifier &&
		_me->button() == Qt::LeftButton )
	{
		setFixedHeight( DEFAULT_TRACK_HEIGHT );
		m_track->setHeight( DEFAULT_TRACK_HEIGHT );
	}


	if( m_trackContainerView->allowRubberband() == true )
	{
		QWidget::mousePressEvent( _me );
	}
	else if( _me->button() == Qt::LeftButton )
	{
		if( _me->modifiers() & Qt::ShiftModifier )
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




/*! \brief Handle a mouse move event on this track View.
 *
 *  If this track container supports rubber band selection, let the
 *  widget handle that and don't bother with any other handling.
 *
 *  Otherwise if we've started the move process (from mousePressEvent())
 *  then move ourselves into that position, reordering the track list
 *  with moveTrackViewUp() and moveTrackViewDown() to suit.  We make a
 *  note of this in the undo journal in case the user wants to undo this
 *  move.
 *
 *  Likewise if we've started a resize process, handle this too, making
 *  sure that we never go below the minimum track height.
 *
 *  \param _me the MouseEvent to handle.
 */
void trackView::mouseMoveEvent( QMouseEvent * _me )
{

	if( m_trackContainerView->allowRubberband() == true )
	{
		QWidget::mouseMoveEvent( _me );
	}
	else if( m_action == MoveTrack )
	{
		// look which track-widget the mouse-cursor is over
		const int y_pos = m_trackContainerView->contentWidget()->mapFromGlobal( _me->globalPos() ).y();
		const trackView * track_at_y =	m_trackContainerView->trackViewAt( y_pos );

// debug code
//			qDebug( "y position %d", y_pos );

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
		}
	}
	else if( m_action == ResizeTrack )
	{
		setFixedHeight( qMax<int>( _me->y(), MINIMAL_TRACK_HEIGHT ) );
		m_trackContainerView->realignTracks();
		m_track->setHeight( height() );
	}

	if( height() < DEFAULT_TRACK_HEIGHT )
	{
		toolTip::add( this, m_track->m_name );
	}
}



/*! \brief Handle a mouse release event on this track View.
 *
 *  \param _me the MouseEvent to handle.
 */
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




/*! \brief Repaint this track View.
 *
 *  \param _pe the PaintEvent to start.
 */
void trackView::paintEvent( QPaintEvent * _pe )
{
	QStyleOption opt;
	opt.initFrom( this );
	QPainter p( this );
	style()->drawPrimitive( QStyle::PE_Widget, &opt, &p, this );
}




/*! \brief Create a trackContentObject View in this track View.
 *
 *  \param _tco the trackContentObject to create the view for.
 *  \todo is this a good description for what this method does?
 */
void trackView::createTCOView( trackContentObject * _tco )
{
	_tco->createView( this );
}





#include "moc_track.cxx"


