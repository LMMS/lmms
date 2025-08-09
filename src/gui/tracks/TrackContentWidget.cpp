/*
 * TrackContentWidget.cpp - implementation of TrackContentWidget class
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

#include "TrackContentWidget.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPainter>

#include "AutomationClip.h"
#include "Clipboard.h"
#include "DataFile.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "PatternEditor.h"
#include "PatternStore.h"
#include "Song.h"
#include "SongEditor.h"
#include "StringPairDrag.h"
#include "TrackContainerView.h"
#include "ClipView.h"
#include "TrackView.h"

namespace lmms::gui
{

/*! Alternate between a darker and a lighter background color every 4 bars
 */
const int BARS_PER_GROUP = 4;
/* Lines between bars will disappear if zoomed too far out (i.e
	if there are less than 4 pixels between lines)*/
const int MIN_PIXELS_BETWEEN_LINES = 4;

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
	m_coarseGridColor( Qt::SolidPattern ),
	m_fineGridColor( Qt::SolidPattern ),
	m_horizontalColor( Qt::SolidPattern ),
	m_embossColor( Qt::SolidPattern ),
	m_coarseGridWidth(2),
	m_fineGridWidth(1),
	m_horizontalWidth(1),
	m_embossWidth(0),
	m_embossOffset(0)
{
	setAcceptDrops( true );

	connect( parent->trackContainerView(),
			SIGNAL( positionChanged( const lmms::TimePos& ) ),
			this, SLOT( changePosition( const lmms::TimePos& ) ) );

	// Update background if snap size changes
	connect(getGUI()->songEditor()->m_editor->snappingModel(), &Model::dataChanged,
			this, &TrackContentWidget::updateBackground);

	// Also update background if proportional snap is enabled/disabled
	connect(getGUI()->songEditor()->m_editor, &SongEditor::proportionalSnapChanged,
			this, &TrackContentWidget::updateBackground);

	setStyle( QApplication::style() );

	updateBackground();
}




void TrackContentWidget::updateBackground()
{		
	// use snapSize to determine number of lines to draw
	float snapSize = getGUI()->songEditor()->m_editor->getSnapSize();

	const TrackContainerView * tcv = m_trackView->trackContainerView();

	// Assume even-pixels-per-bar. Makes sense, should be like this anyways
	int ppb = static_cast<int>( tcv->pixelsPerBar() );

	// Coarse grid appears every bar (less frequently if quantization > 1 bar)
	float coarseGridResolution = (snapSize >= 1) ? snapSize : 1;
	// Fine grid appears within bars
	float fineGridResolution = snapSize;
	// Increase fine grid resolution (size between lines) if it results in less than	
	// 4 pixels between each line to avoid cluttering
	float pixelsBetweenLines = ppb * snapSize;
	if (pixelsBetweenLines < MIN_PIXELS_BETWEEN_LINES) {
		// Scale fineGridResolution so that there are enough pixels between lines
		// scaleFactor should be a power of 2
		int scaleFactor = 1 << static_cast<int>( std::ceil( std::log2( MIN_PIXELS_BETWEEN_LINES / pixelsBetweenLines ) ) );
		fineGridResolution *= scaleFactor;
	}

	int w = ppb * BARS_PER_GROUP;
	int h = height();
	m_background = QPixmap( w * 2, height() );
	QPainter pmp( &m_background );

	pmp.fillRect( 0, 0, w, h, darkerColor() );
	pmp.fillRect( w, 0, w , h, lighterColor() );

	// draw lines
	// draw fine grid
	pmp.setPen( QPen( fineGridColor(), fineGridWidth() ) );
	for (float x = 0; x < w * 2; x += ppb * fineGridResolution)
	{
		pmp.drawLine( QLineF( x, 0.0, x, h ) );
	}

	// draw coarse grid
	pmp.setPen( QPen( coarseGridColor(), coarseGridWidth() ) );
	for (float x = 0; x <= w * 2; x += ppb * coarseGridResolution)
	{
		pmp.drawLine( QLineF( x, 0.0, x, h ) );
	}

	pmp.setPen( QPen( embossColor(), embossWidth() ) );
	for (float x = (coarseGridWidth() + embossOffset()); x < w * 2; x += ppb * coarseGridResolution)
	{
		pmp.drawLine( QLineF( x, 0.0, x, h ) );
	}

	// draw horizontal line
	pmp.setPen( QPen( horizontalColor(), horizontalWidth() ) );
	pmp.drawLine(0, h - (horizontalWidth() + 1) / 2, w * 2, h - (horizontalWidth() + 1) / 2);

	pmp.end();

	// Force redraw
	update();
}




/*! \brief Adds a ClipView to this widget.
 *
 *  Adds a(nother) ClipView to our list of views.  We also
 *  check that our position is up-to-date.
 *
 * \param clipv The ClipView to add.
 */
void TrackContentWidget::addClipView( ClipView * clipv )
{
	Clip * clip = clipv->getClip();

	m_clipViews.push_back( clipv );

	clip->saveJournallingState( false );
	changePosition();
	clip->restoreJournallingState();
}




/*! \brief Removes the given ClipView from this widget.
 *
 *  Removes the given ClipView from our list of views.
 *
 * \param clipv The ClipView to add.
 */
void TrackContentWidget::removeClipView( ClipView * clipv )
{
	clipViewVector::iterator it = std::find( m_clipViews.begin(),
						m_clipViews.end(),
						clipv );
	if( it != m_clipViews.end() )
	{
		m_clipViews.erase( it );
		Engine::getSong()->setModified();
	}
}




/*! \brief Update ourselves by updating all the ClipViews attached.
 *
 */
void TrackContentWidget::update()
{
	for (const auto& clipView : m_clipViews)
	{
		clipView->setFixedHeight(height() - 1);
		clipView->update();
	}
	QWidget::update();
}




// resposible for moving track-content-widgets to appropriate position after
// change of visible viewport
/*! \brief Move the trackContentWidget to a new place in time
 *
 * \param newPos The MIDI time to move to.
 */
void TrackContentWidget::changePosition( const TimePos & newPos )
{
	if (m_trackView->trackContainerView() == getGUI()->patternEditor()->m_editor)
	{
		const int curPattern = Engine::patternStore()->currentPattern();
		setUpdatesEnabled( false );

		// first show clip for current pattern...
		for (const auto& clipView : m_clipViews)
		{
			if (clipView->getClip()->startPosition().getBar() == curPattern)
			{
				clipView->move(0, clipView->y());
				clipView->raise();
				clipView->show();
			}
			else { clipView->lower(); }
		}
		// ...then hide others to avoid flickering
		for (const auto& clipView : m_clipViews)
		{
			if (clipView->getClip()->startPosition().getBar() != curPattern) { clipView->hide(); }
		}
		setUpdatesEnabled( true );
		return;
	}

	TimePos pos = newPos;
	if( pos < 0 )
	{
		pos = m_trackView->trackContainerView()->currentPosition();
	}

	const int begin = pos;
	const int end = endPosition( pos );
	const float ppb = m_trackView->trackContainerView()->pixelsPerBar();

	setUpdatesEnabled( false );
	for (const auto& clipView : m_clipViews)
	{
		Clip* clip = clipView->getClip();

		clip->changeLength( clip->length() );

		const int ts = clip->startPosition();
		const int te = clip->endPosition()-3;
		if( ( ts >= begin && ts <= end ) ||
			( te >= begin && te <= end ) ||
			( ts <= begin && te >= end ) )
		{
			clipView->move(static_cast<int>((ts - begin) * ppb / TimePos::ticksPerBar()), clipView->y());
			if (!clipView->isVisible())
			{
				clipView->show();
			}
		}
		else
		{
			clipView->move(-clipView->width() - 10, clipView->y());
		}
	}
	setUpdatesEnabled( true );

	// redraw background
	updateBackground();
//	update();
}




/*! \brief Return the position of the trackContentWidget in bars.
 *
 * \param mouseX the mouse's current X position in pixels.
 */
TimePos TrackContentWidget::getPosition( int mouseX )
{
	TrackContainerView * tv = m_trackView->trackContainerView();
	return TimePos( tv->currentPosition() +
					 mouseX *
					 TimePos::ticksPerBar() /
					 static_cast<int>( tv->pixelsPerBar() ) );
}




/*! \brief Respond to a drag enter event on the trackContentWidget
 *
 * \param dee the Drag Enter Event to respond to
 */
void TrackContentWidget::dragEnterEvent( QDragEnterEvent * dee )
{
	TimePos clipPos = getPosition( dee->pos().x() );
	if( canPasteSelection( clipPos, dee ) == false )
	{
		dee->ignore();
	}
	else
	{
		StringPairDrag::processDragEnterEvent( dee, "clip_" +
						QString::number( static_cast<int>(getTrack()->type()) ) );
	}
}




/*! \brief Returns whether a selection of Clips can be pasted into this
 *
 * \param clipPos the position of the Clip slot being pasted on
 * \param de the DropEvent generated
 */
bool TrackContentWidget::canPasteSelection( TimePos clipPos, const QDropEvent* de )
{
	const QMimeData * mimeData = de->mimeData();

	// If the source of the DropEvent is the current instance of LMMS we don't allow pasting in the same bar
	// if it's another instance of LMMS we allow it
	return de->source()
		? canPasteSelection( clipPos, mimeData )
		: canPasteSelection( clipPos, mimeData, true );
}

// Overloaded method to make it possible to call this method without a Drag&Drop event
bool TrackContentWidget::canPasteSelection( TimePos clipPos, const QMimeData* md , bool allowSameBar )
{
	// For decodeKey() and decodeValue()
	using namespace Clipboard;

	Track * t = getTrack();
	QString type = decodeKey( md );
	QString value = decodeValue( md );

	// We can only paste into tracks of the same type
	if (type != ("clip_" + QString::number(static_cast<int>(t->type()))))
	{
		return false;
	}

	// value contains XML needed to reconstruct Clips and place them
	DataFile dataFile( value.toUtf8() );

	// Extract the metadata and which Clip was grabbed
	QDomElement metadata = dataFile.content().firstChildElement( "copyMetadata" );
	QDomAttr clipPosAttr = metadata.attributeNode( "grabbedClipPos" );
	TimePos grabbedClipPos = clipPosAttr.value().toInt();
	TimePos grabbedClipBar = TimePos( grabbedClipPos.getBar(), 0 );

	// Extract the track index that was originally clicked
	QDomAttr tiAttr = metadata.attributeNode( "initialTrackIndex" );
	const int initialTrackIndex = tiAttr.value().toInt();

	// Get the current track's index
	const TrackContainer::TrackList& tracks = t->trackContainer()->tracks();
	const auto currentTrackIt = std::find(tracks.begin(), tracks.end(), t);
	const int currentTrackIndex = currentTrackIt != tracks.end() ? std::distance(tracks.begin(), currentTrackIt) : -1;

	// Don't paste if we're on the same bar and allowSameBar is false
	auto sourceTrackContainerId = metadata.attributeNode( "trackContainerId" ).value().toUInt();
	if( !allowSameBar && sourceTrackContainerId == t->trackContainer()->id() &&
			clipPos == grabbedClipBar && currentTrackIndex == initialTrackIndex )
	{
		return false;
	}

	// Extract the clip data
	QDomElement clipParent = dataFile.content().firstChildElement("clips");
	QDomNodeList clipNodes = clipParent.childNodes();

	// If we are pasting into the PatternEditor, only a single Clip is allowed to be pasted
	// so we don't have the unexpected behavior of pasting on different PatternTracks
	if (m_trackView->trackContainerView()->fixedClips() == true &&
			clipNodes.length() > 1)
	{
		return false;
	}

	// Determine if all the Clips will land on a valid track
	for( int i = 0; i < clipNodes.length(); i++ )
	{
		QDomElement clipElement = clipNodes.item( i ).toElement();
		int trackIndex = clipElement.attributeNode( "trackIndex" ).value().toInt();
		int finalTrackIndex = trackIndex + currentTrackIndex - initialTrackIndex;

		// Track must be in TrackContainer's tracks
		if (finalTrackIndex < 0 || static_cast<std::size_t>(finalTrackIndex) >= tracks.size())
		{
			return false;
		}

		// Track must be of the same type
		auto startTrackType = static_cast<Track::Type>(clipElement.attributeNode("trackType").value().toInt());
		Track * endTrack = tracks.at( finalTrackIndex );
		if( startTrackType != endTrack->type() )
		{
			return false;
		}
	}

	return true;
}

/*! \brief Pastes a selection of Clips onto the track
 *
 * \param clipPos the position of the Clip slot being pasted on
 * \param de the DropEvent generated
 */
bool TrackContentWidget::pasteSelection( TimePos clipPos, QDropEvent * de )
{
	const QMimeData * mimeData = de->mimeData();

	if( canPasteSelection( clipPos, de ) == false )
	{
		return false;
	}

	// We set skipSafetyCheck to true because we already called canPasteSelection
	return pasteSelection( clipPos, mimeData, true );
}

// Overloaded method so we can call it without a Drag&Drop event
bool TrackContentWidget::pasteSelection( TimePos clipPos, const QMimeData * md, bool skipSafetyCheck )
{
	// For decodeKey() and decodeValue()
	using namespace Clipboard;

	// When canPasteSelection was already called before, skipSafetyCheck will skip this
	if( !skipSafetyCheck && canPasteSelection( clipPos, md ) == false )
	{
		return false;
	}

	QString type = decodeKey( md );
	QString value = decodeValue( md );

	getTrack()->addJournalCheckPoint();

	// value contains XML needed to reconstruct Clips and place them
	DataFile dataFile( value.toUtf8() );

	// Extract the clip data
	QDomElement clipParent = dataFile.content().firstChildElement("clips");
	QDomNodeList clipNodes = clipParent.childNodes();

	// Extract the track index that was originally clicked
	QDomElement metadata = dataFile.content().firstChildElement( "copyMetadata" );
	QDomAttr tiAttr = metadata.attributeNode( "initialTrackIndex" );
	int initialTrackIndex = tiAttr.value().toInt();
	QDomAttr clipPosAttr = metadata.attributeNode( "grabbedClipPos" );
	TimePos grabbedClipPos = clipPosAttr.value().toInt();

	// Snap the mouse position to the beginning of the dropped bar, in ticks
	const TrackContainer::TrackList& tracks = getTrack()->trackContainer()->tracks();
	const auto currentTrackIt = std::find(tracks.begin(), tracks.end(), getTrack());
	const int currentTrackIndex = currentTrackIt != tracks.end() ? std::distance(tracks.begin(), currentTrackIt) : -1;

	bool wasSelection = m_trackView->trackContainerView()->rubberBand()->selectedObjects().count();

	// Unselect the old group
		const QVector<selectableObject *> so =
			m_trackView->trackContainerView()->selectedObjects();
		for (const auto& obj : so)
		{
			obj->setSelected(false);
		}


	// TODO -- Need to draw the hovericon either way, or ghost the Clips
	// onto their final position.

	float snapSize = getGUI()->songEditor()->m_editor->getSnapSize();
	// All clips should be offset the same amount as the grabbed clip
	auto offset = TimePos(clipPos - grabbedClipPos);
	// Users expect clips to "fall" backwards, so bias the offset
	offset -= TimePos::ticksPerBar() * snapSize / 2;
	// The offset is quantized (rather than the positions) to preserve fine adjustments
	offset = offset.quantize(snapSize);

	// Get the leftmost Clip and fix the offset if it reaches below bar 0
	TimePos leftmostPos = grabbedClipPos;
	for(int i = 0; i < clipNodes.length(); ++i)
	{
		QDomElement outerClipElement = clipNodes.item(i).toElement();
		QDomElement clipElement = outerClipElement.firstChildElement();

		TimePos pos = clipElement.attributeNode("pos").value().toInt();

		if(pos < leftmostPos) { leftmostPos = pos; }
	}
	// Fix offset if it sets the left most Clip to a negative position
	offset = std::max(offset.getTicks(), -leftmostPos.getTicks());

	for( int i = 0; i<clipNodes.length(); i++ )
	{
		QDomElement outerClipElement = clipNodes.item( i ).toElement();
		QDomElement clipElement = outerClipElement.firstChildElement();

		int trackIndex = outerClipElement.attributeNode( "trackIndex" ).value().toInt();
		int finalTrackIndex = trackIndex + ( currentTrackIndex - initialTrackIndex );
		Track * t = tracks.at( finalTrackIndex );

		// The new position is the old position plus the offset.
		TimePos pos = clipElement.attributeNode( "pos" ).value().toInt() + offset;
		// If we land on ourselves, offset by one snap
		TimePos shift = TimePos::ticksPerBar() * getGUI()->songEditor()->m_editor->getSnapSize();
		if (offset == 0 && initialTrackIndex == currentTrackIndex) { pos += shift; }

		Clip * clip = t->createClip( pos );
		clip->restoreState( clipElement );
		clip->movePosition(pos); // Because we restored the state, we need to move the Clip again.
		if( wasSelection )
		{
			clip->selectViewOnCreate( true );
		}
	}

	AutomationClip::resolveAllIDs();

	return true;
}


/*! \brief Respond to a drop event on the trackContentWidget
 *
 * \param de the Drop Event to respond to
 */
void TrackContentWidget::dropEvent( QDropEvent * de )
{
	TimePos clipPos = TimePos( getPosition( de->pos().x() ) );
	if( pasteSelection( clipPos, de ) == true )
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
	// Enable box select if control is held when clicking an empty space
	// (If we had clicked a Clip it would have intercepted the mouse event)
	if( me->modifiers() & Qt::ControlModifier ){
		getGUI()->songEditor()->m_editor->setEditMode(SongEditor::EditMode::Select);
	}
	// Forward event to allow box select if the editor supports it and is in that mode
	if( m_trackView->trackContainerView()->allowRubberband() == true )
	{
		QWidget::mousePressEvent( me );
	}
	// Forward shift clicks so tracks can be resized
	else if( me->modifiers() & Qt::ShiftModifier )
	{
		QWidget::mousePressEvent( me );
	}
	// For an unmodified click, create a new Clip
	else if( me->button() == Qt::LeftButton &&
			!m_trackView->trackContainerView()->fixedClips() )
	{
		QVector<selectableObject*> so =  m_trackView->trackContainerView()->rubberBand()->selectedObjects();
		for( int i = 0; i < so.count(); ++i )
		{
			so.at( i )->setSelected( false);
		}
		getTrack()->addJournalCheckPoint();
		const float snapSize = getGUI()->songEditor()->m_editor->getSnapSize();
		const TimePos pos = TimePos(getPosition(me->x())).quantize(snapSize, true);
		getTrack()->createClip(pos);
	}
}




void TrackContentWidget::mouseReleaseEvent( QMouseEvent * me )
{
	getGUI()->songEditor()->syncEditMode();
	QWidget::mouseReleaseEvent(me);
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
	// Don't draw background on Pattern Editor
	if (m_trackView->trackContainerView() != getGUI()->patternEditor()->m_editor)
	{
		p.drawTiledPixmap(rect(), m_background, QPoint(
				tcv->currentPosition().getTicks() * ppb / TimePos::ticksPerBar(), 0));
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
TimePos TrackContentWidget::endPosition( const TimePos & posStart )
{
	const float ppb = m_trackView->trackContainerView()->pixelsPerBar();
	const int w = width();
	return posStart + static_cast<int>( w * TimePos::ticksPerBar() / ppb );
}

void TrackContentWidget::contextMenuEvent( QContextMenuEvent * cme )
{
	// For hasFormat(), MimeType enum class and getMimeData()
	using namespace Clipboard;

	if( cme->modifiers() )
	{
		return;
	}

	// If we don't have Clip data in the clipboard there's no need to create this menu
	// since "paste" is the only action at the moment.
	if( ! hasFormat( MimeType::StringPair )  )
	{
		return;
	}

	QMenu contextMenu( this );
	QAction *pasteA = contextMenu.addAction( embed::getIconPixmap( "edit_paste" ),
					tr( "Paste" ), [this, cme](){ contextMenuAction( cme, ContextMenuAction::Paste ); } );
	// If we can't paste in the current TCW for some reason, disable the action so the user knows
	pasteA->setEnabled( canPasteSelection( getPosition( cme->x() ), getMimeData() ) ? true : false );

	contextMenu.exec( QCursor::pos() );
}

void TrackContentWidget::contextMenuAction( QContextMenuEvent * cme, ContextMenuAction action )
{
	// For getMimeData()
	using namespace Clipboard;

	switch( action )
	{
		case ContextMenuAction::Paste:
		// Paste the selection on the TimePos of the context menu event
		TimePos clipPos = getPosition( cme->x() );

		pasteSelection( clipPos, getMimeData() );
		break;
	}
}



// qproperty access methods
//! \brief CSS theming qproperty access method
QBrush TrackContentWidget::darkerColor() const
{ return m_darkerColor; }

//! \brief CSS theming qproperty access method
QBrush TrackContentWidget::lighterColor() const
{ return m_lighterColor; }

//! \brief CSS theming qproperty access method
QBrush TrackContentWidget::coarseGridColor() const
{ return m_coarseGridColor; }

//! \brief CSS theming qproperty access method
QBrush TrackContentWidget::fineGridColor() const
{ return m_fineGridColor; }

//! \brief CSS theming qproperty access method
QBrush TrackContentWidget::horizontalColor() const
{ return m_horizontalColor; }

//! \brief CSS theming qproperty access method
QBrush TrackContentWidget::embossColor() const
{ return m_embossColor; }

//! \brief CSS theming qproperty access method
int TrackContentWidget::coarseGridWidth() const
{ return m_coarseGridWidth; }

//! \brief CSS theming qproperty access method
int TrackContentWidget::fineGridWidth() const
{ return m_fineGridWidth; }

//! \brief CSS theming qproperty access method
int TrackContentWidget::horizontalWidth() const
{ return m_horizontalWidth; }

//! \brief CSS theming qproperty access method
int TrackContentWidget::embossWidth() const
{ return m_embossWidth; }

//! \brief CSS theming qproperty access method
int TrackContentWidget::embossOffset() const
{ return m_embossOffset; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setDarkerColor( const QBrush & c )
{ m_darkerColor = c; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setLighterColor( const QBrush & c )
{ m_lighterColor = c; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setCoarseGridColor( const QBrush & c )
{ m_coarseGridColor = c; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setFineGridColor( const QBrush & c )
{ m_fineGridColor = c; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setHorizontalColor( const QBrush & c )
{ m_horizontalColor = c; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setEmbossColor( const QBrush & c )
{ m_embossColor = c; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setCoarseGridWidth(int c)
{ m_coarseGridWidth = c; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setFineGridWidth(int c)
{ m_fineGridWidth = c; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setHorizontalWidth(int c)
{ m_horizontalWidth = c; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setEmbossWidth(int c)
{ m_embossWidth = c; }

//! \brief CSS theming qproperty access method
void TrackContentWidget::setEmbossOffset(int c)
{ m_embossOffset = c; }

} // namespace lmms::gui
