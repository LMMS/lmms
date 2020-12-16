/*
 * TrackContentObjectView.cpp - implementation of TrackContentObjectView class
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

#include "TrackContentObjectView.h"

#include <set>

#include <QMenu>
#include <QMouseEvent>
#include <QPainter>

#include "AutomationPattern.h"
#include "Clipboard.h"
#include "ColorChooser.h"
#include "ComboBoxModel.h"
#include "DataFile.h"
#include "Engine.h"
#include "embed.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "Note.h"
#include "Pattern.h"
#include "SampleTrack.h"
#include "Song.h"
#include "SongEditor.h"
#include "StringPairDrag.h"
#include "TextFloat.h"
#include "TrackContainer.h"
#include "TrackContainerView.h"
#include "TrackView.h"


/*! The width of the resize grip in pixels
 */
const int RESIZE_GRIP_WIDTH = 4;


/*! A pointer for that text bubble used when moving segments, etc.
 *
 * In a number of situations, LMMS displays a floating text bubble
 * beside the cursor as you move or resize elements of a track about.
 * This pointer keeps track of it, as you only ever need one at a time.
 */
TextFloat * TrackContentObjectView::s_textFloat = NULL;


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
	m_initialTCOPos( TimePos(0) ),
	m_initialTCOEnd( TimePos(0) ),
	m_initialOffsets( QVector<TimePos>() ),
	m_hint( NULL ),
	m_mutedColor( 0, 0, 0 ),
	m_mutedBackgroundColor( 0, 0, 0 ),
	m_selectedColor( 0, 0, 0 ),
	m_textColor( 0, 0, 0 ),
	m_textShadowColor( 0, 0, 0 ),
	m_BBPatternBackground( 0, 0, 0 ),
	m_gradient( true ),
	m_mouseHotspotHand( 0, 0 ),
	m_cursorSetYet( false ),
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
	setCursor( QCursor( embed::getIconPixmap( "hand" ), m_mouseHotspotHand.width(), m_mouseHotspotHand.height() ) );
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
	connect( m_tco, SIGNAL( trackColorChanged() ), this, SLOT( update() ) );
	connect( m_trackView->getTrackOperationsWidget(), SIGNAL( colorParented() ), this, SLOT( useTrackColor() ) );

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
	if( !m_cursorSetYet )
	{
		setCursor( QCursor( embed::getIconPixmap( "hand" ), m_mouseHotspotHand.width(), m_mouseHotspotHand.height() ) );
		m_cursorSetYet = true;
	}

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

void TrackContentObjectView::setMouseHotspotHand(const QSize & s)
{
	m_mouseHotspotHand = s;
}

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
					TimePos::ticksPerBar() ) + 1 /*+
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




void TrackContentObjectView::changeClipColor()
{
	// Get a color from the user
	QColor new_color = ColorChooser( this ).withPalette( ColorChooser::Palette::Track )->getColor( m_tco->color() );
	if( ! new_color.isValid() )
	{ return; }

	// Use that color
	m_tco->setColor( new_color );
	m_tco->useCustomClipColor( true );
	update();
}



void TrackContentObjectView::useTrackColor()
{
	m_tco->useCustomClipColor( false );
	update();
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
	TimePos tcoPos = TimePos( m_tco->startPosition() );

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
		TimePos tcoPos = TimePos( m_tco->startPosition() );

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
	TimePos pos = m_tco->startPosition();
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
		setCursor( QCursor( embed::getIconPixmap( "hand" ), m_mouseHotspotHand.width(), m_mouseHotspotHand.height() ) );
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
	// Right now, active is only used on right/mid clicks actions, so we use a ternary operator
	// to avoid the overhead of calling getClickedTCOs when it's not used
	auto active = me->button() == Qt::LeftButton
		? QVector<TrackContentObjectView *>()
		: getClickedTCOs();

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
				else if( m_tco->getAutoResize() || me->x() < width() - RESIZE_GRIP_WIDTH )
				{
					m_action = Move;
					setCursor( Qt::SizeAllCursor );
				}
				else
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
								TimePos::ticksPerBar() ) );
				}
				else if( m_action == Resize || m_action == ResizeLeft )
				{
					s_textFloat->setTitle( tr( "Current length" ) );
					s_textFloat->setText( tr( "%1:%2 (%3:%4 to %5:%6)" ).
							arg( m_tco->length().getBar() ).
							arg( m_tco->length().getTicks() %
									TimePos::ticksPerBar() ).
							arg( m_tco->startPosition().getBar() + 1 ).
							arg( m_tco->startPosition().getTicks() %
									TimePos::ticksPerBar() ).
							arg( m_tco->endPosition().getBar() + 1 ).
							arg( m_tco->endPosition().getTicks() %
									TimePos::ticksPerBar() ) );
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
			toggleMute( active );
		}
		else if( me->modifiers() & Qt::ShiftModifier && !fixedTCOs() )
		{
			remove( active );
		}
	}
	else if( me->button() == Qt::MidButton )
	{
		if( me->modifiers() & Qt::ControlModifier )
		{
			toggleMute( active );
		}
		else if( !fixedTCOs() )
		{
			remove( active );
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
		TimePos newPos = draggedTCOPos( me );

		m_tco->movePosition(newPos);
		newPos = m_tco->startPosition(); // Get the real position the TCO was dragged to for the label
		m_trackView->getTrackContentWidget()->changePosition();
		s_textFloat->setText( QString( "%1:%2" ).
				arg( newPos.getBar() + 1 ).
				arg( newPos.getTicks() %
						TimePos::ticksPerBar() ) );
		s_textFloat->moveGlobal( this, QPoint( width() + 2, height() + 2 ) );
	}
	else if( m_action == MoveSelection )
	{
		// 1: Find the position we want to move the grabbed TCO to
		TimePos newPos = draggedTCOPos( me );

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
			leftmost = std::min(leftmost, m_initialOffsets[index].getTicks());
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
		const TimePos snapLength = TimePos( (int)(snapSize * TimePos::ticksPerBar()) );

		if( m_action == Resize )
		{
			// The clip's new length
			TimePos l = static_cast<int>( me->x() * TimePos::ticksPerBar() / ppb );

			if ( unquantized )
			{	// We want to preserve this adjusted offset,
				// even if the user switches to snapping later
				setInitialPos( m_initialMousePos );
				// Don't resize to less than 1 tick
				m_tco->changeLength( qMax<int>( 1, l ) );
			}
			else if ( me->modifiers() & Qt::ShiftModifier )
			{	// If shift is held, quantize clip's end position
				TimePos end = TimePos( m_initialTCOPos + l ).quantize( snapSize );
				// The end position has to be after the clip's start
				TimePos min = m_initialTCOPos.quantize( snapSize );
				if ( min <= m_initialTCOPos ) min += snapLength;
				m_tco->changeLength( qMax<int>(min - m_initialTCOPos, end - m_initialTCOPos) );
			}
			else
			{	// Otherwise, resize in fixed increments
				TimePos initialLength = m_initialTCOEnd - m_initialTCOPos;
				TimePos offset = TimePos( l - initialLength ).quantize( snapSize );
				// Don't resize to less than 1 tick
				TimePos min = TimePos( initialLength % snapLength );
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

				TimePos t = qMax( 0, (int)
									m_trackView->trackContainerView()->currentPosition() +
									static_cast<int>( x * TimePos::ticksPerBar() / ppb ) );

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
					TimePos max = m_initialTCOEnd.quantize( snapSize );
					if ( max >= m_initialTCOEnd ) max -= snapLength;
					t = qMin<int>( max, t.quantize( snapSize ) );
				}
				else
				{	// Otherwise, resize in fixed increments
					// Don't resize to less than 1 tick
					TimePos initialLength = m_initialTCOEnd - m_initialTCOPos;
					TimePos minLength = TimePos( initialLength % snapLength );
					if (minLength < 1) minLength += snapLength;
					TimePos offset = TimePos(t - m_initialTCOPos).quantize( snapSize );
					t = qMin<int>( m_initialTCOEnd - minLength, m_initialTCOPos + offset );
				}

				TimePos oldPos = m_tco->startPosition();
				if( m_tco->length() + ( oldPos - t ) >= 1 )
				{
					m_tco->movePosition( t );
					m_tco->changeLength( m_tco->length() + ( oldPos - t ) );
					sTco->setStartTimeOffset( sTco->startTimeOffset() + ( oldPos - t ) );
				}
			}
		}
		s_textFloat->setText( tr( "%1:%2 (%3:%4 to %5:%6)" ).
				arg( m_tco->length().getBar() ).
				arg( m_tco->length().getTicks() %
						TimePos::ticksPerBar() ).
				arg( m_tco->startPosition().getBar() + 1 ).
				arg( m_tco->startPosition().getTicks() %
						TimePos::ticksPerBar() ).
				arg( m_tco->endPosition().getBar() + 1 ).
				arg( m_tco->endPosition().getTicks() %
						TimePos::ticksPerBar() ) );
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
	QVector<TrackContentObjectView*> selectedTCOs = getClickedTCOs();

	// Depending on whether we right-clicked a selection or an individual TCO we will have
	// different labels for the actions.
	bool individualTCO = selectedTCOs.size() <= 1;

	if( cme->modifiers() )
	{
		return;
	}

	QMenu contextMenu( this );

	if( fixedTCOs() == false )
	{
		contextMenu.addAction(
			embed::getIconPixmap( "cancel" ),
			individualTCO
				? tr("Delete (middle mousebutton)")
				: tr("Delete selection (middle mousebutton)"),
			[this](){ contextMenuAction( Remove ); } );

		contextMenu.addSeparator();

		contextMenu.addAction(
			embed::getIconPixmap( "edit_cut" ),
			individualTCO
				? tr("Cut")
				: tr("Cut selection"),
			[this](){ contextMenuAction( Cut ); } );

		if (canMergeSelection(selectedTCOs))
		{
			contextMenu.addAction(
				embed::getIconPixmap("edit_merge"),
				tr("Merge Selection"),
				[this]() { contextMenuAction(Merge); }
			);
		}
	}

	contextMenu.addAction(
		embed::getIconPixmap( "edit_copy" ),
		individualTCO
			? tr("Copy")
			: tr("Copy selection"),
		[this](){ contextMenuAction( Copy ); } );

	contextMenu.addAction(
		embed::getIconPixmap( "edit_paste" ),
		tr( "Paste" ),
		[this](){ contextMenuAction( Paste ); } );

	contextMenu.addSeparator();

	contextMenu.addAction(
		embed::getIconPixmap( "muted" ),
		(individualTCO
			? tr("Mute/unmute (<%1> + middle click)")
			: tr("Mute/unmute selection (<%1> + middle click)")).arg(UI_CTRL_KEY),
		[this](){ contextMenuAction( Mute ); } );

	contextMenu.addSeparator();

	contextMenu.addAction( embed::getIconPixmap( "colorize" ),
			tr( "Set clip color" ), this, SLOT( changeClipColor() ) );
	contextMenu.addAction( embed::getIconPixmap( "colorize" ),
			tr( "Use track color" ), this, SLOT( useTrackColor() ) );

	constructContextMenu( &contextMenu );

	contextMenu.exec( QCursor::pos() );
}

// This method processes the actions from the context menu of the TCO View.
void TrackContentObjectView::contextMenuAction( ContextMenuAction action )
{
	QVector<TrackContentObjectView *> active = getClickedTCOs();
	// active will be later used for the remove, copy, cut or toggleMute methods

	switch( action )
	{
		case Remove:
			remove( active );
			break;
		case Cut:
			cut( active );
			break;
		case Copy:
			copy( active );
			break;
		case Paste:
			paste();
			break;
		case Mute:
			toggleMute( active );
			break;
		case Merge:
			mergeTCOs(active);
			break;
	}
}

QVector<TrackContentObjectView *> TrackContentObjectView::getClickedTCOs()
{
	// Get a list of selected selectableObjects
	QVector<selectableObject *> sos = gui->songEditor()->m_editor->selectedObjects();

	// Convert to a list of selected TCOVs
	QVector<TrackContentObjectView *> selection;
	selection.reserve( sos.size() );
	for( auto so: sos )
	{
		TrackContentObjectView *tcov = dynamic_cast<TrackContentObjectView *> ( so );
		if( tcov != nullptr )
		{
			selection.append( tcov );
		}
	}

	// If we clicked part of the selection, affect all selected clips. Otherwise affect the clip we clicked
	return selection.contains(this)
		? selection
		: QVector<TrackContentObjectView *>( 1, this );
}

void TrackContentObjectView::remove( QVector<TrackContentObjectView *> tcovs )
{
	for( auto tcov: tcovs )
	{
		// No need to check if it's nullptr because we check when building the QVector
		tcov->remove();
	}
}

void TrackContentObjectView::copy( QVector<TrackContentObjectView *> tcovs )
{
	// For copyStringPair()
	using namespace Clipboard;

	// Write the TCOs to a DataFile for copying
	DataFile dataFile = createTCODataFiles( tcovs );

	// Copy the TCO type as a key and the TCO data file to the clipboard
	copyStringPair( QString( "tco_%1" ).arg( m_tco->getTrack()->type() ),
		dataFile.toString() );
}

void TrackContentObjectView::cut( QVector<TrackContentObjectView *> tcovs )
{
	// Copy the selected TCOs
	copy( tcovs );

	// Now that the TCOs are copied we can delete them, since we are cutting
	remove( tcovs );
}

void TrackContentObjectView::paste()
{
	// For getMimeData()
	using namespace Clipboard;

	// If possible, paste the selection on the TimePos of the selected Track and remove it
	TimePos tcoPos = TimePos( m_tco->startPosition() );

	TrackContentWidget *tcw = getTrackView()->getTrackContentWidget();

	if( tcw->pasteSelection( tcoPos, getMimeData() ) )
	{
		// If we succeed on the paste we delete the TCO we pasted on
		remove();
	}
}

void TrackContentObjectView::toggleMute( QVector<TrackContentObjectView *> tcovs )
{
	for( auto tcov: tcovs )
	{
		// No need to check for nullptr because we check while building the tcovs QVector
		tcov->getTrackContentObject()->toggleMute();
	}
}

bool TrackContentObjectView::canMergeSelection(QVector<TrackContentObjectView*> tcovs)
{
	// Can't merge a single TCO
	if (tcovs.size() < 2) { return false; }

	// We check if the owner of the first TCO is an Instrument Track
	bool isInstrumentTrack = dynamic_cast<InstrumentTrackView*>(tcovs.at(0)->getTrackView());

	// Then we create a set with all the TCOs owners
	std::set<TrackView*> ownerTracks;
	for (auto tcov: tcovs) { ownerTracks.insert(tcov->getTrackView()); }

	// Can merge if there's only one owner track and it's an Instrument Track
	return isInstrumentTrack && ownerTracks.size() == 1;
}

void TrackContentObjectView::mergeTCOs(QVector<TrackContentObjectView*> tcovs)
{
	// Get the track that we are merging TCOs in
	InstrumentTrack* track =
		dynamic_cast<InstrumentTrack*>(tcovs.at(0)->getTrackView()->getTrack());

	if (!track)
	{
		qWarning("Warning: Couldn't retrieve InstrumentTrack in mergeTCOs()");
		return;
	}

	// For Undo/Redo
	track->addJournalCheckPoint();
	track->saveJournallingState(false);

	// Find the earliest position of all the selected TCOVs
	const auto earliestTCOV = std::min_element(tcovs.constBegin(), tcovs.constEnd(),
		[](TrackContentObjectView* a, TrackContentObjectView* b)
		{
			return a->getTrackContentObject()->startPosition() <
				b->getTrackContentObject()->startPosition();
		}
	);

	const TimePos earliestPos = (*earliestTCOV)->getTrackContentObject()->startPosition();

	// Create a pattern where all notes will be added
	Pattern* newPattern = dynamic_cast<Pattern*>(track->createTCO(earliestPos));
	if (!newPattern)
	{
		qWarning("Warning: Failed to convert TCO to Pattern on mergeTCOs");
		return;
	}

	newPattern->saveJournallingState(false);

	// Add the notes and remove the TCOs that are being merged
	for (auto tcov: tcovs)
	{
		// Convert TCOV to PatternView
		PatternView* pView = dynamic_cast<PatternView*>(tcov);

		if (!pView)
		{
			qWarning("Warning: Non-pattern TCO on InstrumentTrack");
			continue;
		}

		NoteVector currentTCONotes = pView->getPattern()->notes();
		TimePos pViewPos = pView->getPattern()->startPosition();

		for (Note* note: currentTCONotes)
		{
			Note* newNote = newPattern->addNote(*note, false);
			TimePos originalNotePos = newNote->pos();
			newNote->setPos(originalNotePos + (pViewPos - earliestPos));
		}

		// We disable the journalling system before removing, so the
		// removal doesn't get added to the undo/redo history
		tcov->getTrackContentObject()->saveJournallingState(false);
		// No need to check for nullptr because we check while building the tcovs QVector
		tcov->remove();
	}

	// Update length since we might have moved notes beyond the end of the pattern length
	newPattern->updateLength();
	// Rearrange notes because we might have moved them
	newPattern->rearrangeAllNotes();
	// Restore journalling states now that the operation is finished
	newPattern->restoreJournallingState();
	track->restoreJournallingState();
	// Update song
	Engine::getSong()->setModified();
	gui->songEditor()->update();
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
	QVector<TimePos> offsets;
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
TimePos TrackContentObjectView::draggedTCOPos( QMouseEvent * me )
{
	//Pixels per bar
	const float ppb = m_trackView->trackContainerView()->pixelsPerBar();
	// The pixel distance that the mouse has moved
	const int mouseOff = mapToGlobal(me->pos()).x() - m_initialMouseGlobalPos.x();
	TimePos newPos = m_initialTCOPos + mouseOff * TimePos::ticksPerBar() / ppb;
	TimePos offset = newPos - m_initialTCOPos;
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
		TimePos startQ = newPos.quantize( gui->songEditor()->m_editor->getSnapSize() );
		// Find start position that gives snapped clip end position
		TimePos endQ = ( newPos + m_tco->length() );
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


// Return the color that the TCO's background should be
QColor TrackContentObjectView::getColorForDisplay( QColor defaultColor )
{
	// Get the pure TCO color
	auto tcoColor = m_tco->hasColor()
					? m_tco->usesCustomClipColor()
						? m_tco->color()
						: m_tco->getTrack()->color()
					: defaultColor;

	// Set variables
	QColor c, mutedCustomColor;
	bool muted = m_tco->getTrack()->isMuted() || m_tco->isMuted();
	mutedCustomColor = tcoColor;
	mutedCustomColor.setHsv( mutedCustomColor.hsvHue(), mutedCustomColor.hsvSaturation() / 4, mutedCustomColor.value() );

	// Change the pure color by state: selected, muted, colored, normal
	if( isSelected() )
	{
		c = m_tco->hasColor()
			? ( muted
				? mutedCustomColor.darker( 350 )
				: tcoColor.darker( 150 ) )
			: selectedColor();
	}
	else
	{
		if( muted )
		{
			c = m_tco->hasColor()
				? mutedCustomColor.darker( 250 )
				: mutedBackgroundColor();
		}
		else
		{
			c = tcoColor;
		}
	}

	// Return color to caller
	return c;
}

