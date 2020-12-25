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

#include "AutomationPattern.h"
#include "BBEditor.h"
#include "BBTrackContainer.h"
#include "Clipboard.h"
#include "DataFile.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "Song.h"
#include "SongEditor.h"
#include "StringPairDrag.h"
#include "TrackContainerView.h"
#include "TrackContentObjectView.h"
#include "TrackView.h"


/*! Alternate between a darker and a lighter background color every 4 bars
 */
const int BARS_PER_GROUP = 4;


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
			SIGNAL( positionChanged( const TimePos & ) ),
			this, SLOT( changePosition( const TimePos & ) ) );

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
void TrackContentWidget::changePosition( const TimePos & newPos )
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

	TimePos pos = newPos;
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
						TimePos::ticksPerBar() ),
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
	TimePos tcoPos = getPosition( dee->pos().x() );
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
bool TrackContentWidget::canPasteSelection( TimePos tcoPos, const QDropEvent* de )
{
	const QMimeData * mimeData = de->mimeData();

	// If the source of the DropEvent is the current instance of LMMS we don't allow pasting in the same bar
	// if it's another instance of LMMS we allow it
	return de->source()
		? canPasteSelection( tcoPos, mimeData )
		: canPasteSelection( tcoPos, mimeData, true );
}

// Overloaded method to make it possible to call this method without a Drag&Drop event
bool TrackContentWidget::canPasteSelection( TimePos tcoPos, const QMimeData* md , bool allowSameBar )
{
	// For decodeKey() and decodeValue()
	using namespace Clipboard;

	Track * t = getTrack();
	QString type = decodeKey( md );
	QString value = decodeValue( md );

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
	TimePos grabbedTCOPos = tcoPosAttr.value().toInt();
	TimePos grabbedTCOBar = TimePos( grabbedTCOPos.getBar(), 0 );

	// Extract the track index that was originally clicked
	QDomAttr tiAttr = metadata.attributeNode( "initialTrackIndex" );
	const int initialTrackIndex = tiAttr.value().toInt();

	// Get the current track's index
	const TrackContainer::TrackList tracks = t->trackContainer()->tracks();
	const int currentTrackIndex = tracks.indexOf( t );

	// Don't paste if we're on the same bar and allowSameBar is false
	auto sourceTrackContainerId = metadata.attributeNode( "trackContainerId" ).value().toUInt();
	if( !allowSameBar && sourceTrackContainerId == t->trackContainer()->id() &&
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
bool TrackContentWidget::pasteSelection( TimePos tcoPos, QDropEvent * de )
{
	const QMimeData * mimeData = de->mimeData();

	if( canPasteSelection( tcoPos, de ) == false )
	{
		return false;
	}

	// We set skipSafetyCheck to true because we already called canPasteSelection
	return pasteSelection( tcoPos, mimeData, true );
}

// Overloaded method so we can call it without a Drag&Drop event
bool TrackContentWidget::pasteSelection( TimePos tcoPos, const QMimeData * md, bool skipSafetyCheck )
{
	// For decodeKey() and decodeValue()
	using namespace Clipboard;

	// When canPasteSelection was already called before, skipSafetyCheck will skip this
	if( !skipSafetyCheck && canPasteSelection( tcoPos, md ) == false )
	{
		return false;
	}

	QString type = decodeKey( md );
	QString value = decodeValue( md );

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
	TimePos grabbedTCOPos = tcoPosAttr.value().toInt();

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

	float snapSize = gui->songEditor()->m_editor->getSnapSize();
	// All patterns should be offset the same amount as the grabbed pattern
	TimePos offset = TimePos(tcoPos - grabbedTCOPos);
	// Users expect clips to "fall" backwards, so bias the offset
	offset -= TimePos::ticksPerBar() * snapSize / 2;
	// The offset is quantized (rather than the positions) to preserve fine adjustments
	offset = offset.quantize(snapSize);

	// Get the leftmost TCO and fix the offset if it reaches below bar 0
	TimePos leftmostPos = grabbedTCOPos;
	for(int i = 0; i < tcoNodes.length(); ++i)
	{
		QDomElement outerTCOElement = tcoNodes.item(i).toElement();
		QDomElement tcoElement = outerTCOElement.firstChildElement();

		TimePos pos = tcoElement.attributeNode("pos").value().toInt();

		if(pos < leftmostPos) { leftmostPos = pos; }
	}
	// Fix offset if it sets the left most TCO to a negative position
	offset = std::max(offset.getTicks(), -leftmostPos.getTicks());

	for( int i = 0; i<tcoNodes.length(); i++ )
	{
		QDomElement outerTCOElement = tcoNodes.item( i ).toElement();
		QDomElement tcoElement = outerTCOElement.firstChildElement();

		int trackIndex = outerTCOElement.attributeNode( "trackIndex" ).value().toInt();
		int finalTrackIndex = trackIndex + ( currentTrackIndex - initialTrackIndex );
		Track * t = tracks.at( finalTrackIndex );

		// The new position is the old position plus the offset.
		TimePos pos = tcoElement.attributeNode( "pos" ).value().toInt() + offset;
		// If we land on ourselves, offset by one snap
		TimePos shift = TimePos::ticksPerBar() * gui->songEditor()->m_editor->getSnapSize();
		if (offset == 0 && initialTrackIndex == currentTrackIndex) { pos += shift; }

		TrackContentObject * tco = t->createTCO( pos );
		tco->restoreState( tcoElement );
		tco->movePosition(pos); // Because we restored the state, we need to move the TCO again.
		if( wasSelection )
		{
			tco->selectViewOnCreate( true );
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
	TimePos tcoPos = TimePos( getPosition( de->pos().x() ) );
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
		const TimePos pos = getPosition( me->x() ).getBar() *
						TimePos::ticksPerBar();
		getTrack()->createTCO(pos);
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

	// If we don't have TCO data in the clipboard there's no need to create this menu
	// since "paste" is the only action at the moment.
	if( ! hasFormat( MimeType::StringPair )  )
	{
		return;
	}

	QMenu contextMenu( this );
	QAction *pasteA = contextMenu.addAction( embed::getIconPixmap( "edit_paste" ),
					tr( "Paste" ), [this, cme](){ contextMenuAction( cme, Paste ); } );
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
		case Paste:
		// Paste the selection on the TimePos of the context menu event
		TimePos tcoPos = getPosition( cme->x() );

		pasteSelection( tcoPos, getMimeData() );
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

