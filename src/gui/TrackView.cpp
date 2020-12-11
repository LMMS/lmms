/*
 * TrackView.cpp - implementation of TrackView class
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


#include "TrackView.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <QtGlobal>


#include "ConfigManager.h"
#include "DataFile.h"
#include "Engine.h"
#include "FadeButton.h"
#include "Mixer.h"
#include "PixmapButton.h"
#include "StringPairDrag.h"
#include "ToolTip.h"
#include "Track.h"
#include "TrackContainerView.h"
#include "TrackContentObjectView.h"


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

	connect(&m_track->m_mutedModel, SIGNAL(dataChanged()),
			this, SLOT(muteChanged()));

	connect( &m_track->m_soloModel, SIGNAL( dataChanged() ),
			m_track, SLOT( toggleSolo() ), Qt::DirectConnection );

	connect( &m_trackOperationsWidget, SIGNAL( colorChanged( QColor & ) ),
			m_track, SLOT( trackColorChanged( QColor & ) ) );

	connect( &m_trackOperationsWidget, SIGNAL( colorReset() ),
			m_track, SLOT( trackColorReset() ) );

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
	Q_ASSERT( m_track != NULL );
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




void TrackView::muteChanged()
{
	FadeButton * indicator = getActivityIndicator();
	if (indicator) { setIndicatorMute(indicator, m_track->m_mutedModel.value()); }
}




void TrackView::setIndicatorMute(FadeButton* indicator, bool muted)
{
	QPalette::ColorRole role = muted ? QPalette::Highlight : QPalette::BrightText;
	indicator->setActiveColor(QApplication::palette().color(QPalette::Active, role));
}
