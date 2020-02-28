/*
 * Track.cpp - implementation of classes concerning tracks -> necessary for
 *             all track-like objects (beat/bassline, sample-track...)
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

/** \file Track.cpp
 *  \brief All classes concerning tracks and track-like objects
 */

/*
 * \mainpage Track classes
 *
 * \section introduction Introduction
 *
 * \todo fill this out
 */

#include "Track.h"

#include <assert.h>

#include <QLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>


#include "AutomationPattern.h"
#include "AutomationTrack.h"
#include "AutomationEditor.h"
#include "BBEditor.h"
#include "BBTrack.h"
#include "BBTrackContainer.h"
#include "ConfigManager.h"
#include "Clipboard.h"
#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "FxMixerView.h"
#include "gui_templates.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "ProjectJournal.h"
#include "SampleTrack.h"
#include "Song.h"
#include "SongEditor.h"
#include "StringPairDrag.h"
#include "TextFloat.h"


/*! The width of the resize grip in pixels
 */
const int RESIZE_GRIP_WIDTH = 4;

/*! Alternate between a darker and a lighter background color every 4 bars
 */
const int BARS_PER_GROUP = 4;


/*! A pointer for that text bubble used when moving segments, etc.
 *
 * In a number of situations, LMMS displays a floating text bubble
 * beside the cursor as you move or resize elements of a track about.
 * This pointer keeps track of it, as you only ever need one at a time.
 */
TextFloat * TrackContentObjectView::s_textFloat = NULL;


// ===========================================================================
// TrackContentObject
// ===========================================================================
/*! \brief Create a new TrackContentObject
 *
 *  Creates a new track content object for the given track.
 *
 * \param _track The track that will contain the new object
 */
TrackContentObject::TrackContentObject( Track * track ) :
	Model( track ),
	m_track( track ),
	m_startPosition(),
	m_length(),
	m_mutedModel( false, this, tr( "Mute" ) ),
	m_selectViewOnCreate( false )
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




/*! \brief Destroy a TrackContentObject
 *
 *  Destroys the given track content object.
 *
 */
TrackContentObject::~TrackContentObject()
{
	emit destroyedTCO();

	if( getTrack() )
	{
		getTrack()->removeTCO( this );
	}
}




/*! \brief Move this TrackContentObject's position in time
 *
 *  If the track content object has moved, update its position.  We
 *  also add a journal entry for undo and update the display.
 *
 * \param _pos The new position of the track content object.
 */
void TrackContentObject::movePosition( const MidiTime & pos )
{
	if( m_startPosition != pos )
	{
		Engine::mixer()->requestChangeInModel();
		m_startPosition = pos;
		Engine::mixer()->doneChangeInModel();
		Engine::getSong()->updateLength();
		emit positionChanged();
	}
}




/*! \brief Change the length of this TrackContentObject
 *
 *  If the track content object's length has chaanged, update it.  We
 *  also add a journal entry for undo and update the display.
 *
 * \param _length The new length of the track content object.
 */
void TrackContentObject::changeLength( const MidiTime & length )
{
	m_length = length;
	Engine::getSong()->updateLength();
	emit lengthChanged();
}




bool TrackContentObject::comparePosition(const TrackContentObject *a, const TrackContentObject *b)
{
	return a->startPosition() < b->startPosition();
}




/*! \brief Copy this TrackContentObject to the clipboard.
 *
 *  Copies this track content object to the clipboard.
 */
void TrackContentObject::copy()
{
	Clipboard::copy( this );
}




/*! \brief Pastes this TrackContentObject into a track.
 *
 *  Pastes this track content object into a track.
 *
 * \param _je The journal entry to undo
 */
void TrackContentObject::paste()
{
	if( Clipboard::getContent( nodeName() ) != NULL )
	{
		const MidiTime pos = startPosition();
		restoreState( *( Clipboard::getContent( nodeName() ) ) );
		movePosition( pos );
	}
	AutomationPattern::resolveAllIDs();
	GuiApplication::instance()->automationEditor()->m_editor->updateAfterPatternChange();
}




/*! \brief Mutes this TrackContentObject
 *
 *  Restore the previous state of this track content object.  This will
 *  restore the position or the length of the track content object
 *  depending on what was changed.
 *
 * \param _je The journal entry to undo
 */
void TrackContentObject::toggleMute()
{
	m_mutedModel.setValue( !m_mutedModel.value() );
	emit dataChanged();
}




MidiTime TrackContentObject::startTimeOffset() const
{
	return m_startTimeOffset;
}




void TrackContentObject::setStartTimeOffset( const MidiTime &startTimeOffset )
{
	m_startTimeOffset = startTimeOffset;
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
TrackContentObjectView::TrackContentObjectView( TrackContentObject * tco,
							TrackView * tv ) :
	selectableObject( tv->getTrackContentWidget() ),
	ModelView( NULL, this ),
	m_tco( tco ),
	m_trackView( tv ),
	m_action( NoAction ),
	m_initialMousePos( QPoint( 0, 0 ) ),
	m_initialMouseGlobalPos( QPoint( 0, 0 ) ),
	m_initialTCOPos( MidiTime(0) ),
	m_initialTCOEnd( MidiTime(0) ),
	m_initialOffsets( QVector<MidiTime>() ),
	m_hint( NULL ),
	m_mutedColor( 0, 0, 0 ),
	m_mutedBackgroundColor( 0, 0, 0 ),
	m_selectedColor( 0, 0, 0 ),
	m_textColor( 0, 0, 0 ),
	m_textShadowColor( 0, 0, 0 ),
	m_BBPatternBackground( 0, 0, 0 ),
	m_gradient( true ),
	m_needsUpdate( true )
{
	if( s_textFloat == NULL )
	{
		s_textFloat = new TextFloat;
		s_textFloat->setPixmap( embed::getIconPixmap( "clock" ) );
	}

	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setAttribute( Qt::WA_DeleteOnClose, true );
	setFocusPolicy( Qt::StrongFocus );
	setCursor( QCursor( embed::getIconPixmap( "hand" ), 3, 3 ) );
	move( 0, 0 );
	show();

	setFixedHeight( tv->getTrackContentWidget()->height() - 1);
	setAcceptDrops( true );
	setMouseTracking( true );

	connect( m_tco, SIGNAL( lengthChanged() ),
			this, SLOT( updateLength() ) );
	connect( gui->songEditor()->m_editor->zoomingModel(), SIGNAL( dataChanged() ), this, SLOT( updateLength() ) );
	connect( m_tco, SIGNAL( positionChanged() ),
			this, SLOT( updatePosition() ) );
	connect( m_tco, SIGNAL( destroyedTCO() ), this, SLOT( close() ) );
	setModel( m_tco );

	m_trackView->getTrackContentWidget()->addTCOView( this );
	updateLength();
	updatePosition();
}




/*! \brief Destroy a trackContentObjectView
 *
 *  Destroys the given track content object view.
 *
 */
TrackContentObjectView::~TrackContentObjectView()
{
	delete m_hint;
	// we have to give our track-container the focus because otherwise the
	// op-buttons of our track-widgets could become focus and when the user
	// presses space for playing song, just one of these buttons is pressed
	// which results in unwanted effects
	m_trackView->trackContainerView()->setFocus();
}


/*! \brief Update a TrackContentObjectView
 *
 *  TCO's get drawn only when needed,
 *  and when a TCO is updated,
 *  it needs to be redrawn.
 *
 */
void TrackContentObjectView::update()
{
	if( fixedTCOs() )
	{
		updateLength();
	}
	m_needsUpdate = true;
	selectableObject::update();
}



/*! \brief Does this trackContentObjectView have a fixed TCO?
 *
 *  Returns whether the containing trackView has fixed
 *  TCOs.
 *
 * \todo What the hell is a TCO here - track content object?  And in
 *  what circumstance are they fixed?
 */
bool TrackContentObjectView::fixedTCOs()
{
	return m_trackView->trackContainerView()->fixedTCOs();
}



// qproperty access functions, to be inherited & used by TCOviews
//! \brief CSS theming qproperty access method
QColor TrackContentObjectView::mutedColor() const
{ return m_mutedColor; }

QColor TrackContentObjectView::mutedBackgroundColor() const
{ return m_mutedBackgroundColor; }

QColor TrackContentObjectView::selectedColor() const
{ return m_selectedColor; }

QColor TrackContentObjectView::textColor() const
{ return m_textColor; }

QColor TrackContentObjectView::textBackgroundColor() const
{
	return m_textBackgroundColor;
}

QColor TrackContentObjectView::textShadowColor() const
{ return m_textShadowColor; }

QColor TrackContentObjectView::BBPatternBackground() const
{ return m_BBPatternBackground; }

bool TrackContentObjectView::gradient() const
{ return m_gradient; }

//! \brief CSS theming qproperty access method
void TrackContentObjectView::setMutedColor( const QColor & c )
{ m_mutedColor = QColor( c ); }

void TrackContentObjectView::setMutedBackgroundColor( const QColor & c )
{ m_mutedBackgroundColor = QColor( c ); }

void TrackContentObjectView::setSelectedColor( const QColor & c )
{ m_selectedColor = QColor( c ); }

void TrackContentObjectView::setTextColor( const QColor & c )
{ m_textColor = QColor( c ); }

void TrackContentObjectView::setTextBackgroundColor( const QColor & c )
{
	m_textBackgroundColor = c;
}

void TrackContentObjectView::setTextShadowColor( const QColor & c )
{ m_textShadowColor = QColor( c ); }

void TrackContentObjectView::setBBPatternBackground( const QColor & c )
{ m_BBPatternBackground = QColor( c ); }

void TrackContentObjectView::setGradient( const bool & b )
{ m_gradient = b; }

// access needsUpdate member variable
bool TrackContentObjectView::needsUpdate()
{ return m_needsUpdate; }
void TrackContentObjectView::setNeedsUpdate( bool b )
{ m_needsUpdate = b; }

/*! \brief Close a trackContentObjectView
 *
 *  Closes a track content object view by asking the track
 *  view to remove us and then asking the QWidget to close us.
 *
 * \return Boolean state of whether the QWidget was able to close.
 */
bool TrackContentObjectView::close()
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
void TrackContentObjectView::remove()
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
void TrackContentObjectView::cut()
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
void TrackContentObjectView::updateLength()
{
	if( fixedTCOs() )
	{
		setFixedWidth( parentWidget()->width() );
	}
	else
	{
		setFixedWidth(
		static_cast<int>( m_tco->length() * pixelsPerBar() /
					MidiTime::ticksPerBar() ) + 1 /*+
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
void TrackContentObjectView::updatePosition()
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
 * \param dee The QDragEnterEvent to watch.
 */
void TrackContentObjectView::dragEnterEvent( QDragEnterEvent * dee )
{
	TrackContentWidget * tcw = getTrackView()->getTrackContentWidget();
	MidiTime tcoPos = MidiTime( m_tco->startPosition() );

	if( tcw->canPasteSelection( tcoPos, dee ) == false )
	{
		dee->ignore();
	}
	else
	{
		StringPairDrag::processDragEnterEvent( dee, "tco_" +
					QString::number( m_tco->getTrack()->type() ) );
	}
}




/*! \brief Handle something being dropped on this trackContentObjectView.
 *
 *  When something has been dropped on this trackContentObjectView, and
 *  it's a track content object, then use an instance of our dataFile reader
 *  to take the xml of the track content object and turn it into something
 *  we can write over our current state.
 *
 * \param de The QDropEvent to handle.
 */
void TrackContentObjectView::dropEvent( QDropEvent * de )
{
	QString type = StringPairDrag::decodeKey( de );
	QString value = StringPairDrag::decodeValue( de );

	// Track must be the same type to paste into
	if( type != ( "tco_" + QString::number( m_tco->getTrack()->type() ) ) )
	{
		return;
	}

	// Defer to rubberband paste if we're in that mode
	if( m_trackView->trackContainerView()->allowRubberband() == true )
	{
		TrackContentWidget * tcw = getTrackView()->getTrackContentWidget();
		MidiTime tcoPos = MidiTime( m_tco->startPosition() );

		if( tcw->pasteSelection( tcoPos, de ) == true )
		{
			de->accept();
		}
		return;
	}

	// Don't allow pasting a tco into itself.
	QObject* qwSource = de->source();
	if( qwSource != NULL &&
	    dynamic_cast<TrackContentObjectView *>( qwSource ) == this )
	{
		return;
	}

	// Copy state into existing tco
	DataFile dataFile( value.toUtf8() );
	MidiTime pos = m_tco->startPosition();
	QDomElement tcos = dataFile.content().firstChildElement( "tcos" );
	m_tco->restoreState( tcos.firstChildElement().firstChildElement() );
	m_tco->movePosition( pos );
	AutomationPattern::resolveAllIDs();
	de->accept();
}




/*! \brief Handle a dragged selection leaving our 'airspace'.
 *
 * \param e The QEvent to watch.
 */
void TrackContentObjectView::leaveEvent( QEvent * e )
{
	if( cursor().shape() != Qt::BitmapCursor )
	{
		setCursor( QCursor( embed::getIconPixmap( "hand" ), 3, 3 ) );
	}
	if( e != NULL )
	{
		QWidget::leaveEvent( e );
	}
}

/*! \brief Create a DataFile suitable for copying multiple trackContentObjects.
 *
 *	trackContentObjects in the vector are written to the "tcos" node in the
 *  DataFile.  The trackContentObjectView's initial mouse position is written
 *  to the "initialMouseX" node in the DataFile.  When dropped on a track,
 *  this is used to create copies of the TCOs.
 *
 * \param tcos The trackContectObjects to save in a DataFile
 */
DataFile TrackContentObjectView::createTCODataFiles(
    				const QVector<TrackContentObjectView *> & tcoViews) const
{
	Track * t = m_trackView->getTrack();
	TrackContainer * tc = t->trackContainer();
	DataFile dataFile( DataFile::DragNDropData );
	QDomElement tcoParent = dataFile.createElement( "tcos" );

	typedef QVector<TrackContentObjectView *> tcoViewVector;
	for( tcoViewVector::const_iterator it = tcoViews.begin();
			it != tcoViews.end(); ++it )
	{
		// Insert into the dom under the "tcos" element
		Track* tcoTrack = ( *it )->m_trackView->getTrack();
		int trackIndex = tc->tracks().indexOf( tcoTrack );
		QDomElement tcoElement = dataFile.createElement( "tco" );
		tcoElement.setAttribute( "trackIndex", trackIndex );
		tcoElement.setAttribute( "trackType", tcoTrack->type() );
		tcoElement.setAttribute( "trackName", tcoTrack->name() );
		( *it )->m_tco->saveState( dataFile, tcoElement );
		tcoParent.appendChild( tcoElement );
	}

	dataFile.content().appendChild( tcoParent );

	// Add extra metadata needed for calculations later
	int initialTrackIndex = tc->tracks().indexOf( t );
	if( initialTrackIndex < 0 )
	{
		printf("Failed to find selected track in the TrackContainer.\n");
		return dataFile;
	}
	QDomElement metadata = dataFile.createElement( "copyMetadata" );
	// initialTrackIndex is the index of the track that was touched
	metadata.setAttribute( "initialTrackIndex", initialTrackIndex );
	metadata.setAttribute( "trackContainerId", tc->id() );
	// grabbedTCOPos is the pos of the bar containing the TCO we grabbed
	metadata.setAttribute( "grabbedTCOPos", m_tco->startPosition() );

	dataFile.content().appendChild( metadata );

	return dataFile;
}

void TrackContentObjectView::paintTextLabel(QString const & text, QPainter & painter)
{
	if (text.trimmed() == "")
	{
		return;
	}

	painter.setRenderHint( QPainter::TextAntialiasing );

	QFont labelFont = this->font();
	labelFont.setHintingPreference( QFont::PreferFullHinting );
	painter.setFont( labelFont );

	const int textTop = TCO_BORDER_WIDTH + 1;
	const int textLeft = TCO_BORDER_WIDTH + 3;

	QFontMetrics fontMetrics(labelFont);
	QString elidedPatternName = fontMetrics.elidedText(text, Qt::ElideMiddle, width() - 2 * textLeft);

	if (elidedPatternName.length() < 2)
	{
		elidedPatternName = text.trimmed();
	}

	painter.fillRect(QRect(0, 0, width(), fontMetrics.height() + 2 * textTop), textBackgroundColor());

	int const finalTextTop = textTop + fontMetrics.ascent();
	painter.setPen(textShadowColor());
	painter.drawText( textLeft + 1, finalTextTop + 1, elidedPatternName );
	painter.setPen( textColor() );
	painter.drawText( textLeft, finalTextTop, elidedPatternName );
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
 * \param me The QMouseEvent to handle.
 */
void TrackContentObjectView::mousePressEvent( QMouseEvent * me )
{
	setInitialPos( me->pos() );
	setInitialOffsets();
	if( !fixedTCOs() && me->button() == Qt::LeftButton )
	{
		if( me->modifiers() & Qt::ControlModifier )
		{
			if( isSelected() )
			{
				m_action = CopySelection;
			}
			else
			{
				m_action = ToggleSelected;
			}
		}
		else if( !me->modifiers()
			|| (me->modifiers() & Qt::AltModifier)
			|| (me->modifiers() & Qt::ShiftModifier) )
		{
			if( isSelected() )
			{
				m_action = MoveSelection;
			}
			else
			{
				gui->songEditor()->m_editor->selectAllTcos( false );
				m_tco->addJournalCheckPoint();

				// move or resize
				m_tco->setJournalling( false );

				setInitialPos( me->pos() );
				setInitialOffsets();

				SampleTCO * sTco = dynamic_cast<SampleTCO*>( m_tco );
				if( me->x() < RESIZE_GRIP_WIDTH && sTco
						&& !m_tco->getAutoResize() )
				{
					m_action = ResizeLeft;
					setCursor( Qt::SizeHorCursor );
				}
				else if( me->x() < width() - RESIZE_GRIP_WIDTH )
				{
					m_action = Move;
					setCursor( Qt::SizeAllCursor );
				}
				else if( !m_tco->getAutoResize() )
				{
					m_action = Resize;
					setCursor( Qt::SizeHorCursor );
				}

				if( m_action == Move )
				{
					s_textFloat->setTitle( tr( "Current position" ) );
					s_textFloat->setText( QString( "%1:%2" ).
						arg( m_tco->startPosition().getBar() + 1 ).
						arg( m_tco->startPosition().getTicks() %
								MidiTime::ticksPerBar() ) );
				}
				else if( m_action == Resize || m_action == ResizeLeft )
				{
					s_textFloat->setTitle( tr( "Current length" ) );
					s_textFloat->setText( tr( "%1:%2 (%3:%4 to %5:%6)" ).
							arg( m_tco->length().getBar() ).
							arg( m_tco->length().getTicks() %
									MidiTime::ticksPerBar() ).
							arg( m_tco->startPosition().getBar() + 1 ).
							arg( m_tco->startPosition().getTicks() %
									MidiTime::ticksPerBar() ).
							arg( m_tco->endPosition().getBar() + 1 ).
							arg( m_tco->endPosition().getTicks() %
									MidiTime::ticksPerBar() ) );
				}
				// s_textFloat->reparent( this );
				// setup text-float as if TCO was already moved/resized
				s_textFloat->moveGlobal( this, QPoint( width() + 2, height() + 2) );
				s_textFloat->show();
			}

			delete m_hint;
			QString hint = m_action == Move || m_action == MoveSelection
						? tr( "Press <%1> and drag to make a copy." )
						: tr( "Press <%1> for free resizing." );
			m_hint = TextFloat::displayMessage( tr( "Hint" ), hint.arg(UI_CTRL_KEY),
					embed::getIconPixmap( "hint" ), 0 );
		}
	}
	else if( me->button() == Qt::RightButton )
	{
		if( me->modifiers() & Qt::ControlModifier )
		{
			m_tco->toggleMute();
		}
		else if( me->modifiers() & Qt::ShiftModifier && !fixedTCOs() )
		{
			remove();
		}
	}
	else if( me->button() == Qt::MidButton )
	{
		if( me->modifiers() & Qt::ControlModifier )
		{
			m_tco->toggleMute();
		}
		else if( !fixedTCOs() )
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
 * \param me The QMouseEvent to handle.
 * \todo what does the final else case do here?
 */
void TrackContentObjectView::mouseMoveEvent( QMouseEvent * me )
{
	if( m_action == CopySelection || m_action == ToggleSelected )
	{
		if( mouseMovedDistance( me, 2 ) == true )
		{
			QVector<TrackContentObjectView *> tcoViews;
			if( m_action == CopySelection )
			{
				// Collect all selected TCOs
				QVector<selectableObject *> so =
					m_trackView->trackContainerView()->selectedObjects();
				for( auto it = so.begin(); it != so.end(); ++it )
				{
					TrackContentObjectView * tcov =
						dynamic_cast<TrackContentObjectView *>( *it );
					if( tcov != NULL )
					{
						tcoViews.push_back( tcov );
					}
				}
			}
			else
			{
				gui->songEditor()->m_editor->selectAllTcos( false );
				tcoViews.push_back( this );
			}
			// Clear the action here because mouseReleaseEvent will not get
			// triggered once we go into drag.
			m_action = NoAction;

			// Write the TCOs to the DataFile for copying
			DataFile dataFile = createTCODataFiles( tcoViews );

			// TODO -- thumbnail for all selected
			QPixmap thumbnail = grab().scaled(
				128, 128,
				Qt::KeepAspectRatio,
				Qt::SmoothTransformation );
			new StringPairDrag( QString( "tco_%1" ).arg(
								m_tco->getTrack()->type() ),
								dataFile.toString(), thumbnail, this );
		}
	}

	if( me->modifiers() & Qt::ControlModifier )
	{
		delete m_hint;
		m_hint = NULL;
	}

	const float ppb = m_trackView->trackContainerView()->pixelsPerBar();
	if( m_action == Move )
	{
		MidiTime newPos = draggedTCOPos( me );

		// Don't go left of bar zero
		newPos = max( 0, newPos.getTicks() );
		m_tco->movePosition( newPos );
		m_trackView->getTrackContentWidget()->changePosition();
		s_textFloat->setText( QString( "%1:%2" ).
				arg( newPos.getBar() + 1 ).
				arg( newPos.getTicks() %
						MidiTime::ticksPerBar() ) );
		s_textFloat->moveGlobal( this, QPoint( width() + 2, height() + 2 ) );
	}
	else if( m_action == MoveSelection )
	{
		// 1: Find the position we want to move the grabbed TCO to
		MidiTime newPos = draggedTCOPos( me );

		// 2: Handle moving the other selected TCOs the same distance
		QVector<selectableObject *> so =
			m_trackView->trackContainerView()->selectedObjects();
		QVector<TrackContentObject *> tcos; // List of selected clips
		int leftmost = 0; // Leftmost clip's offset from grabbed clip
		// Populate tcos, find leftmost
		for( QVector<selectableObject *>::iterator it = so.begin();
							it != so.end(); ++it )
		{
			TrackContentObjectView * tcov =
				dynamic_cast<TrackContentObjectView *>( *it );
			if( tcov == NULL ) { continue; }
			tcos.push_back( tcov->m_tco );
			int index = std::distance( so.begin(), it );
			leftmost = min (leftmost, m_initialOffsets[index].getTicks() );
		}
		// Make sure the leftmost clip doesn't get moved to a negative position
		if ( newPos.getTicks() + leftmost < 0 ) { newPos = -leftmost; }

		for( QVector<TrackContentObject *>::iterator it = tcos.begin();
							it != tcos.end(); ++it )
		{
			int index = std::distance( tcos.begin(), it );
			( *it )->movePosition( newPos + m_initialOffsets[index] );
		}
	}
	else if( m_action == Resize || m_action == ResizeLeft )
	{
		// If the user is holding alt, or pressed ctrl after beginning the drag, don't quantize
		const bool unquantized = (me->modifiers() & Qt::ControlModifier) || (me->modifiers() & Qt::AltModifier);
		const float snapSize = gui->songEditor()->m_editor->getSnapSize();
		// Length in ticks of one snap increment
		const MidiTime snapLength = MidiTime( (int)(snapSize * MidiTime::ticksPerBar()) );

		if( m_action == Resize )
		{
			// The clip's new length
			MidiTime l = static_cast<int>( me->x() * MidiTime::ticksPerBar() / ppb );

			if ( unquantized )
			{	// We want to preserve this adjusted offset,
				// even if the user switches to snapping later
				setInitialPos( m_initialMousePos );
				// Don't resize to less than 1 tick
				m_tco->changeLength( qMax<int>( 1, l ) );
			}
			else if ( me->modifiers() & Qt::ShiftModifier )
			{	// If shift is held, quantize clip's end position
				MidiTime end = MidiTime( m_initialTCOPos + l ).quantize( snapSize );
				// The end position has to be after the clip's start
				MidiTime min = m_initialTCOPos.quantize( snapSize );
				if ( min <= m_initialTCOPos ) min += snapLength;
				m_tco->changeLength( qMax<int>(min - m_initialTCOPos, end - m_initialTCOPos) );
			}
			else
			{	// Otherwise, resize in fixed increments
				MidiTime initialLength = m_initialTCOEnd - m_initialTCOPos;
				MidiTime offset = MidiTime( l - initialLength ).quantize( snapSize );
				// Don't resize to less than 1 tick
				MidiTime min = MidiTime( initialLength % snapLength );
				if (min < 1) min += snapLength;
				m_tco->changeLength( qMax<int>( min, initialLength + offset) );
			}
		}
		else
		{
			SampleTCO * sTco = dynamic_cast<SampleTCO*>( m_tco );
			if( sTco )
			{
				const int x = mapToParent( me->pos() ).x() - m_initialMousePos.x();

				MidiTime t = qMax( 0, (int)
									m_trackView->trackContainerView()->currentPosition() +
									static_cast<int>( x * MidiTime::ticksPerBar() / ppb ) );

				if( unquantized )
				{	// We want to preserve this adjusted offset,
					// even if the user switches to snapping later
					setInitialPos( m_initialMousePos );
					//Don't resize to less than 1 tick
					t = qMin<int>( m_initialTCOEnd - 1, t);
				}
				else if( me->modifiers() & Qt::ShiftModifier )
				{	// If shift is held, quantize clip's start position
					// Don't let the start position move past the end position
					MidiTime max = m_initialTCOEnd.quantize( snapSize );
					if ( max >= m_initialTCOEnd ) max -= snapLength;
					t = qMin<int>( max, t.quantize( snapSize ) );
				}
				else
				{	// Otherwise, resize in fixed increments
					// Don't resize to less than 1 tick
					MidiTime initialLength = m_initialTCOEnd - m_initialTCOPos;
					MidiTime minLength = MidiTime( initialLength % snapLength );
					if (minLength < 1) minLength += snapLength;
					MidiTime offset = MidiTime(t - m_initialTCOPos).quantize( snapSize );
					t = qMin<int>( m_initialTCOEnd - minLength, m_initialTCOPos + offset );
				}

				MidiTime oldPos = m_tco->startPosition();
				if( m_tco->length() + ( oldPos - t ) >= 1 )
				{
					m_tco->movePosition( t );
					m_trackView->getTrackContentWidget()->changePosition();
					m_tco->changeLength( m_tco->length() + ( oldPos - t ) );
					sTco->setStartTimeOffset( sTco->startTimeOffset() + ( oldPos - t ) );
				}
			}
		}
		s_textFloat->setText( tr( "%1:%2 (%3:%4 to %5:%6)" ).
				arg( m_tco->length().getBar() ).
				arg( m_tco->length().getTicks() %
						MidiTime::ticksPerBar() ).
				arg( m_tco->startPosition().getBar() + 1 ).
				arg( m_tco->startPosition().getTicks() %
						MidiTime::ticksPerBar() ).
				arg( m_tco->endPosition().getBar() + 1 ).
				arg( m_tco->endPosition().getTicks() %
						MidiTime::ticksPerBar() ) );
		s_textFloat->moveGlobal( this, QPoint( width() + 2, height() + 2) );
	}
	else
	{
		SampleTCO * sTco = dynamic_cast<SampleTCO*>( m_tco );
		if( ( me->x() > width() - RESIZE_GRIP_WIDTH && !me->buttons() && !m_tco->getAutoResize() )
		||  ( me->x() < RESIZE_GRIP_WIDTH && !me->buttons() && sTco && !m_tco->getAutoResize() ) )
		{
			setCursor( Qt::SizeHorCursor );
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
 * \param me The QMouseEvent to handle.
 */
void TrackContentObjectView::mouseReleaseEvent( QMouseEvent * me )
{
	// If the CopySelection was chosen as the action due to mouse movement,
	// it will have been cleared.  At this point Toggle is the desired action.
	// An active StringPairDrag will prevent this method from being called,
	// so a real CopySelection would not have occurred.
	if( m_action == CopySelection ||
	    ( m_action == ToggleSelected && mouseMovedDistance( me, 2 ) == false ) )
	{
		setSelected( !isSelected() );
	}

	if( m_action == Move || m_action == Resize || m_action == ResizeLeft )
	{
		// TODO: Fix m_tco->setJournalling() consistency
		m_tco->setJournalling( true );
	}
	m_action = NoAction;
	delete m_hint;
	m_hint = NULL;
	s_textFloat->hide();
	leaveEvent( NULL );
	selectableObject::mouseReleaseEvent( me );
}




/*! \brief Set up the context menu for this trackContentObjectView.
 *
 *  Set up the various context menu events that can apply to a
 *  track content object view.
 *
 * \param cme The QContextMenuEvent to add the actions to.
 */
void TrackContentObjectView::contextMenuEvent( QContextMenuEvent * cme )
{
	if( cme->modifiers() )
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
				tr( "Mute/unmute (<%1> + middle click)" ).arg(UI_CTRL_KEY),
						m_tco, SLOT( toggleMute() ) );
	constructContextMenu( &contextMenu );

	contextMenu.exec( QCursor::pos() );
}




/*! \brief How many pixels a bar takes for this trackContentObjectView.
 *
 * \return the number of pixels per bar.
 */
float TrackContentObjectView::pixelsPerBar()
{
	return m_trackView->trackContainerView()->pixelsPerBar();
}


/*! \brief Save the offsets between all selected tracks and a clicked track */
void TrackContentObjectView::setInitialOffsets()
{
	QVector<selectableObject *> so = m_trackView->trackContainerView()->selectedObjects();
	QVector<MidiTime> offsets;
	for( QVector<selectableObject *>::iterator it = so.begin();
						it != so.end(); ++it )
	{
		TrackContentObjectView * tcov =
			dynamic_cast<TrackContentObjectView *>( *it );
		if( tcov == NULL )
		{
			continue;
		}
		offsets.push_back( tcov->m_tco->startPosition() - m_initialTCOPos );
	}

	m_initialOffsets = offsets;
}




/*! \brief Detect whether the mouse moved more than n pixels on screen.
 *
 * \param _me The QMouseEvent.
 * \param distance The threshold distance that the mouse has moved to return true.
 */
bool TrackContentObjectView::mouseMovedDistance( QMouseEvent * me, int distance )
{
	QPoint dPos = mapToGlobal( me->pos() ) - m_initialMouseGlobalPos;
	const int pixelsMoved = dPos.manhattanLength();
	return ( pixelsMoved > distance || pixelsMoved < -distance );
}



/*! \brief Calculate the new position of a dragged TCO from a mouse event
 *
 *
 * \param me The QMouseEvent
 */
MidiTime TrackContentObjectView::draggedTCOPos( QMouseEvent * me )
{
	//Pixels per bar
	const float ppb = m_trackView->trackContainerView()->pixelsPerBar();
	// The pixel distance that the mouse has moved
	const int mouseOff = mapToGlobal(me->pos()).x() - m_initialMouseGlobalPos.x();
	MidiTime newPos = m_initialTCOPos + mouseOff * MidiTime::ticksPerBar() / ppb;
	MidiTime offset = newPos - m_initialTCOPos;
	// If the user is holding alt, or pressed ctrl after beginning the drag, don't quantize
	if (    me->button() != Qt::NoButton
		|| (me->modifiers() & Qt::ControlModifier)
		|| (me->modifiers() & Qt::AltModifier)    )
	{
		// We want to preserve this adjusted offset,
		// even if the user switches to snapping
		setInitialPos( m_initialMousePos );
	}
	else if ( me->modifiers() & Qt::ShiftModifier )
	{	// If shift is held, quantize position (Default in 1.2.0 and earlier)
		// or end position, whichever is closest to the actual position
		MidiTime startQ = newPos.quantize( gui->songEditor()->m_editor->getSnapSize() );
		// Find start position that gives snapped clip end position
		MidiTime endQ = ( newPos + m_tco->length() );
		endQ = endQ.quantize( gui->songEditor()->m_editor->getSnapSize() );
		endQ = endQ - m_tco->length();
		// Select the position closest to actual position
		if ( abs(newPos - startQ) < abs(newPos - endQ) ) newPos = startQ;
		else newPos = endQ;
	}
	else
	{	// Otherwise, quantize moved distance (preserves user offsets)
		newPos = m_initialTCOPos + offset.quantize( gui->songEditor()->m_editor->getSnapSize() );
	}
	return newPos;
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
 * \param parent The parent track.
 */
TrackContentWidget::TrackContentWidget( TrackView * parent ) :
	QWidget( parent ),
	m_trackView( parent ),
	m_darkerColor( Qt::SolidPattern ),
	m_lighterColor( Qt::SolidPattern ),
	m_gridColor( Qt::SolidPattern ),
	m_embossColor( Qt::SolidPattern )
{
	setAcceptDrops( true );

	connect( parent->trackContainerView(),
			SIGNAL( positionChanged( const MidiTime & ) ),
			this, SLOT( changePosition( const MidiTime & ) ) );

	setStyle( QApplication::style() );

	updateBackground();
}




/*! \brief Destroy this trackContentWidget
 *
 *  Destroys the trackContentWidget.
 */
TrackContentWidget::~TrackContentWidget()
{
}




void TrackContentWidget::updateBackground()
{
	const TrackContainerView * tcv = m_trackView->trackContainerView();

	// Assume even-pixels-per-bar. Makes sense, should be like this anyways
	int ppb = static_cast<int>( tcv->pixelsPerBar() );

	int w = ppb * BARS_PER_GROUP;
	int h = height();
	m_background = QPixmap( w * 2, height() );
	QPainter pmp( &m_background );

	pmp.fillRect( 0, 0, w, h, darkerColor() );
	pmp.fillRect( w, 0, w , h, lighterColor() );

	// draw lines
	// vertical lines
	pmp.setPen( QPen( gridColor(), 1 ) );
	for( float x = 0; x < w * 2; x += ppb )
	{
		pmp.drawLine( QLineF( x, 0.0, x, h ) );
	}

	pmp.setPen( QPen( embossColor(), 1 ) );
	for( float x = 1.0; x < w * 2; x += ppb )
	{
		pmp.drawLine( QLineF( x, 0.0, x, h ) );
	}

	// horizontal line
	pmp.setPen( QPen( gridColor(), 1 ) );
	pmp.drawLine( 0, h-1, w*2, h-1 );

	pmp.end();

	// Force redraw
	update();
}




/*! \brief Adds a trackContentObjectView to this widget.
 *
 *  Adds a(nother) trackContentObjectView to our list of views.  We also
 *  check that our position is up-to-date.
 *
 * \param tcov The trackContentObjectView to add.
 */
void TrackContentWidget::addTCOView( TrackContentObjectView * tcov )
{
	TrackContentObject * tco = tcov->getTrackContentObject();

	m_tcoViews.push_back( tcov );

	tco->saveJournallingState( false );
	changePosition();
	tco->restoreJournallingState();
}




/*! \brief Removes the given trackContentObjectView to this widget.
 *
 *  Removes the given trackContentObjectView from our list of views.
 *
 * \param tcov The trackContentObjectView to add.
 */
void TrackContentWidget::removeTCOView( TrackContentObjectView * tcov )
{
	tcoViewVector::iterator it = std::find( m_tcoViews.begin(),
						m_tcoViews.end(),
						tcov );
	if( it != m_tcoViews.end() )
	{
		m_tcoViews.erase( it );
		Engine::getSong()->setModified();
	}
}




/*! \brief Update ourselves by updating all the tCOViews attached.
 *
 */
void TrackContentWidget::update()
{
	for( tcoViewVector::iterator it = m_tcoViews.begin();
				it != m_tcoViews.end(); ++it )
	{
		( *it )->setFixedHeight( height() - 1 );
		( *it )->update();
	}
	QWidget::update();
}




// resposible for moving track-content-widgets to appropriate position after
// change of visible viewport
/*! \brief Move the trackContentWidget to a new place in time
 *
 * \param newPos The MIDI time to move to.
 */
void TrackContentWidget::changePosition( const MidiTime & newPos )
{
	if( m_trackView->trackContainerView() == gui->getBBEditor()->trackContainerView() )
	{
		const int curBB = Engine::getBBTrackContainer()->currentBB();
		setUpdatesEnabled( false );

		// first show TCO for current BB...
		for( tcoViewVector::iterator it = m_tcoViews.begin();
						it != m_tcoViews.end(); ++it )
		{
		if( ( *it )->getTrackContentObject()->
						startPosition().getBar() == curBB )
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
						startPosition().getBar() != curBB )
			{
				( *it )->hide();
			}
		}
		setUpdatesEnabled( true );
		return;
	}

	MidiTime pos = newPos;
	if( pos < 0 )
	{
		pos = m_trackView->trackContainerView()->currentPosition();
	}

	const int begin = pos;
	const int end = endPosition( pos );
	const float ppb = m_trackView->trackContainerView()->pixelsPerBar();

	setUpdatesEnabled( false );
	for( tcoViewVector::iterator it = m_tcoViews.begin();
						it != m_tcoViews.end(); ++it )
	{
		TrackContentObjectView * tcov = *it;
		TrackContentObject * tco = tcov->getTrackContentObject();

		tco->changeLength( tco->length() );

		const int ts = tco->startPosition();
		const int te = tco->endPosition()-3;
		if( ( ts >= begin && ts <= end ) ||
			( te >= begin && te <= end ) ||
			( ts <= begin && te >= end ) )
		{
			tcov->move( static_cast<int>( ( ts - begin ) * ppb /
						MidiTime::ticksPerBar() ),
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




/*! \brief Return the position of the trackContentWidget in bars.
 *
 * \param mouseX the mouse's current X position in pixels.
 */
MidiTime TrackContentWidget::getPosition( int mouseX )
{
	TrackContainerView * tv = m_trackView->trackContainerView();
	return MidiTime( tv->currentPosition() +
					 mouseX *
					 MidiTime::ticksPerBar() /
					 static_cast<int>( tv->pixelsPerBar() ) );
}




/*! \brief Respond to a drag enter event on the trackContentWidget
 *
 * \param dee the Drag Enter Event to respond to
 */
void TrackContentWidget::dragEnterEvent( QDragEnterEvent * dee )
{
	MidiTime tcoPos = getPosition( dee->pos().x() );
  if( canPasteSelection( tcoPos, dee ) == false )
	{
		dee->ignore();
	}
	else
	{
		StringPairDrag::processDragEnterEvent( dee, "tco_" +
						QString::number( getTrack()->type() ) );
	}
}




/*! \brief Returns whether a selection of TCOs can be pasted into this
 *
 * \param tcoPos the position of the TCO slot being pasted on
 * \param de the DropEvent generated
 */
bool TrackContentWidget::canPasteSelection( MidiTime tcoPos, const QDropEvent* de )
{
	const QMimeData * mimeData = de->mimeData();

	Track * t = getTrack();
	QString type = StringPairDrag::decodeMimeKey( mimeData );
	QString value = StringPairDrag::decodeMimeValue( mimeData );

	// We can only paste into tracks of the same type
	if( type != ( "tco_" + QString::number( t->type() ) ) ||
		m_trackView->trackContainerView()->fixedTCOs() == true )
	{
		return false;
	}

	// value contains XML needed to reconstruct TCOs and place them
	DataFile dataFile( value.toUtf8() );

	// Extract the metadata and which TCO was grabbed
	QDomElement metadata = dataFile.content().firstChildElement( "copyMetadata" );
	QDomAttr tcoPosAttr = metadata.attributeNode( "grabbedTCOPos" );
	MidiTime grabbedTCOPos = tcoPosAttr.value().toInt();
	MidiTime grabbedTCOBar = MidiTime( grabbedTCOPos.getBar(), 0 );

	// Extract the track index that was originally clicked
	QDomAttr tiAttr = metadata.attributeNode( "initialTrackIndex" );
	const int initialTrackIndex = tiAttr.value().toInt();

	// Get the current track's index
	const TrackContainer::TrackList tracks = t->trackContainer()->tracks();
	const int currentTrackIndex = tracks.indexOf( t );

	// Don't paste if we're on the same bar
	auto sourceTrackContainerId = metadata.attributeNode( "trackContainerId" ).value().toUInt();
	if( de->source() && sourceTrackContainerId == t->trackContainer()->id() &&
			tcoPos == grabbedTCOBar && currentTrackIndex == initialTrackIndex )
	{
		return false;
	}

	// Extract the tco data
	QDomElement tcoParent = dataFile.content().firstChildElement( "tcos" );
	QDomNodeList tcoNodes = tcoParent.childNodes();

	// Determine if all the TCOs will land on a valid track
	for( int i = 0; i < tcoNodes.length(); i++ )
	{
		QDomElement tcoElement = tcoNodes.item( i ).toElement();
		int trackIndex = tcoElement.attributeNode( "trackIndex" ).value().toInt();
		int finalTrackIndex = trackIndex + currentTrackIndex - initialTrackIndex;

		// Track must be in TrackContainer's tracks
		if( finalTrackIndex < 0 || finalTrackIndex >= tracks.size() )
		{
			return false;
		}

		// Track must be of the same type
		auto startTrackType = tcoElement.attributeNode("trackType").value().toInt();
		Track * endTrack = tracks.at( finalTrackIndex );
		if( startTrackType != endTrack->type() )
		{
			return false;
		}
	}

	return true;
}

/*! \brief Pastes a selection of TCOs onto the track
 *
 * \param tcoPos the position of the TCO slot being pasted on
 * \param de the DropEvent generated
 */
bool TrackContentWidget::pasteSelection( MidiTime tcoPos, QDropEvent * de )
{
	if( canPasteSelection( tcoPos, de ) == false )
	{
		return false;
	}

	QString type = StringPairDrag::decodeKey( de );
	QString value = StringPairDrag::decodeValue( de );

	getTrack()->addJournalCheckPoint();

	// value contains XML needed to reconstruct TCOs and place them
	DataFile dataFile( value.toUtf8() );

	// Extract the tco data
	QDomElement tcoParent = dataFile.content().firstChildElement( "tcos" );
	QDomNodeList tcoNodes = tcoParent.childNodes();

	// Extract the track index that was originally clicked
	QDomElement metadata = dataFile.content().firstChildElement( "copyMetadata" );
	QDomAttr tiAttr = metadata.attributeNode( "initialTrackIndex" );
	int initialTrackIndex = tiAttr.value().toInt();
	QDomAttr tcoPosAttr = metadata.attributeNode( "grabbedTCOPos" );
	MidiTime grabbedTCOPos = tcoPosAttr.value().toInt();

	// Snap the mouse position to the beginning of the dropped bar, in ticks
	const TrackContainer::TrackList tracks = getTrack()->trackContainer()->tracks();
	const int currentTrackIndex = tracks.indexOf( getTrack() );

	bool wasSelection = m_trackView->trackContainerView()->rubberBand()->selectedObjects().count();

	// Unselect the old group
		const QVector<selectableObject *> so =
			m_trackView->trackContainerView()->selectedObjects();
		for( QVector<selectableObject *>::const_iterator it = so.begin();
		    	it != so.end(); ++it )
		{
			( *it )->setSelected( false );
		}


	// TODO -- Need to draw the hovericon either way, or ghost the TCOs
	// onto their final position.

	// All patterns should be offset the same amount as the grabbed pattern
	// The offset is quantized (rather than the positions) to preserve fine adjustments
	int offset = MidiTime(tcoPos - grabbedTCOPos).quantize(gui->songEditor()->m_editor->getSnapSize());

	for( int i = 0; i<tcoNodes.length(); i++ )
	{
		QDomElement outerTCOElement = tcoNodes.item( i ).toElement();
		QDomElement tcoElement = outerTCOElement.firstChildElement();

		int trackIndex = outerTCOElement.attributeNode( "trackIndex" ).value().toInt();
		int finalTrackIndex = trackIndex + ( currentTrackIndex - initialTrackIndex );
		Track * t = tracks.at( finalTrackIndex );

		// The new position is the old position plus the offset.
		MidiTime pos = tcoElement.attributeNode( "pos" ).value().toInt() + offset;
		// If we land on ourselves, offset by one snap
		MidiTime shift = MidiTime::ticksPerBar() * gui->songEditor()->m_editor->getSnapSize();
		if (offset == 0) { pos += shift; }

		TrackContentObject * tco = t->createTCO( pos );
		tco->restoreState( tcoElement );
		tco->movePosition( pos );
		if( wasSelection )
		{
			tco->selectViewOnCreate( true );
		}

		//check tco name, if the same as source track name dont copy
		QString sourceTrackName = outerTCOElement.attributeNode( "trackName" ).value();
		if( tco->name() == sourceTrackName )
		{
			tco->setName( "" );
		}
	}

	AutomationPattern::resolveAllIDs();

	return true;
}


/*! \brief Respond to a drop event on the trackContentWidget
 *
 * \param de the Drop Event to respond to
 */
void TrackContentWidget::dropEvent( QDropEvent * de )
{
	MidiTime tcoPos = MidiTime( getPosition( de->pos().x() ) );
	if( pasteSelection( tcoPos, de ) == true )
	{
		de->accept();
	}
}




/*! \brief Respond to a mouse press on the trackContentWidget
 *
 * \param me the mouse press event to respond to
 */
void TrackContentWidget::mousePressEvent( QMouseEvent * me )
{
	if( m_trackView->trackContainerView()->allowRubberband() == true )
	{
		QWidget::mousePressEvent( me );
	}
	else if( me->modifiers() & Qt::ShiftModifier )
	{
		QWidget::mousePressEvent( me );
	}
	else if( me->button() == Qt::LeftButton &&
			!m_trackView->trackContainerView()->fixedTCOs() )
	{
		QVector<selectableObject*> so =  m_trackView->trackContainerView()->rubberBand()->selectedObjects();
		for( int i = 0; i < so.count(); ++i )
		{
			so.at( i )->setSelected( false);
		}
		getTrack()->addJournalCheckPoint();
		const MidiTime pos = getPosition( me->x() ).getBar() *
						MidiTime::ticksPerBar();
		TrackContentObject * tco = getTrack()->createTCO( pos );

		tco->saveJournallingState( false );
		tco->movePosition( pos );
		tco->restoreJournallingState();
	}
}




/*! \brief Repaint the trackContentWidget on command
 *
 * \param pe the Paint Event to respond to
 */
void TrackContentWidget::paintEvent( QPaintEvent * pe )
{
	// Assume even-pixels-per-bar. Makes sense, should be like this anyways
	const TrackContainerView * tcv = m_trackView->trackContainerView();
	int ppb = static_cast<int>( tcv->pixelsPerBar() );
	QPainter p( this );
	// Don't draw background on BB-Editor
	if( m_trackView->trackContainerView() != gui->getBBEditor()->trackContainerView() )
	{
		p.drawTiledPixmap( rect(), m_background, QPoint(
				tcv->currentPosition().getBar() * ppb, 0 ) );
	}
}




/*! \brief Updates the background tile pixmap on size changes.
 *
 * \param resizeEvent the resize event to pass to base class
 */
void TrackContentWidget::resizeEvent( QResizeEvent * resizeEvent )
{
	// Update backgroud
	updateBackground();
	// Force redraw
	QWidget::resizeEvent( resizeEvent );
}




/*! \brief Return the track shown by the trackContentWidget
 *
 */
Track * TrackContentWidget::getTrack()
{
	return m_trackView->getTrack();
}




/*! \brief Return the end position of the trackContentWidget in Bars.
 *
 * \param posStart the starting position of the Widget (from getPosition())
 */
MidiTime TrackContentWidget::endPosition( const MidiTime & posStart )
{
	const float ppb = m_trackView->trackContainerView()->pixelsPerBar();
	const int w = width();
	return posStart + static_cast<int>( w * MidiTime::ticksPerBar() / ppb );
}




// qproperty access methods
//! \brief CSS theming qproperty access method
QBrush TrackContentWidget::darkerColor() const
{ return m_darkerColor; }

//! \brief CSS theming qproperty access method
QBrush TrackContentWidget::lighterColor() const
{ return m_lighterColor; }

//! \brief CSS theming qproperty access method
QBrush TrackContentWidget::gridColor() const
{ return m_gridColor; }

//! \brief CSS theming qproperty access method
QBrush TrackContentWidget::embossColor() const
{ return m_embossColor; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setDarkerColor( const QBrush & c )
{ m_darkerColor = c; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setLighterColor( const QBrush & c )
{ m_lighterColor = c; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setGridColor( const QBrush & c )
{ m_gridColor = c; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setEmbossColor( const QBrush & c )
{ m_embossColor = c; }


// ===========================================================================
// trackOperationsWidget
// ===========================================================================


QPixmap * TrackOperationsWidget::s_grip = NULL;     /*!< grip pixmap */


/*! \brief Create a new trackOperationsWidget
 *
 * The trackOperationsWidget is the grip and the mute button of a track.
 *
 * \param parent the trackView to contain this widget
 */
TrackOperationsWidget::TrackOperationsWidget( TrackView * parent ) :
	QWidget( parent ),             /*!< The parent widget */
	m_trackView( parent )          /*!< The parent track view */
{
	ToolTip::add( this, tr( "Press <%1> while clicking on move-grip "
				"to begin a new drag'n'drop action." ).arg(UI_CTRL_KEY) );

	QMenu * toMenu = new QMenu( this );
	toMenu->setFont( pointSize<9>( toMenu->font() ) );
	connect( toMenu, SIGNAL( aboutToShow() ), this, SLOT( updateMenu() ) );


	setObjectName( "automationEnabled" );


	m_trackOps = new QPushButton( this );
	m_trackOps->move( 12, 1 );
	m_trackOps->setFocusPolicy( Qt::NoFocus );
	m_trackOps->setMenu( toMenu );
	ToolTip::add( m_trackOps, tr( "Actions" ) );


	m_muteBtn = new PixmapButton( this, tr( "Mute" ) );
	m_muteBtn->setActiveGraphic( embed::getIconPixmap( "led_off" ) );
	m_muteBtn->setInactiveGraphic( embed::getIconPixmap( "led_green" ) );
	m_muteBtn->setCheckable( true );

	m_soloBtn = new PixmapButton( this, tr( "Solo" ) );
	m_soloBtn->setActiveGraphic( embed::getIconPixmap( "led_red" ) );
	m_soloBtn->setInactiveGraphic( embed::getIconPixmap( "led_off" ) );
	m_soloBtn->setCheckable( true );

	if( ConfigManager::inst()->value( "ui",
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
	ToolTip::add( m_muteBtn, tr( "Mute" ) );

	m_soloBtn->show();
	ToolTip::add( m_soloBtn, tr( "Solo" ) );

	connect( this, SIGNAL( trackRemovalScheduled( TrackView * ) ),
			m_trackView->trackContainerView(),
				SLOT( deleteTrackView( TrackView * ) ),
							Qt::QueuedConnection );
}




/*! \brief Destroy an existing trackOperationsWidget
 *
 */
TrackOperationsWidget::~TrackOperationsWidget()
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
 *  \param me The mouse event to respond to.
 */
void TrackOperationsWidget::mousePressEvent( QMouseEvent * me )
{
	if( me->button() == Qt::LeftButton &&
		me->modifiers() & Qt::ControlModifier &&
			m_trackView->getTrack()->type() != Track::BBTrack )
	{
		DataFile dataFile( DataFile::DragNDropData );
		m_trackView->getTrack()->saveState( dataFile, dataFile.content() );
		new StringPairDrag( QString( "track_%1" ).arg(
					m_trackView->getTrack()->type() ),
			dataFile.toString(), m_trackView->getTrackSettingsWidget()->grab(),
									this );
	}
	else if( me->button() == Qt::LeftButton )
	{
		// track-widget (parent-widget) initiates track-move
		me->ignore();
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
 *  \param pe The paint event to respond to
 */
void TrackOperationsWidget::paintEvent( QPaintEvent * pe )
{
	QPainter p( this );
	p.fillRect( rect(), palette().brush(QPalette::Background) );

	if( m_trackView->isMovingTrack() == false )
	{
		s_grip = new QPixmap( embed::getIconPixmap(
							"track_op_grip" ) );

		p.drawPixmap( 2, 2, *s_grip );
	}
	else
	{
		s_grip = new QPixmap( embed::getIconPixmap(
							"track_op_grip_c" ) );

		p.drawPixmap( 2, 2, *s_grip );
	}
}




/*! \brief Clone this track
 *
 */
void TrackOperationsWidget::cloneTrack()
{
	TrackContainerView *tcView = m_trackView->trackContainerView();

	Track *newTrack = m_trackView->getTrack()->clone();
	TrackView *newTrackView = tcView->createTrackView( newTrack );

	int index = tcView->trackViews().indexOf( m_trackView );
	int i = tcView->trackViews().size();
	while ( i != index + 1 )
	{
		tcView->moveTrackView( newTrackView, i - 1 );
		i--;
	}
}


/*! \brief Clear this track - clears all TCOs from the track */
void TrackOperationsWidget::clearTrack()
{
	Track * t = m_trackView->getTrack();
	t->addJournalCheckPoint();
	t->lock();
	t->deleteTCOs();
	t->unlock();
}



/*! \brief Remove this track from the track list
 *
 */
void TrackOperationsWidget::removeTrack()
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
void TrackOperationsWidget::updateMenu()
{
	QMenu * toMenu = m_trackOps->menu();
	toMenu->clear();
	toMenu->addAction( embed::getIconPixmap( "edit_copy", 16, 16 ),
						tr( "Clone this track" ),
						this, SLOT( cloneTrack() ) );
	toMenu->addAction( embed::getIconPixmap( "cancel", 16, 16 ),
						tr( "Remove this track" ),
						this, SLOT( removeTrack() ) );

	if( ! m_trackView->trackContainerView()->fixedTCOs() )
	{
		toMenu->addAction( tr( "Clear this track" ), this, SLOT( clearTrack() ) );
	}
	if (QMenu *fxMenu = m_trackView->createFxMenu(tr("FX %1: %2"), tr("Assign to new FX Channel")))
	{
		toMenu->addMenu(fxMenu);
	}

	if (InstrumentTrackView * trackView = dynamic_cast<InstrumentTrackView *>(m_trackView))
	{
		toMenu->addSeparator();
		toMenu->addMenu(trackView->midiMenu());
	}
	if( dynamic_cast<AutomationTrackView *>( m_trackView ) )
	{
		toMenu->addAction( tr( "Turn all recording on" ), this, SLOT( recordingOn() ) );
		toMenu->addAction( tr( "Turn all recording off" ), this, SLOT( recordingOff() ) );
	}
}


void TrackOperationsWidget::toggleRecording( bool on )
{
	AutomationTrackView * atv = dynamic_cast<AutomationTrackView *>( m_trackView );
	if( atv )
	{
		for( TrackContentObject * tco : atv->getTrack()->getTCOs() )
		{
			AutomationPattern * ap = dynamic_cast<AutomationPattern *>( tco );
			if( ap ) { ap->setRecording( on ); }
		}
		atv->update();
	}
}



void TrackOperationsWidget::recordingOn()
{
	toggleRecording( true );
}


void TrackOperationsWidget::recordingOff()
{
	toggleRecording( false );
}


// ===========================================================================
// track
// ===========================================================================

/*! \brief Create a new (empty) track object
 *
 *  The track object is the whole track, linking its contents, its
 *  automation, name, type, and so forth.
 *
 * \param type The type of track (Song Editor or Beat+Bassline Editor)
 * \param tc The track Container object to encapsulate in this track.
 *
 * \todo check the definitions of all the properties - are they OK?
 */
Track::Track( TrackTypes type, TrackContainer * tc ) :
	Model( tc ),                   /*!< The track Model */
	m_trackContainer( tc ),        /*!< The track container object */
	m_type( type ),                /*!< The track type */
	m_name(),                       /*!< The track's name */
	m_mutedModel( false, this, tr( "Mute" ) ),
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
 *  Then delete the TrackContentObject's contents, remove this track from
 *  the track container.
 *
 *  Finally step through this track's automation and forget all of them.
 */
Track::~Track()
{
	lock();
	emit destroyedTrack();

	while( !m_trackContentObjects.isEmpty() )
	{
		delete m_trackContentObjects.last();
	}

	m_trackContainer->removeTrack( this );
	unlock();
}




/*! \brief Create a track based on the given track type and container.
 *
 *  \param tt The type of track to create
 *  \param tc The track container to attach to
 */
Track * Track::create( TrackTypes tt, TrackContainer * tc )
{
	Engine::mixer()->requestChangeInModel();

	Track * t = NULL;

	switch( tt )
	{
		case InstrumentTrack: t = new ::InstrumentTrack( tc ); break;
		case BBTrack: t = new ::BBTrack( tc ); break;
		case SampleTrack: t = new ::SampleTrack( tc ); break;
//		case EVENT_TRACK:
//		case VIDEO_TRACK:
		case AutomationTrack: t = new ::AutomationTrack( tc ); break;
		case HiddenAutomationTrack:
						t = new ::AutomationTrack( tc, true ); break;
		default: break;
	}

	if( tc == Engine::getBBTrackContainer() && t )
	{
		t->createTCOsForBB( Engine::getBBTrackContainer()->numOfBBs()
									- 1 );
	}

	tc->updateAfterTrackAdd();

	Engine::mixer()->doneChangeInModel();

	return t;
}




/*! \brief Create a track inside TrackContainer from track type in a QDomElement and restore state from XML
 *
 *  \param element The QDomElement containing the type of track to create
 *  \param tc The track container to attach to
 */
Track * Track::create( const QDomElement & element, TrackContainer * tc )
{
	Engine::mixer()->requestChangeInModel();

	Track * t = create(
		static_cast<TrackTypes>( element.attribute( "type" ).toInt() ),
									tc );
	if( t != NULL )
	{
		t->restoreState( element );
	}

	Engine::mixer()->doneChangeInModel();

	return t;
}




/*! \brief Clone a track from this track
 *
 */
Track * Track::clone()
{
	QDomDocument doc;
	QDomElement parent = doc.createElement( "clone" );
	saveState( doc, parent );
	return create( parent.firstChild().toElement(), m_trackContainer );
}






/*! \brief Save this track's settings to file
 *
 *  We save the track type and its muted state and solo state, then append the track-
 *  specific settings.  Then we iterate through the trackContentObjects
 *  and save all their states in turn.
 *
 *  \param doc The QDomDocument to use to save
 *  \param element The The QDomElement to save into
 *  \todo Does this accurately describe the parameters?  I think not!?
 *  \todo Save the track height
 */
void Track::saveSettings( QDomDocument & doc, QDomElement & element )
{
	if( !m_simpleSerializingMode )
	{
		element.setTagName( "track" );
	}
	element.setAttribute( "type", type() );
	element.setAttribute( "name", name() );
	m_mutedModel.saveSettings( doc, element, "muted" );
	m_soloModel.saveSettings( doc, element, "solo" );

	if( m_height >= MINIMAL_TRACK_HEIGHT )
	{
		element.setAttribute( "trackheight", m_height );
	}

	QDomElement tsDe = doc.createElement( nodeName() );
	// let actual track (InstrumentTrack, bbTrack, sampleTrack etc.) save
	// its settings
	element.appendChild( tsDe );
	saveTrackSpecificSettings( doc, tsDe );

	if( m_simpleSerializingMode )
	{
		m_simpleSerializingMode = false;
		return;
	}

	// now save settings of all TCO's
	for( tcoVector::const_iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		( *it )->saveState( doc, element );
	}
}




/*! \brief Load the settings from a file
 *
 *  We load the track's type and muted state and solo state, then clear out our
 *  current TrackContentObject.
 *
 *  Then we step through the QDomElement's children and load the
 *  track-specific settings and trackContentObjects states from it
 *  one at a time.
 *
 *  \param element the QDomElement to load track settings from
 *  \todo Load the track height.
 */
void Track::loadSettings( const QDomElement & element )
{
	if( element.attribute( "type" ).toInt() != type() )
	{
		qWarning( "Current track-type does not match track-type of "
							"settings-node!\n" );
	}

	setName( element.hasAttribute( "name" ) ? element.attribute( "name" ) :
			element.firstChild().toElement().attribute( "name" ) );

	m_mutedModel.loadSettings( element, "muted" );
	m_soloModel.loadSettings( element, "solo" );

	if( m_simpleSerializingMode )
	{
		QDomNode node = element.firstChild();
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

	QDomNode node = element.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.nodeName() == nodeName() )
			{
				loadTrackSpecificSettings( node.toElement() );
			}
			else if( node.nodeName() != "muted"
			&& node.nodeName() != "solo"
			&& !node.toElement().attribute( "metadata" ).toInt() )
			{
				TrackContentObject * tco = createTCO(
								MidiTime( 0 ) );
				tco->restoreState( node.toElement() );
				saveJournallingState( false );
				restoreJournallingState();
			}
		}
		node = node.nextSibling();
	}

	int storedHeight = element.attribute( "trackheight" ).toInt();
	if( storedHeight >= MINIMAL_TRACK_HEIGHT )
	{
		m_height = storedHeight;
	}
}




/*! \brief Add another TrackContentObject into this track
 *
 *  \param tco The TrackContentObject to attach to this track.
 */
TrackContentObject * Track::addTCO( TrackContentObject * tco )
{
	m_trackContentObjects.push_back( tco );

	emit trackContentObjectAdded( tco );

	return tco;		// just for convenience
}




/*! \brief Remove a given TrackContentObject from this track
 *
 *  \param tco The TrackContentObject to remove from this track.
 */
void Track::removeTCO( TrackContentObject * tco )
{
	tcoVector::iterator it = std::find( m_trackContentObjects.begin(),
					m_trackContentObjects.end(),
					tco );
	if( it != m_trackContentObjects.end() )
	{
		m_trackContentObjects.erase( it );
		if( Engine::getSong() )
		{
			Engine::getSong()->updateLength();
			Engine::getSong()->setModified();
		}
	}
}


/*! \brief Remove all TCOs from this track */
void Track::deleteTCOs()
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
int Track::numOfTCOs()
{
	return m_trackContentObjects.size();
}




/*! \brief Get a TrackContentObject by number
 *
 *  If the TCO number is less than our TCO array size then fetch that
 *  numbered object from the array.  Otherwise we warn the user that
 *  we've somehow requested a TCO that is too large, and create a new
 *  TCO for them.
 *  \param tcoNum The number of the TrackContentObject to fetch.
 *  \return the given TrackContentObject or a new one if out of range.
 *  \todo reject TCO numbers less than zero.
 *  \todo if we create a TCO here, should we somehow attach it to the
 *     track?
 */
TrackContentObject * Track::getTCO( int tcoNum )
{
	if( tcoNum < m_trackContentObjects.size() )
	{
		return m_trackContentObjects[tcoNum];
	}
	printf( "called Track::getTCO( %d ), "
			"but TCO %d doesn't exist\n", tcoNum, tcoNum );
	return createTCO( tcoNum * MidiTime::ticksPerBar() );

}




/*! \brief Determine the given TrackContentObject's number in our array.
 *
 *  \param tco The TrackContentObject to search for.
 *  \return its number in our array.
 */
int Track::getTCONum( const TrackContentObject * tco )
{
//	for( int i = 0; i < getTrackContentWidget()->numOfTCOs(); ++i )
	tcoVector::iterator it = std::find( m_trackContentObjects.begin(),
					m_trackContentObjects.end(),
					tco );
	if( it != m_trackContentObjects.end() )
	{
/*		if( getTCO( i ) == _tco )
		{
			return i;
		}*/
		return it - m_trackContentObjects.begin();
	}
	qWarning( "Track::getTCONum(...) -> _tco not found!\n" );
	return 0;
}




/*! \brief Retrieve a list of trackContentObjects that fall within a period.
 *
 *  Here we're interested in a range of trackContentObjects that intersect
 *  the given time period.
 *
 *  We return the TCOs we find in order by time, earliest TCOs first.
 *
 *  \param tcoV The list to contain the found trackContentObjects.
 *  \param start The MIDI start time of the range.
 *  \param end   The MIDI endi time of the range.
 */
void Track::getTCOsInRange( tcoVector & tcoV, const MidiTime & start,
							const MidiTime & end )
{
	for( TrackContentObject* tco : m_trackContentObjects )
	{
		int s = tco->startPosition();
		int e = tco->endPosition();
		if( ( s <= end ) && ( e >= start ) )
		{
			// TCO is within given range
			// Insert sorted by TCO's position
			tcoV.insert(std::upper_bound(tcoV.begin(), tcoV.end(), tco, TrackContentObject::comparePosition),
						tco);
		}
	}
}




/*! \brief Swap the position of two trackContentObjects.
 *
 *  First, we arrange to swap the positions of the two TCOs in the
 *  trackContentObjects list.  Then we swap their start times as well.
 *
 *  \param tcoNum1 The first TrackContentObject to swap.
 *  \param tcoNum2 The second TrackContentObject to swap.
 */
void Track::swapPositionOfTCOs( int tcoNum1, int tcoNum2 )
{
	qSwap( m_trackContentObjects[tcoNum1],
					m_trackContentObjects[tcoNum2] );

	const MidiTime pos = m_trackContentObjects[tcoNum1]->startPosition();

	m_trackContentObjects[tcoNum1]->movePosition(
			m_trackContentObjects[tcoNum2]->startPosition() );
	m_trackContentObjects[tcoNum2]->movePosition( pos );
}




void Track::createTCOsForBB( int bb )
{
	while( numOfTCOs() < bb + 1 )
	{
		MidiTime position = MidiTime( numOfTCOs(), 0 );
		TrackContentObject * tco = createTCO( position );
		tco->movePosition( position );
		tco->changeLength( MidiTime( 1, 0 ) );
	}
}




/*! \brief Move all the trackContentObjects after a certain time later by one bar.
 *
 *  \param pos The time at which we want to insert the bar.
 *  \todo if we stepped through this list last to first, and the list was
 *    in ascending order by TCO time, once we hit a TCO that was earlier
 *    than the insert time, we could fall out of the loop early.
 */
void Track::insertBar( const MidiTime & pos )
{
	// we'll increase the position of every TCO, positioned behind pos, by
	// one bar
	for( tcoVector::iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		if( ( *it )->startPosition() >= pos )
		{
			( *it )->movePosition( (*it)->startPosition() +
						MidiTime::ticksPerBar() );
		}
	}
}




/*! \brief Move all the trackContentObjects after a certain time earlier by one bar.
 *
 *  \param pos The time at which we want to remove the bar.
 */
void Track::removeBar( const MidiTime & pos )
{
	// we'll decrease the position of every TCO, positioned behind pos, by
	// one bar
	for( tcoVector::iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		if( ( *it )->startPosition() >= pos )
		{
			( *it )->movePosition( qMax( ( *it )->startPosition() -
						MidiTime::ticksPerBar(), 0 ) );
		}
	}
}




/*! \brief Return the length of the entire track in bars
 *
 *  We step through our list of TCOs and determine their end position,
 *  keeping track of the latest time found in ticks.  Then we return
 *  that in bars by dividing by the number of ticks per bar.
 */
bar_t Track::length() const
{
	// find last end-position
	tick_t last = 0;
	for( tcoVector::const_iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		if( Engine::getSong()->isExporting() &&
				( *it )->isMuted() )
		{
			continue;
		}

		const tick_t cur = ( *it )->endPosition();
		if( cur > last )
		{
			last = cur;
		}
	}

	return last / MidiTime::ticksPerBar();
}



/*! \brief Invert the track's solo state.
 *
 *  We have to go through all the tracks determining if any other track
 *  is already soloed.  Then we have to save the mute state of all tracks,
 *  and set our mute state to on and all the others to off.
 */
void Track::toggleSolo()
{
	const TrackContainer::TrackList & tl = m_trackContainer->tracks();

	bool soloBefore = false;
	for( TrackContainer::TrackList::const_iterator it = tl.begin();
							it != tl.end(); ++it )
	{
		if( *it != this )
		{
			if( ( *it )->m_soloModel.value() )
			{
				soloBefore = true;
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
			if( !soloBefore )
			{
				( *it )->m_mutedBeforeSolo = ( *it )->isMuted();
			}
			( *it )->setMuted( *it == this ? false : true );
			if( *it != this )
			{
				( *it )->m_soloModel.setValue( false );
			}
		}
		else if( !soloBefore )
		{
			( *it )->setMuted( ( *it )->m_mutedBeforeSolo );
		}
	}
}




BoolModel *Track::getMutedModel()
{
	return &m_mutedModel;
}






// ===========================================================================
// trackView
// ===========================================================================

/*! \brief Create a new track View.
 *
 *  The track View is handles the actual display of the track, including
 *  displaying its various widgets and the track segments.
 *
 *  \param track The track to display.
 *  \param tcv The track Container View for us to be displayed in.
 *  \todo Is my description of these properties correct?
 */
TrackView::TrackView( Track * track, TrackContainerView * tcv ) :
	QWidget( tcv->contentWidget() ),   /*!< The Track Container View's content widget. */
	ModelView( NULL, this ),            /*!< The model view of this track */
	m_track( track ),                  /*!< The track we're displaying */
	m_trackContainerView( tcv ),       /*!< The track Container View we're displayed in */
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
		SIGNAL( trackContentObjectAdded( TrackContentObject * ) ),
			this, SLOT( createTCOView( TrackContentObject * ) ),
			Qt::QueuedConnection );

	connect( &m_track->m_mutedModel, SIGNAL( dataChanged() ),
			&m_trackContentWidget, SLOT( update() ) );

	connect( &m_track->m_soloModel, SIGNAL( dataChanged() ),
			m_track, SLOT( toggleSolo() ), Qt::DirectConnection );
	// create views for already existing TCOs
	for( Track::tcoVector::iterator it =
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
TrackView::~TrackView()
{
}




/*! \brief Resize this track View.
 *
 *  \param re the Resize Event to handle.
 */
void TrackView::resizeEvent( QResizeEvent * re )
{
	if( ConfigManager::inst()->value( "ui",
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
void TrackView::update()
{
	m_trackContentWidget.update();
	if( !m_trackContainerView->fixedTCOs() )
	{
		m_trackContentWidget.changePosition();
	}
	QWidget::update();
}




/*! \brief Create a menu for assigning/creating channels for this track.
 *
 */
QMenu * TrackView::createFxMenu(QString title, QString newFxLabel)
{
	Q_UNUSED(title)
	Q_UNUSED(newFxLabel)
	return NULL;
}




/*! \brief Close this track View.
 *
 */
bool TrackView::close()
{
	m_trackContainerView->removeTrackView( this );
	return QWidget::close();
}




/*! \brief Register that the model of this track View has changed.
 *
 */
void TrackView::modelChanged()
{
	m_track = castModel<Track>();
	assert( m_track != NULL );
	connect( m_track, SIGNAL( destroyedTrack() ), this, SLOT( close() ) );
	m_trackOperationsWidget.m_muteBtn->setModel( &m_track->m_mutedModel );
	m_trackOperationsWidget.m_soloBtn->setModel( &m_track->m_soloModel );
	ModelView::modelChanged();
	setFixedHeight( m_track->getHeight() );
}




/*! \brief Start a drag event on this track View.
 *
 *  \param dee the DragEnterEvent to start.
 */
void TrackView::dragEnterEvent( QDragEnterEvent * dee )
{
	StringPairDrag::processDragEnterEvent( dee, "track_" +
					QString::number( m_track->type() ) );
}




/*! \brief Accept a drop event on this track View.
 *
 *  We only accept drop events that are of the same type as this track.
 *  If so, we decode the data from the drop event by just feeding it
 *  back into the engine as a state.
 *
 *  \param de the DropEvent to handle.
 */
void TrackView::dropEvent( QDropEvent * de )
{
	QString type = StringPairDrag::decodeKey( de );
	QString value = StringPairDrag::decodeValue( de );
	if( type == ( "track_" + QString::number( m_track->type() ) ) )
	{
		// value contains our XML-data so simply create a
		// DataFile which does the rest for us...
		DataFile dataFile( value.toUtf8() );
		Engine::mixer()->requestChangeInModel();
		m_track->restoreState( dataFile.content().firstChild().toElement() );
		Engine::mixer()->doneChangeInModel();
		de->accept();
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
 *  \param me the MouseEvent to handle.
 */
void TrackView::mousePressEvent( QMouseEvent * me )
{

	// If previously dragged too small, restore on shift-leftclick
	if( height() < DEFAULT_TRACK_HEIGHT &&
		me->modifiers() & Qt::ShiftModifier &&
		me->button() == Qt::LeftButton )
	{
		setFixedHeight( DEFAULT_TRACK_HEIGHT );
		m_track->setHeight( DEFAULT_TRACK_HEIGHT );
	}


	int widgetTotal = ConfigManager::inst()->value( "ui",
							"compacttrackbuttons" ).toInt()==1 ?
		DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT + TRACK_OP_WIDTH_COMPACT :
		DEFAULT_SETTINGS_WIDGET_WIDTH + TRACK_OP_WIDTH;
	if( m_trackContainerView->allowRubberband() == true  && me->x() > widgetTotal )
	{
		QWidget::mousePressEvent( me );
	}
	else if( me->button() == Qt::LeftButton )
	{
		if( me->modifiers() & Qt::ShiftModifier )
		{
			m_action = ResizeTrack;
			QCursor::setPos( mapToGlobal( QPoint( me->x(),
								height() ) ) );
			QCursor c( Qt::SizeVerCursor);
			QApplication::setOverrideCursor( c );
		}
		else
		{
			if( me->x()>10 ) // 10 = The width of the grip + 2 pixels to the left and right.
			{
				QWidget::mousePressEvent( me );
				return;
			}

			m_action = MoveTrack;

			QCursor c( Qt::SizeVerCursor );
			QApplication::setOverrideCursor( c );
			// update because in move-mode, all elements in
			// track-op-widgets are hidden as a visual feedback
			m_trackOperationsWidget.update();
		}

		me->accept();
	}
	else
	{
		QWidget::mousePressEvent( me );
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
 *  \param me the MouseEvent to handle.
 */
void TrackView::mouseMoveEvent( QMouseEvent * me )
{
	int widgetTotal = ConfigManager::inst()->value( "ui",
							"compacttrackbuttons" ).toInt()==1 ?
		DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT + TRACK_OP_WIDTH_COMPACT :
		DEFAULT_SETTINGS_WIDGET_WIDTH + TRACK_OP_WIDTH;
	if( m_trackContainerView->allowRubberband() == true && me->x() > widgetTotal )
	{
		QWidget::mouseMoveEvent( me );
	}
	else if( m_action == MoveTrack )
	{
		// look which track-widget the mouse-cursor is over
		const int yPos =
			m_trackContainerView->contentWidget()->mapFromGlobal( me->globalPos() ).y();
		const TrackView * trackAtY = m_trackContainerView->trackViewAt( yPos );

		// debug code
		//	qDebug( "y position %d", yPos );

		// a track-widget not equal to ourself?
		if( trackAtY != NULL && trackAtY != this )
		{
			// then move us up/down there!
			if( me->y() < 0 )
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
		setFixedHeight( qMax<int>( me->y(), MINIMAL_TRACK_HEIGHT ) );
		m_trackContainerView->realignTracks();
		m_track->setHeight( height() );
	}

	if( height() < DEFAULT_TRACK_HEIGHT )
	{
		ToolTip::add( this, m_track->m_name );
	}
}



/*! \brief Handle a mouse release event on this track View.
 *
 *  \param me the MouseEvent to handle.
 */
void TrackView::mouseReleaseEvent( QMouseEvent * me )
{
	m_action = NoAction;
	while( QApplication::overrideCursor() != NULL )
	{
		QApplication::restoreOverrideCursor();
	}
	m_trackOperationsWidget.update();

	QWidget::mouseReleaseEvent( me );
}




/*! \brief Repaint this track View.
 *
 *  \param pe the PaintEvent to start.
 */
void TrackView::paintEvent( QPaintEvent * pe )
{
	QStyleOption opt;
	opt.initFrom( this );
	QPainter p( this );
	style()->drawPrimitive( QStyle::PE_Widget, &opt, &p, this );
}




/*! \brief Create a TrackContentObject View in this track View.
 *
 *  \param tco the TrackContentObject to create the view for.
 *  \todo is this a good description for what this method does?
 */
void TrackView::createTCOView( TrackContentObject * tco )
{
	TrackContentObjectView * tv = tco->createView( this );
	if( tco->getSelectViewOnCreate() == true )
	{
		tv->setSelected( true );
	}
	tco->selectViewOnCreate( false );
}
