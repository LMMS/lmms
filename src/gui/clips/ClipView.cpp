/*
 * ClipView.cpp - implementation of ClipView class
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

#include "ClipView.h"

#include <set>
#include <cassert>

#include <QMenu>
#include <QMouseEvent>
#include <QPainter>

#include "AutomationClip.h"
#include "Clipboard.h"
#include "ColorChooser.h"
#include "DataFile.h"
#include "Engine.h"
#include "embed.h"
#include "GuiApplication.h"
#include "KeyboardShortcuts.h"
#include "MidiClipView.h"
#include "PatternClip.h"
#include "PatternStore.h"
#include "Song.h"
#include "SongEditor.h"
#include "StringPairDrag.h"
#include "TextFloat.h"
#include "TrackContainer.h"
#include "TrackContainerView.h"
#include "TrackView.h"

namespace lmms::gui
{


/*! The width of the resize grip in pixels
 */
const int RESIZE_GRIP_WIDTH = 4;


/*! A pointer for that text bubble used when moving segments, etc.
 *
 * In a number of situations, LMMS displays a floating text bubble
 * beside the cursor as you move or resize elements of a track about.
 * This pointer keeps track of it, as you only ever need one at a time.
 */
TextFloat * ClipView::s_textFloat = nullptr;


/*! \brief Create a new ClipView
 *
 *  Creates a new clip view for the given clip in the given track view.
 *
 * \param _clip The clip to be displayed
 * \param _tv  The track view that will contain the new object
 */
ClipView::ClipView( Clip * clip,
							TrackView * tv ) :
	selectableObject( tv->getTrackContentWidget() ),
	ModelView( nullptr, this ),
	m_trackView( tv ),
	m_initialClipPos( TimePos(0) ),
	m_initialClipEnd( TimePos(0) ),
	m_clip( clip ),
	m_action( Action::None ),
	m_initialMousePos( QPoint( 0, 0 ) ),
	m_initialMouseGlobalPos( QPoint( 0, 0 ) ),
	m_initialOffsets( QVector<TimePos>() ),
	m_hint( nullptr ),
	m_mutedColor( 0, 0, 0 ),
	m_mutedBackgroundColor( 0, 0, 0 ),
	m_selectedColor( 0, 0, 0 ),
	m_textColor( 0, 0, 0 ),
	m_textShadowColor( 0, 0, 0 ),
	m_patternClipBackground( 0, 0, 0 ),
	m_gradient( true ),
	m_markerColor(0, 0, 0),
	m_mouseHotspotHand( 0, 0 ),
	m_mouseHotspotKnife( 0, 0 ),
	m_cursorHand( QCursor( embed::getIconPixmap( "hand" ) ) ),
	m_cursorKnife( QCursor( embed::getIconPixmap( "cursor_knife" ) ) ),
	m_cursorSetYet( false ),
	m_needsUpdate( true )
{
	if( s_textFloat == nullptr )
	{
		s_textFloat = new TextFloat;
		s_textFloat->setPixmap( embed::getIconPixmap( "clock" ) );
	}

	setAttribute( Qt::WA_DeleteOnClose, true );
	setFocusPolicy( Qt::StrongFocus );
	setCursor( m_cursorHand );
	move( 0, 0 );
	show();

	setFixedHeight( tv->getTrackContentWidget()->height() - 1);
	setAcceptDrops( true );
	setMouseTracking( true );

	connect( m_clip, SIGNAL(lengthChanged()),
			this, SLOT(updateLength()));
	connect(getGUI()->songEditor()->m_editor, &SongEditor::pixelsPerBarChanged, this, &ClipView::updateLength);
	connect( m_clip, SIGNAL(positionChanged()),
			this, SLOT(updatePosition()));
	connect( m_clip, SIGNAL(destroyedClip()), this, SLOT(close()));
	setModel( m_clip );
	connect(m_clip, SIGNAL(colorChanged()), this, SLOT(update()));

	connect(m_trackView->getTrack(), &Track::colorChanged, this, [this]
	{
		// redraw if clip uses track color
		if (!m_clip->color().has_value()) { update(); }
	});

	m_trackView->getTrackContentWidget()->addClipView( this );
	updateLength();
	updatePosition();
}




/*! \brief Destroy a ClipView
 *
 *  Destroys the given ClipView.
 *
 */
ClipView::~ClipView()
{
	delete m_hint;
	// we have to give our track-container the focus because otherwise the
	// op-buttons of our track-widgets could become focus and when the user
	// presses space for playing song, just one of these buttons is pressed
	// which results in unwanted effects
	m_trackView->trackContainerView()->setFocus();
}


/*! \brief Update a ClipView
 *
 *  Clip's get drawn only when needed,
 *  and when a Clip is updated,
 *  it needs to be redrawn.
 *
 */
void ClipView::update()
{
	if( !m_cursorSetYet )
	{
		m_cursorHand = QCursor( embed::getIconPixmap( "hand" ), m_mouseHotspotHand.width(), m_mouseHotspotHand.height() );
		m_cursorKnife = QCursor( embed::getIconPixmap( "cursor_knife" ), m_mouseHotspotKnife.width(), m_mouseHotspotKnife.height() );
		setCursor( m_cursorHand );
		m_cursorSetYet = true;
	}

	if( fixedClips() )
	{
		updateLength();
	}
	m_needsUpdate = true;
	selectableObject::update();
}



/*! \brief Does this ClipView have a fixed Clip?
 *
 *  Returns whether the containing trackView has fixed
 *  Clips.
 *
 * \todo In what circumstance are they fixed?
 */
bool ClipView::fixedClips()
{
	return m_trackView->trackContainerView()->fixedClips();
}



// qproperty access functions, to be inherited & used by Clipviews
//! \brief CSS theming qproperty access method
QColor ClipView::mutedColor() const
{ return m_mutedColor; }

QColor ClipView::mutedBackgroundColor() const
{ return m_mutedBackgroundColor; }

QColor ClipView::selectedColor() const
{ return m_selectedColor; }

QColor ClipView::textColor() const
{ return m_textColor; }

QColor ClipView::textBackgroundColor() const
{
	return m_textBackgroundColor;
}

QColor ClipView::textShadowColor() const
{ return m_textShadowColor; }

QColor ClipView::patternClipBackground() const
{ return m_patternClipBackground; }

bool ClipView::gradient() const
{ return m_gradient; }

QColor ClipView::markerColor() const
{ return m_markerColor; }

//! \brief CSS theming qproperty access method
void ClipView::setMutedColor( const QColor & c )
{ m_mutedColor = QColor( c ); }

void ClipView::setMutedBackgroundColor( const QColor & c )
{ m_mutedBackgroundColor = QColor( c ); }

void ClipView::setSelectedColor( const QColor & c )
{ m_selectedColor = QColor( c ); }

void ClipView::setTextColor( const QColor & c )
{ m_textColor = QColor( c ); }

void ClipView::setTextBackgroundColor( const QColor & c )
{
	m_textBackgroundColor = c;
}

void ClipView::setTextShadowColor( const QColor & c )
{ m_textShadowColor = QColor( c ); }

void ClipView::setPatternClipBackground( const QColor & c )
{ m_patternClipBackground = QColor( c ); }

void ClipView::setGradient( const bool & b )
{ m_gradient = b; }

void ClipView::setMarkerColor(const QColor & c)
{ m_markerColor = QColor(c); }

// access needsUpdate member variable
bool ClipView::needsUpdate()
{ return m_needsUpdate; }
void ClipView::setNeedsUpdate( bool b )
{ m_needsUpdate = b; }

/*! \brief Close a ClipView
 *
 *  Closes a ClipView by asking the track
 *  view to remove us and then asking the QWidget to close us.
 *
 * \return Boolean state of whether the QWidget was able to close.
 */
bool ClipView::close()
{
	m_trackView->getTrackContentWidget()->removeClipView( this );
	return QWidget::close();
}




/*! \brief Removes a ClipView from its track view.
 *
 *  Like the close() method, this asks the track view to remove this
 *  ClipView.  However, the clip is
 *  scheduled for later deletion rather than closed immediately.
 *
 */
void ClipView::remove()
{
	m_trackView->getTrack()->addJournalCheckPoint();

	// delete ourself
	close();

	if (m_clip->getTrack())
	{
		auto guard = Engine::audioEngine()->requestChangesGuard();
		m_clip->getTrack()->removeClip(m_clip);
	}

	// TODO: Clip::~Clip should not be responsible for removing the Clip from the Track.
	// One would expect that a call to Track::removeClip would already do that for you, as well
	// as actually deleting the Clip with the deleteLater function. That being said, it shouldn't
	// be possible to make a Clip without a Track (i.e., Clip::getTrack is never nullptr).
	m_clip->deleteLater();

	m_trackView->update();
}




/*! \brief Updates a ClipView's length
 *
 *  If this ClipView has a fixed Clip, then we must
 *  keep the width of our parent.  Otherwise, calculate our width from
 *  the clip's length in pixels adding in the border.
 *
 */
void ClipView::updateLength()
{
	if( fixedClips() )
	{
		setFixedWidth( parentWidget()->width() );
	}
	else
	{
		// this std::max function is needed for clips that do not start or end on the beat, otherwise, they "disappear" when zooming to min 
		// 3 is the minimun width needed to make a clip visible
		setFixedWidth(std::max(static_cast<int>(m_clip->length() * pixelsPerBar() / TimePos::ticksPerBar() + 1), 3));
	}
	m_trackView->trackContainerView()->update();
}




/*! \brief Updates a ClipView's position.
 *
 *  Ask our track view to change our position.  Then make sure that the
 *  track view is updated in case this position has changed the track
 *  view's length.
 *
 */
void ClipView::updatePosition()
{
	m_trackView->getTrackContentWidget()->changePosition();
	// moving a Clip can result in change of song-length etc.,
	// therefore we update the track-container
	m_trackView->trackContainerView()->update();
}

void ClipView::selectColor()
{
	// Get a color from the user
	const auto newColor = ColorChooser{this}
		.withPalette(ColorChooser::Palette::Track)
		->getColor(m_clip->color().value_or(palette().window().color()));
	if (newColor.isValid()) { setColor(newColor); }
}

void ClipView::randomizeColor()
{
	setColor(ColorChooser::getPalette(ColorChooser::Palette::Mixer)[std::rand() % 48]);
}

void ClipView::resetColor()
{
	setColor(std::nullopt);
}

/*! \brief Change color of all selected clips
 *
 *  \param color The new color.
 */
void ClipView::setColor(const std::optional<QColor>& color)
{
	std::set<Track*> journaledTracks;

	auto selectedClips = getClickedClips();
	for (auto clipv : selectedClips)
	{
		auto clip = clipv->getClip();
		auto track = clip->getTrack();

		// TODO journal whole Song or group of clips instead of one journal entry for each track

		// If only one clip changed, store that in the journal
		if (selectedClips.length() == 1)
		{
			clip->addJournalCheckPoint();
		}
		// If multiple clips changed, store whole Track in the journal
		// Check if track has been journaled already by trying to add it to the set
		else if (journaledTracks.insert(track).second)
		{
			track->addJournalCheckPoint();
		}

		clip->setColor(color);
		clipv->update();
	}

	Engine::getSong()->setModified();
}

/*! \brief Change the ClipView's display when something
 *  being dragged enters it.
 *
 *  We need to notify Qt to change our display if something being
 *  dragged has entered our 'airspace'.
 *
 * \param dee The QDragEnterEvent to watch.
 */
void ClipView::dragEnterEvent( QDragEnterEvent * dee )
{
	TrackContentWidget * tcw = getTrackView()->getTrackContentWidget();
	TimePos clipPos{m_clip->startPosition()};

	if( tcw->canPasteSelection( clipPos, dee ) == false )
	{
		dee->ignore();
	}
	else
	{
		StringPairDrag::processDragEnterEvent(dee, {
			QString("clip_%1").arg(static_cast<int>(m_clip->getTrack()->type()))
		});
	}
}




/*! \brief Handle something being dropped on this ClipObjectView.
 *
 *  When something has been dropped on this ClipView, and
 *  it's a clip, then use an instance of our dataFile reader
 *  to take the xml of the clip and turn it into something
 *  we can write over our current state.
 *
 * \param de The QDropEvent to handle.
 */
void ClipView::dropEvent(QDropEvent* de)
{
	const auto [type, value] = Clipboard::decodeMimeData(de->mimeData());

	// Track must be the same type to paste into
	if( type != ( "clip_" + QString::number( static_cast<int>(m_clip->getTrack()->type()) ) ) )
	{
		de->ignore();
		return;
	}

	// Defer to rubberband paste if we're in that mode
	if( m_trackView->trackContainerView()->allowRubberband() == true )
	{
		TrackContentWidget * tcw = getTrackView()->getTrackContentWidget();
		TimePos clipPos{m_clip->startPosition()};

		if( tcw->pasteSelection( clipPos, de ) == true )
		{
			de->accept();
		}
		de->ignore();
		return;
	}

	// Don't allow pasting a clip into itself.
	QObject* qwSource = de->source();
	if( qwSource != nullptr &&
	    dynamic_cast<ClipView *>( qwSource ) == this )
	{
		de->ignore();
		return;
	}

	// Copy state into existing clip
	DataFile dataFile( value.toUtf8() );
	TimePos pos = m_clip->startPosition();
	QDomElement clips = dataFile.content().firstChildElement("clips");
	m_clip->restoreState( clips.firstChildElement().firstChildElement() );
	m_clip->movePosition( pos );
	AutomationClip::resolveAllIDs();
	de->accept();
}




/* @brief Chooses the correct cursor to be displayed on the widget
 *
 * @param me The QMouseEvent that is triggering the cursor change
 */
void ClipView::updateCursor(QMouseEvent * me)
{
	// If we are at the edges, use the resize cursor
	if (!me->buttons() && m_clip->manuallyResizable() && !isSelected()
		&& ((me->x() > width() - RESIZE_GRIP_WIDTH) || (me->x() < RESIZE_GRIP_WIDTH)))
	{
		setCursor(Qt::SizeHorCursor);
	}
	// If we are in the middle on knife mode, use the knife cursor
	else if (m_trackView->trackContainerView()->knifeMode() && !isSelected())
	{
		setCursor(m_cursorKnife);
	}
	// If we are in the middle in any other mode, use the hand cursor
	else { setCursor(m_cursorHand); }
}




/*! \brief Create a DataFile suitable for copying multiple clips.
 *
 *	Clips in the vector are written to the "clips" node in the
 *  DataFile.  The ClipView's initial mouse position is written
 *  to the "initialMouseX" node in the DataFile.  When dropped on a track,
 *  this is used to create copies of the Clips.
 *
 * \param clips The trackContectObjects to save in a DataFile
 */
DataFile ClipView::createClipDataFiles(
    				const QVector<ClipView *> & clipViews) const
{
	Track * t = m_trackView->getTrack();
	TrackContainer * tc = t->trackContainer();
	DataFile dataFile( DataFile::Type::DragNDropData );
	QDomElement clipParent = dataFile.createElement("clips");

	for (const auto& clipView : clipViews)
	{
		// Insert into the dom under the "clips" element
		Track* clipTrack = clipView->m_trackView->getTrack();
		const auto trackIt = std::find(tc->tracks().begin(), tc->tracks().end(), clipTrack);
		assert(trackIt != tc->tracks().end());
		int trackIndex = std::distance(tc->tracks().begin(), trackIt);
		QDomElement clipElement = dataFile.createElement("clip");
		clipElement.setAttribute( "trackIndex", trackIndex );
		clipElement.setAttribute( "trackType", static_cast<int>(clipTrack->type()) );
		clipElement.setAttribute( "trackName", clipTrack->name() );
		clipView->m_clip->saveState(dataFile, clipElement);
		clipParent.appendChild( clipElement );
	}

	dataFile.content().appendChild( clipParent );

	// Add extra metadata needed for calculations later

	const auto initialTrackIt = std::find(tc->tracks().begin(), tc->tracks().end(), t);
	if (initialTrackIt == tc->tracks().end())
	{
		printf("Failed to find selected track in the TrackContainer.\n");
		return dataFile;
	}

	const int initialTrackIndex = std::distance(tc->tracks().begin(), initialTrackIt);
	QDomElement metadata = dataFile.createElement( "copyMetadata" );
	// initialTrackIndex is the index of the track that was touched
	metadata.setAttribute( "initialTrackIndex", initialTrackIndex );
	metadata.setAttribute( "trackContainerId", tc->id() );
	// grabbedClipPos is the pos of the bar containing the Clip we grabbed
	metadata.setAttribute( "grabbedClipPos", m_clip->startPosition() );

	dataFile.content().appendChild( metadata );

	return dataFile;
}

void ClipView::paintTextLabel(QString const & text, QPainter & painter)
{
	if (text.trimmed() == "")
	{
		return;
	}

	painter.setRenderHint( QPainter::TextAntialiasing );

	QFont labelFont = this->font();
	labelFont.setHintingPreference( QFont::PreferFullHinting );
	painter.setFont( labelFont );

	const int textTop = BORDER_WIDTH + 1;
	const int textLeft = BORDER_WIDTH + 3;

	QFontMetrics fontMetrics(labelFont);
	QString elidedClipName = fontMetrics.elidedText(text, Qt::ElideMiddle, width() - 2 * textLeft);

	if (elidedClipName.length() < 2)
	{
		elidedClipName = text.trimmed();
	}

	painter.fillRect(QRect(0, 0, width(), fontMetrics.height() + 2 * textTop), textBackgroundColor());

	int const finalTextTop = textTop + fontMetrics.ascent();
	painter.setPen(textShadowColor());
	painter.drawText( textLeft + 1, finalTextTop + 1, elidedClipName );
	painter.setPen( textColor() );
	painter.drawText( textLeft, finalTextTop, elidedClipName );
}

/*! \brief Handle a mouse press on this ClipView.
 *
 *  Handles the various ways in which a ClipView can be
 *  used with a click of a mouse button.
 *
 *  * If our container supports rubber band selection then handle
 *    selection events.
 *  * or if shift-left button, add this object to the selection
 *  * or if ctrl-left button, start a drag-copy event
 *  * or if just plain left button, resize if we're resizeable
 *  * or if ctrl-middle button, mute the clip
 *  * or if middle button, maybe delete the clip.
 *
 * \param me The QMouseEvent to handle.
 */
void ClipView::mousePressEvent( QMouseEvent * me )
{
	// Right now, active is only used on right/mid clicks actions, so we use a ternary operator
	// to avoid the overhead of calling getClickedClips when it's not used
	auto active = me->button() == Qt::LeftButton
		? QVector<ClipView *>()
		: getClickedClips();

	setInitialPos( me->pos() );
	setInitialOffsets();
	if( !fixedClips() && me->button() == Qt::LeftButton )
	{
		const bool knifeMode = m_trackView->trackContainerView()->knifeMode();

		if (me->modifiers() & KBD_COPY_MODIFIER && !knifeMode)
		{
			if( isSelected() )
			{
				m_action = Action::CopySelection;
			}
			else
			{
				m_action = Action::ToggleSelected;
			}
		}
		else
		{
			if( isSelected() )
			{
				m_action = Action::MoveSelection;
			}
			else
			{
				getGUI()->songEditor()->m_editor->selectAllClips( false );
				m_clip->addJournalCheckPoint();

				// Action::Move, Action::Resize and Action::ResizeLeft
				// Action::Split action doesn't disable Clip journalling
				if (m_action == Action::Move || m_action == Action::Resize || m_action == Action::ResizeLeft)
				{
					m_clip->setJournalling(false);
				}

				setInitialPos( me->pos() );
				setInitialOffsets();

				if (!m_clip->manuallyResizable() && !knifeMode)
				{	// Always move clips that can't be manually resized
					m_action = Action::Move;
					setCursor( Qt::SizeAllCursor );
				}
				else if( me->x() >= width() - RESIZE_GRIP_WIDTH )
				{
					m_action = Action::Resize;
					setCursor( Qt::SizeHorCursor );
				}
				else if (me->x() < RESIZE_GRIP_WIDTH)
				{
					m_action = Action::ResizeLeft;
					setCursor( Qt::SizeHorCursor );
				}
				else if (knifeMode)
				{
					m_action = Action::Split;
					setCursor( m_cursorKnife );
					setMarkerPos( knifeMarkerPos( me ) );
					setMarkerEnabled( true );
					update();
				}
				else
				{
					m_action = Action::Move;
					setCursor( Qt::SizeAllCursor );
				}

				if( m_action == Action::Move )
				{
					s_textFloat->setTitle( tr( "Current position" ) );
					s_textFloat->setText( QString( "%1:%2" ).
						arg( m_clip->startPosition().getBar() + 1 ).
						arg( m_clip->startPosition().getTicks() %
								TimePos::ticksPerBar() ) );
				}
				else if( m_action == Action::Resize || m_action == Action::ResizeLeft )
				{
					s_textFloat->setTitle( tr( "Current length" ) );
					s_textFloat->setText( tr( "%1:%2 (%3:%4 to %5:%6)" ).
							arg( m_clip->length().getBar() ).
							arg( m_clip->length().getTicks() %
									TimePos::ticksPerBar() ).
							arg( m_clip->startPosition().getBar() + 1 ).
							arg( m_clip->startPosition().getTicks() %
									TimePos::ticksPerBar() ).
							arg( m_clip->endPosition().getBar() + 1 ).
							arg( m_clip->endPosition().getTicks() %
									TimePos::ticksPerBar() ) );
				}
				// s_textFloat->reparent( this );
				// setup text-float as if Clip was already moved/resized
				s_textFloat->moveGlobal( this, QPoint( width() + 2, height() + 2) );
				if ( m_action != Action::Split) { s_textFloat->show(); }
			}

			delete m_hint;
			QString hint;
			if (m_action == Action::Move || m_action == Action::MoveSelection)
			{
				hint = tr("Press <%1> and drag to make a copy.");
			}
			else if (m_action == Action::Split)
			{
				hint = dynamic_cast<MidiClipView*>(this)
					? tr("Press <%1> or <Alt> for unquantized splitting.\nPress <Shift> for destructive splitting.")
					: tr("Press <%1> or <Alt> for unquantized splitting.");
			}
			else
			{
				hint = tr("Press <%1> or <Alt> for unquantized resizing.");
			}
			m_hint = TextFloat::displayMessage( tr( "Hint" ), hint.arg(UI_COPY_KEY),
					embed::getIconPixmap( "hint" ), 0 );
		}
	}
	else if( me->button() == Qt::RightButton )
	{
		if( me->modifiers() & Qt::ControlModifier )
		{
			toggleMute( active );
		}
		else if( me->modifiers() & Qt::ShiftModifier && !fixedClips() )
		{
			remove( active );
		}
		if (m_action == Action::Split)
		{
			m_action = Action::None;
			setMarkerEnabled(false);
			update();
		}
	}
	else if( me->button() == Qt::MiddleButton )
	{
		if( me->modifiers() & Qt::ControlModifier )
		{
			toggleMute( active );
		}
		else if( !fixedClips() )
		{
			remove( active );
		}
	}
}




/*! \brief Handle a mouse movement (drag) on this ClipView.
 *
 *  Handles the various ways in which a ClipView can be
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
void ClipView::mouseMoveEvent( QMouseEvent * me )
{
	if( m_action == Action::CopySelection || m_action == Action::ToggleSelected )
	{
		if( mouseMovedDistance( me, 2 ) == true )
		{
			QVector<ClipView *> clipViews;
			if( m_action == Action::CopySelection )
			{
				// Collect all selected Clips
				QVector<selectableObject *> so =
					m_trackView->trackContainerView()->selectedObjects();
				for (const auto& selectedClip : so)
				{
					auto clipv = dynamic_cast<ClipView*>(selectedClip);
					if( clipv != nullptr )
					{
						clipViews.push_back( clipv );
					}
				}
			}
			else
			{
				getGUI()->songEditor()->m_editor->selectAllClips( false );
				clipViews.push_back( this );
			}
			// Clear the action here because mouseReleaseEvent will not get
			// triggered once we go into drag.
			m_action = Action::None;

			// Write the Clips to the DataFile for copying
			DataFile dataFile = createClipDataFiles( clipViews );

			// TODO -- thumbnail for all selected
			QPixmap thumbnail = grab().scaled(
				128, 128,
				Qt::KeepAspectRatio,
				Qt::SmoothTransformation );
			new StringPairDrag( QString( "clip_%1" ).arg(
								static_cast<int>(m_clip->getTrack()->type()) ),
								dataFile.toString(), thumbnail, this );
		}
	}

	if (me->modifiers() & KBD_COPY_MODIFIER)
	{
		delete m_hint;
		m_hint = nullptr;
	}

	const float ppb = m_trackView->trackContainerView()->pixelsPerBar();
	if( m_action == Action::Move )
	{
		TimePos newPos = draggedClipPos( me );
		m_clip->movePosition(newPos);
		newPos = m_clip->startPosition(); // Get the real position the Clip was dragged to for the label
		m_trackView->getTrackContentWidget()->changePosition();
		s_textFloat->setText( QString( "%1:%2" ).
				arg( newPos.getBar() + 1 ).
				arg( newPos.getTicks() %
						TimePos::ticksPerBar() ) );
		s_textFloat->moveGlobal( this, QPoint( width() + 2, height() + 2 ) );
	}
	else if( m_action == Action::MoveSelection )
	{
		// 1: Find the position we want to move the grabbed Clip to
		TimePos newPos = draggedClipPos( me );

		// 2: Handle moving the other selected Clips the same distance
		QVector<selectableObject *> so =
			m_trackView->trackContainerView()->selectedObjects();
		QVector<Clip *> clips; // List of selected clips
		int leftmost = 0; // Leftmost clip's offset from grabbed clip
		// Populate clips, find leftmost
		for( QVector<selectableObject *>::iterator it = so.begin();
							it != so.end(); ++it )
		{
			auto clipv = dynamic_cast<ClipView*>(*it);
			if( clipv == nullptr ) { continue; }
			clips.push_back( clipv->m_clip );
			int index = std::distance( so.begin(), it );
			leftmost = std::min(leftmost, m_initialOffsets[index].getTicks());
		}
		// Make sure the leftmost clip doesn't get moved to a negative position
		if ( newPos.getTicks() + leftmost < 0 ) { newPos = -leftmost; }

		for( QVector<Clip *>::iterator it = clips.begin();
							it != clips.end(); ++it )
		{
			int index = std::distance( clips.begin(), it );
			( *it )->movePosition( newPos + m_initialOffsets[index] );
		}
	}
	else if( m_action == Action::Resize || m_action == Action::ResizeLeft )
	{
		const float snapSize = getGUI()->songEditor()->m_editor->getSnapSize();
		// Length in ticks of one snap increment
		const TimePos snapLength = TimePos( (int)(snapSize * TimePos::ticksPerBar()) );

		if( m_action == Action::Resize )
		{
			// The clip's new length
			TimePos l = static_cast<int>( me->x() * TimePos::ticksPerBar() / ppb );

			// If the user is holding alt, or pressed ctrl after beginning the drag, don't quantize
			if ( unquantizedModHeld(me) )
			{	// We want to preserve this adjusted offset,
				// even if the user switches to snapping later
				setInitialPos( m_initialMousePos );
				// Don't resize to less than 1 tick
				m_clip->changeLength( qMax<int>( 1, l ) );
				m_clip->setAutoResize(false);
			}
			else if ( me->modifiers() & Qt::ShiftModifier )
			{	// If shift is held, quantize clip's end position
				TimePos end = TimePos( m_initialClipPos + l ).quantize( snapSize );
				// The end position has to be after the clip's start
				TimePos min = m_initialClipPos.quantize( snapSize );
				if ( min <= m_initialClipPos ) min += snapLength;
				m_clip->changeLength( qMax<int>(min - m_initialClipPos, end - m_initialClipPos) );
				m_clip->setAutoResize(false);
			}
			else
			{	// Otherwise, resize in fixed increments
				TimePos initialLength = m_initialClipEnd - m_initialClipPos;
				TimePos offset = TimePos( l - initialLength ).quantize( snapSize );
				// Don't resize to less than 1 tick
				auto min = TimePos(initialLength % snapLength);
				if (min < 1) min += snapLength;
				m_clip->changeLength( qMax<int>( min, initialLength + offset) );
				m_clip->setAutoResize(false);
			}
		}
		else
		{
			auto pClip = dynamic_cast<PatternClip*>(m_clip);

			const int x = mapToParent( me->pos() ).x() - m_initialMousePos.x();

			TimePos t = qMax( 0, (int)
								m_trackView->trackContainerView()->currentPosition() +
								static_cast<int>( x * TimePos::ticksPerBar() / ppb ) );

			if (!isResizableBeforeStart())
			{
				t = std::max(t, static_cast<TimePos>(m_clip->startPosition() + m_clip->startTimeOffset()));
			}

			if( unquantizedModHeld(me) )
			{	// We want to preserve this adjusted offset,
				// even if the user switches to snapping later
				setInitialPos( m_initialMousePos );
				//Don't resize to less than 1 tick
				t = qMin<int>( m_initialClipEnd - 1, t);
			}
			else if( me->modifiers() & Qt::ShiftModifier )
			{	// If shift is held, quantize clip's start position
				// Don't let the start position move past the end position
				TimePos max = m_initialClipEnd.quantize( snapSize );
				if ( max >= m_initialClipEnd ) max -= snapLength;
				t = qMin<int>( max, t.quantize( snapSize ) );
			}
			else
			{	// Otherwise, resize in fixed increments
				// Don't resize to less than 1 tick
				TimePos initialLength = m_initialClipEnd - m_initialClipPos;
				auto minLength = TimePos(initialLength % snapLength);
				if (minLength < 1) minLength += snapLength;
				TimePos offset = TimePos(t - m_initialClipPos).quantize( snapSize );
				t = qMin<int>( m_initialClipEnd - minLength, m_initialClipPos + offset );
			}

			TimePos positionOffset = m_clip->startPosition() - t;
			if (m_clip->length() + positionOffset >= 1)
			{
				m_clip->movePosition(t);
				m_clip->changeLength(m_clip->length() + positionOffset);
				if (pClip)
				{
					// Modulus the start time offset as we need it only for offsets
					// inside the pattern length. This is done to prevent a value overflow.
					// The start time offset may still become larger than the pattern length
					// whenever the pattern length decreases without a clip resize following.
					// To deal safely with it, always modulus before use.
					tick_t patternLength = Engine::patternStore()->lengthOfPattern(pClip->patternIndex())
							* TimePos::ticksPerBar();
					TimePos position = (pClip->startTimeOffset() + positionOffset) % patternLength;
					pClip->setStartTimeOffset(position);
				}
				else
				{
					m_clip->setStartTimeOffset(m_clip->startTimeOffset() + positionOffset);
				}
				m_clip->setAutoResize(false);
			}
		}
		s_textFloat->setText( tr( "%1:%2 (%3:%4 to %5:%6)" ).
				arg( m_clip->length().getBar() ).
				arg( m_clip->length().getTicks() %
						TimePos::ticksPerBar() ).
				arg( m_clip->startPosition().getBar() + 1 ).
				arg( m_clip->startPosition().getTicks() %
						TimePos::ticksPerBar() ).
				arg( m_clip->endPosition().getBar() + 1 ).
				arg( m_clip->endPosition().getTicks() %
						TimePos::ticksPerBar() ) );
		s_textFloat->moveGlobal( this, QPoint( width() + 2, height() + 2) );
	}
	else if( m_action == Action::Split )
	{
		setCursor(m_cursorKnife);
		setMarkerPos(knifeMarkerPos(me));
		update();
	}
	// None of the actions above, we will just handle the cursor
	else { updateCursor(me); }
}




/*! \brief Handle a mouse release on this ClipView.
 *
 *  If we're in move or resize mode, journal the change as appropriate.
 *  Then tidy up.
 *
 * \param me The QMouseEvent to handle.
 */
void ClipView::mouseReleaseEvent( QMouseEvent * me )
{
	// If the Action::CopySelection was chosen as the action due to mouse movement,
	// it will have been cleared.  At this point Toggle is the desired action.
	// An active StringPairDrag will prevent this method from being called,
	// so a real Action::CopySelection would not have occurred.
	if( m_action == Action::CopySelection ||
	    ( m_action == Action::ToggleSelected && mouseMovedDistance( me, 2 ) == false ) )
	{
		setSelected( !isSelected() );
	}
	else if( m_action == Action::Move || m_action == Action::Resize || m_action == Action::ResizeLeft )
	{
		// TODO: Fix m_clip->setJournalling() consistency
		m_clip->setJournalling( true );
	}
	else if( m_action == Action::Split )
	{
		const float ppb = m_trackView->trackContainerView()->pixelsPerBar();
		const TimePos relPos = me->pos().x() * TimePos::ticksPerBar() / ppb;
		if (me->modifiers() & Qt::ShiftModifier)
		{
			destructiveSplitClip(unquantizedModHeld(me) ? relPos : quantizeSplitPos(relPos));
		}
		else
		{
			splitClip(unquantizedModHeld(me) ? relPos : quantizeSplitPos(relPos));
		}
		setMarkerEnabled(false);
	}

	m_action = Action::None;
	delete m_hint;
	m_hint = nullptr;
	s_textFloat->hide();
	updateCursor(me);
	selectableObject::mouseReleaseEvent( me );
}




/*! \brief Set up the context menu for this ClipView.
 *
 *  Set up the various context menu events that can apply to a
 *  ClipView.
 *
 * \param cme The QContextMenuEvent to add the actions to.
 */
void ClipView::contextMenuEvent( QContextMenuEvent * cme )
{
	QVector<ClipView*> selectedClips = getClickedClips();

	// Depending on whether we right-clicked a selection or an individual Clip we will have
	// different labels for the actions.
	bool individualClip = selectedClips.size() <= 1;

	if( cme->modifiers() )
	{
		return;
	}

	QMenu contextMenu( this );

	if( fixedClips() == false )
	{
		contextMenu.addAction(
			embed::getIconPixmap( "cancel" ),
			individualClip
				? tr("Delete (middle mousebutton)")
				: tr("Delete selection (middle mousebutton)"),
			[this](){ contextMenuAction( ContextMenuAction::Remove ); } );

		contextMenu.addSeparator();

		contextMenu.addAction(
			embed::getIconPixmap( "edit_cut" ),
			individualClip
				? tr("Cut")
				: tr("Cut selection"),
			[this](){ contextMenuAction( ContextMenuAction::Cut ); } );
	}

	contextMenu.addAction(
		embed::getIconPixmap( "edit_copy" ),
		individualClip
			? tr("Copy")
			: tr("Copy selection"),
		[this](){ contextMenuAction( ContextMenuAction::Copy ); } );

	contextMenu.addAction(
		embed::getIconPixmap( "edit_paste" ),
		tr( "Paste" ),
		[this](){ contextMenuAction( ContextMenuAction::Paste ); } );

	contextMenu.addSeparator();

	contextMenu.addAction(
		embed::getIconPixmap( "muted" ),
		(individualClip
			? tr("Mute/unmute (<%1> + middle click)")
			: tr("Mute/unmute selection (<%1> + middle click)")).arg(UI_CTRL_KEY),
		[this](){ contextMenuAction( ContextMenuAction::Mute ); } );

	contextMenu.addSeparator();

	QMenu colorMenu (tr("Clip color"), this);
	colorMenu.setIcon(embed::getIconPixmap("colorize"));
	colorMenu.addAction(tr("Change"), this, SLOT(selectColor()));
	colorMenu.addAction(tr("Reset"), this, SLOT(resetColor()));
	colorMenu.addAction(tr("Pick random"), this, SLOT(randomizeColor()));
	contextMenu.addMenu(&colorMenu);

	contextMenu.addAction(
		m_clip->getAutoResize() ? embed::getIconPixmap("auto_resize_disable") : embed::getIconPixmap("auto_resize"),
		m_clip->getAutoResize() ? tr("Disable auto-resize") : tr("Enable auto-resize"),
		this, &ClipView::toggleSelectedAutoResize
	);

	constructContextMenu( &contextMenu );

	contextMenu.exec( QCursor::pos() );
}

// This method processes the actions from the context menu of the Clip View.
void ClipView::contextMenuAction( ContextMenuAction action )
{
	QVector<ClipView *> active = getClickedClips();
	// active will be later used for the remove, copy, cut or toggleMute methods

	switch( action )
	{
		case ContextMenuAction::Remove:
			remove( active );
			break;
		case ContextMenuAction::Cut:
			cut( active );
			break;
		case ContextMenuAction::Copy:
			copy( active );
			break;
		case ContextMenuAction::Paste:
			paste();
			break;
		case ContextMenuAction::Mute:
			toggleMute( active );
			break;
	}
}

QVector<ClipView *> ClipView::getClickedClips()
{
	// Get a list of selected selectableObjects
	QVector<selectableObject *> sos = getGUI()->songEditor()->m_editor->selectedObjects();

	// Convert to a list of selected ClipVs
	QVector<ClipView *> selection;
	selection.reserve( sos.size() );
	for( auto so: sos )
	{
		auto clipv = dynamic_cast<ClipView*>(so);
		if( clipv != nullptr )
		{
			selection.append( clipv );
		}
	}

	// If we clicked part of the selection, affect all selected clips. Otherwise affect the clip we clicked
	return selection.contains(this)
		? selection
		: QVector<ClipView *>( 1, this );
}

void ClipView::remove( QVector<ClipView *> clipvs )
{
	for( auto clipv: clipvs )
	{
		// No need to check if it's nullptr because we check when building the QVector
		clipv->remove();
	}
}

void ClipView::copy( QVector<ClipView *> clipvs )
{
	// For copyStringPair()
	using namespace Clipboard;

	// Write the Clips to a DataFile for copying
	DataFile dataFile = createClipDataFiles( clipvs );

	// Copy the Clip type as a key and the Clip data file to the clipboard
	copyStringPair( QString( "clip_%1" ).arg( static_cast<int>(m_clip->getTrack()->type()) ),
		dataFile.toString() );
}

void ClipView::cut( QVector<ClipView *> clipvs )
{
	// Copy the selected Clips
	copy( clipvs );

	// Now that the Clips are copied we can delete them, since we are cutting
	remove( clipvs );
}

void ClipView::paste()
{
	// For getMimeData()
	using namespace Clipboard;

	// If possible, paste the selection on the TimePos of the selected Track and remove it
	TimePos clipPos{m_clip->startPosition()};

	TrackContentWidget *tcw = getTrackView()->getTrackContentWidget();

	if( tcw->pasteSelection( clipPos, getMimeData() ) )
	{
		// If we succeed on the paste we delete the Clip we pasted on
		remove();
	}
}

void ClipView::toggleMute( QVector<ClipView *> clipvs )
{
	for( auto clipv: clipvs )
	{
		// No need to check for nullptr because we check while building the clipvs QVector
		clipv->getClip()->toggleMute();
	}
}

void ClipView::toggleSelectedAutoResize()
{
	const bool newState = !m_clip->getAutoResize();
	std::set<Track*> journaledTracks;
	for (auto clipv: getClickedClips())
	{
		Clip* clip = clipv->getClip();
		if (journaledTracks.insert(clip->getTrack()).second) { clip->getTrack()->addJournalCheckPoint(); }
		clip->setAutoResize(newState);
		clip->updateLength();
	}
}

/*! \brief How many pixels a bar takes for this ClipView.
 *
 * \return the number of pixels per bar.
 */
float ClipView::pixelsPerBar()
{
	return m_trackView->trackContainerView()->pixelsPerBar();
}


/*! \brief Save the offsets between all selected tracks and a clicked track */
void ClipView::setInitialOffsets()
{
	QVector<selectableObject *> so = m_trackView->trackContainerView()->selectedObjects();
	QVector<TimePos> offsets;
	for (const auto& selectedClip : so)
	{
		auto clipv = dynamic_cast<ClipView*>(selectedClip);
		if( clipv == nullptr )
		{
			continue;
		}
		offsets.push_back( clipv->m_clip->startPosition() - m_initialClipPos );
	}

	m_initialOffsets = offsets;
}




/*! \brief Detect whether the mouse moved more than n pixels on screen.
 *
 * \param _me The QMouseEvent.
 * \param distance The threshold distance that the mouse has moved to return true.
 */
bool ClipView::mouseMovedDistance( QMouseEvent * me, int distance )
{
	QPoint dPos = mapToGlobal( me->pos() ) - m_initialMouseGlobalPos;
	const int pixelsMoved = dPos.manhattanLength();
	return ( pixelsMoved > distance || pixelsMoved < -distance );
}




bool ClipView::unquantizedModHeld( QMouseEvent * me )
{
	return me->modifiers() & Qt::ControlModifier || me->modifiers() & Qt::AltModifier;
}




/*! \brief Calculate the new position of a dragged Clip from a mouse event
 *
 *
 * \param me The QMouseEvent
 */
TimePos ClipView::draggedClipPos( QMouseEvent * me )
{
	//Pixels per bar
	const float ppb = m_trackView->trackContainerView()->pixelsPerBar();
	// The pixel distance that the mouse has moved
	const int mouseOff = mapToGlobal(me->pos()).x() - m_initialMouseGlobalPos.x();
	TimePos newPos = m_initialClipPos + mouseOff * TimePos::ticksPerBar() / ppb;
	TimePos offset = newPos - m_initialClipPos;
	// If the user is holding alt, or pressed ctrl after beginning the drag, don't quantize
	if ( me->button() != Qt::NoButton || unquantizedModHeld(me) )
	{	// We want to preserve this adjusted offset,  even if the user switches to snapping
		setInitialPos( m_initialMousePos );
	}
	else if ( me->modifiers() & Qt::ShiftModifier )
	{	// If shift is held, quantize position (Default in 1.2.0 and earlier)
		// or end position, whichever is closest to the actual position
		TimePos startQ = newPos.quantize( getGUI()->songEditor()->m_editor->getSnapSize() );
		// Find start position that gives snapped clip end position
		TimePos endQ = ( newPos + m_clip->length() );
		endQ = endQ.quantize( getGUI()->songEditor()->m_editor->getSnapSize() );
		endQ = endQ - m_clip->length();

		// Select the position closest to actual position
		if (std::abs(newPos - startQ) < std::abs(newPos - endQ)) { newPos = startQ; }
		else newPos = endQ;
	}
	else
	{	// Otherwise, quantize moved distance (preserves user offsets)
		newPos = m_initialClipPos + offset.quantize( getGUI()->songEditor()->m_editor->getSnapSize() );
	}
	return newPos;
}


int ClipView::knifeMarkerPos( QMouseEvent * me )
{
	//Position relative to start of clip
	const int markerPos = me->pos().x();

	//In unquantized mode, we don't have to mess with the position at all
	if ( unquantizedModHeld(me) ) { return markerPos; }
	else
	{	//Otherwise we...
		//1: Convert the position to a TimePos
		const float ppb = m_trackView->trackContainerView()->pixelsPerBar();
		TimePos midiPos = markerPos * TimePos::ticksPerBar() / ppb;
		//2: Snap to the correct position, based on modifier keys
		midiPos = quantizeSplitPos(midiPos);
		//3: Convert back to a pixel position
		return midiPos * ppb / TimePos::ticksPerBar();
	}
}




TimePos ClipView::quantizeSplitPos(TimePos midiPos)
{
	const float snapSize = getGUI()->songEditor()->m_editor->getSnapSize();
	// quantize the length of the new left clip...
	const TimePos leftPos = midiPos.quantize(snapSize);
	//...or right clip...
	const TimePos rightOff = m_clip->length() - midiPos;
	const TimePos rightPos = m_clip->length() - rightOff.quantize(snapSize);
	//...or the global gridlines
	const TimePos globalPos = TimePos(midiPos + m_initialClipPos).quantize(snapSize) - m_initialClipPos;
	//...whichever gives a position closer to the cursor
	if (abs(leftPos - midiPos) <= abs(rightPos - midiPos) && abs(leftPos - midiPos) <= abs(globalPos - midiPos)) { return leftPos; }
	else if (abs(rightPos - midiPos) <= abs(leftPos - midiPos) && abs(rightPos - midiPos) <= abs(globalPos - midiPos)) { return rightPos; }
	else { return globalPos; }
}




// Return the color that the Clip's background should be
QColor ClipView::getColorForDisplay( QColor defaultColor )
{
	// Get the pure Clip color
	auto clipColor = m_clip->color().value_or(m_clip->getTrack()->color().value_or(defaultColor));

	// Set variables
	QColor c, mutedCustomColor;
	bool muted = m_clip->getTrack()->isMuted() || m_clip->isMuted();
	mutedCustomColor = clipColor;
	mutedCustomColor.setHsv( mutedCustomColor.hsvHue(), mutedCustomColor.hsvSaturation() / 4, mutedCustomColor.value() );

	// Change the pure color by state: selected, muted, colored, normal
	if( isSelected() )
	{
		c = hasCustomColor()
			? ( muted
				? mutedCustomColor.darker( 350 )
				: clipColor.darker( 150 ) )
			: selectedColor();
	}
	else
	{
		if( muted )
		{
			c = hasCustomColor()
				? mutedCustomColor.darker( 250 )
				: mutedBackgroundColor();
		}
		else
		{
			c = clipColor;
		}
	}

	// Return color to caller
	return c;
}

auto ClipView::hasCustomColor() const -> bool
{
	return m_clip->color().has_value() || m_clip->getTrack()->color().has_value();
}

bool ClipView::splitClip(const TimePos pos)
{
	const TimePos splitPos = m_initialClipPos + pos;

	// Don't split if we slid off the Clip or if we're on the clip's start/end
	// Cutting at exactly the start/end position would create a zero length
	// clip (bad), and a clip the same length as the original one (pointless).
	if (splitPos <= m_initialClipPos || splitPos >= m_initialClipEnd) { return false; }

	m_clip->getTrack()->addJournalCheckPoint();
	m_clip->getTrack()->saveJournallingState(false);

	auto rightClip = m_clip->clone();

	m_clip->changeLength(splitPos - m_initialClipPos);
	m_clip->setAutoResize(false);

	rightClip->movePosition(splitPos);
	rightClip->changeLength(m_initialClipEnd - splitPos);
	rightClip->setStartTimeOffset(m_clip->startTimeOffset() - m_clip->length());
	rightClip->setAutoResize(false);

	m_clip->getTrack()->restoreJournallingState();
	return true;
}

} // namespace lmms::gui
