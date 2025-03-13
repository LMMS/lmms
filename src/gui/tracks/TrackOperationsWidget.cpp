/*
 * TrackOperationsWidget.cpp - implementation of TrackOperationsWidget class
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

#include "TrackOperationsWidget.h"

#include <QBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QCheckBox>

#include "AutomationClip.h"
#include "AutomationTrackView.h"
#include "ColorChooser.h"
#include "ConfigManager.h"
#include "DataFile.h"
#include "embed.h"
#include "Engine.h"
#include "InstrumentTrackView.h"
#include "KeyboardShortcuts.h"
#include "PixmapButton.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "Track.h"
#include "TrackContainerView.h"
#include "TrackGrip.h"
#include "TrackView.h"

namespace lmms::gui
{

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
	setToolTip(tr("Press <%1> while clicking on move-grip "
				"to begin a new drag'n'drop action." ).arg(UI_CTRL_KEY) );

	auto toMenu = new QMenu(this);
	connect( toMenu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));


	setObjectName( "automationEnabled" );

	auto layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->setAlignment(Qt::AlignTop);

	m_trackGrip = new TrackGrip(m_trackView->getTrack(), this);
	layout->addWidget(m_trackGrip);

	// This widget holds the gear icon and the mute and solo
	// buttons in a layout.
	auto operationsWidget = new QWidget(this);
	auto operationsLayout = new QHBoxLayout(operationsWidget);
	operationsLayout->setContentsMargins(0, 3, 0, 0);
	operationsLayout->setSpacing(2);

	m_trackOps = new QPushButton(operationsWidget);
	m_trackOps->setFocusPolicy( Qt::NoFocus );
	m_trackOps->setMenu( toMenu );
	m_trackOps->setToolTip(tr("Actions"));

	// This helper lambda wraps a PixmapButton in a QWidget. This is necessary due to some strange effect where the
	// PixmapButtons are resized to a size that's larger than their minimum/fixed size when the method "show" is called
	// in "TrackContainerView::realignTracks". Specifically, with the default theme the buttons are resized from
	// (16, 14) to (26, 26). This then makes them behave not as expected in layouts.
	// The resizing is not done for QWidgets. Therefore we wrap the PixmapButton in a QWidget which is set to a
	// fixed size that will be able to show the active and inactive pixmap. We can then use the QWidget in layouts
	// without any disturbances.
	//
	// The resizing only seems to affect the track view hierarchy and is triggered by Qt's internal mechanisms.
	// For example the buttons in the mixer view do not seem to be affected.
	// If you want to debug this simply override "PixmapButton::resizeEvent" and trigger a break point in there.
	auto buildPixmapButtonWrappedInWidget = [](QWidget* parent, const QString& toolTip,
		std::string_view activeGraphic, std::string_view inactiveGraphic, PixmapButton*& pixmapButton)
	{
		const auto activePixmap = embed::getIconPixmap(activeGraphic);
		const auto inactivePixmap = embed::getIconPixmap(inactiveGraphic);

		auto necessarySize = activePixmap.size().expandedTo(inactivePixmap.size());

		auto wrapperWidget = new QWidget(parent);
		wrapperWidget->setFixedSize(necessarySize);

		auto button = new PixmapButton(wrapperWidget, toolTip);
		button->setCheckable(true);
		button->setActiveGraphic(activePixmap);
		button->setInactiveGraphic(inactivePixmap);
		button->setToolTip(toolTip);

		pixmapButton = button;

		return wrapperWidget;
	};

	auto muteWidget = buildPixmapButtonWrappedInWidget(operationsWidget, tr("Mute"), "mute_active", "mute_inactive", m_muteBtn);
	auto soloWidget = buildPixmapButtonWrappedInWidget(operationsWidget, tr("Solo"), "solo_active", "solo_inactive", m_soloBtn);

	operationsLayout->addWidget(m_trackOps);
	operationsLayout->addWidget(muteWidget);
	operationsLayout->addWidget(soloWidget);

	layout->addWidget(operationsWidget, 0, Qt::AlignTop | Qt::AlignLeading);

	connect( this, SIGNAL(trackRemovalScheduled(lmms::gui::TrackView*)),
			m_trackView->trackContainerView(),
				SLOT(deleteTrackView(lmms::gui::TrackView*)),
							Qt::QueuedConnection );

	connect( m_trackView->getTrack()->getMutedModel(), SIGNAL(dataChanged()),
			this, SLOT(update()));

	connect(m_trackView->getTrack(), SIGNAL(colorChanged()), this, SLOT(update()));
}


/*! \brief Respond to trackOperationsWidget mouse events
 *
 *  If it's the left mouse button, and Ctrl is held down, and we're
 *  not a Pattern Editor track, then start a new drag event to
 *  copy this track.
 *
 *  Otherwise, ignore all other events.
 *
 *  \param me The mouse event to respond to.
 */
void TrackOperationsWidget::mousePressEvent( QMouseEvent * me )
{
	if (me->button() == Qt::LeftButton && me->modifiers() & KBD_COPY_MODIFIER &&
		m_trackView->getTrack()->type() != Track::Type::Pattern)
	{
		DataFile dataFile( DataFile::Type::DragNDropData );
		m_trackView->getTrack()->saveState( dataFile, dataFile.content() );
		new StringPairDrag( QString( "track_%1" ).arg(
					static_cast<int>(m_trackView->getTrack()->type()) ),
			dataFile.toString(), m_trackView->getTrackSettingsWidget()->grab(),
									this );
	}
	else if( me->button() == Qt::LeftButton )
	{
		// track-widget (parent-widget) initiates track-move
		me->ignore();
	}
}


/*!
 * \brief Repaint the trackOperationsWidget
 *
 * Only things that's done for now is to paint the background
 * with the brush of the window from the palette.
 */
void TrackOperationsWidget::paintEvent(QPaintEvent*)
{
	QPainter p( this );

	p.fillRect(rect(), palette().brush(QPalette::Window));
}


/*! \brief Show a message box warning the user that this track is about to be closed */
bool TrackOperationsWidget::confirmRemoval()
{
	bool needConfirm = ConfigManager::inst()->value("ui", "trackdeletionwarning", "1").toInt();
	if (!needConfirm){ return true; }
	
	QString messageRemoveTrack = tr("After removing a track, it can not "
					"be recovered. Are you sure you want to remove track \"%1\"?")
					.arg(m_trackView->getTrack()->name());
	QString messageTitleRemoveTrack = tr("Confirm removal");
	QString askAgainText = tr("Don't ask again");
	auto askAgainCheckBox = new QCheckBox(askAgainText, nullptr);
	connect(askAgainCheckBox, &QCheckBox::stateChanged, [](int state){
		// Invert button state, if it's checked we *shouldn't* ask again
		ConfigManager::inst()->setValue("ui", "trackdeletionwarning", state ? "0" : "1");
	});

	QMessageBox mb;
	mb.setText(messageRemoveTrack);
	mb.setWindowTitle(messageTitleRemoveTrack);
	mb.setIcon(QMessageBox::Warning);
	mb.addButton(QMessageBox::Cancel);
	mb.addButton(QMessageBox::Ok);
	mb.setCheckBox(askAgainCheckBox);
	mb.setDefaultButton(QMessageBox::Cancel);

	return mb.exec() == QMessageBox::Ok;
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

	if (m_soloBtn->model()->value())
	{
		// if this track was solo, make the new track the new solo
		newTrack->toggleSolo();
	}
}


/*! \brief Clear this track - clears all Clips from the track */
void TrackOperationsWidget::clearTrack()
{
	Track * t = m_trackView->getTrack();
	t->addJournalCheckPoint();
	t->lock();
	t->deleteClips();
	t->unlock();
}


/*! \brief Remove this track from the track list
 *
 */
void TrackOperationsWidget::removeTrack()
{
	if (confirmRemoval())
	{
		emit trackRemovalScheduled(m_trackView);
	}
}

void TrackOperationsWidget::selectTrackColor()
{
	const auto newColor = ColorChooser{this}
		.withPalette(ColorChooser::Palette::Track)
		->getColor(m_trackView->getTrack()->color().value_or(Qt::white));

	if (!newColor.isValid()) { return; }

	const auto track = m_trackView->getTrack();
	track->addJournalCheckPoint();
	track->setColor(newColor);
	Engine::getSong()->setModified();
}

void TrackOperationsWidget::resetTrackColor()
{
	auto track = m_trackView->getTrack();
	track->addJournalCheckPoint();
	track->setColor(std::nullopt);
	Engine::getSong()->setModified();
}

void TrackOperationsWidget::randomizeTrackColor()
{
	QColor buffer = ColorChooser::getPalette( ColorChooser::Palette::Track )[ rand() % 48 ];
	auto track = m_trackView->getTrack();
	track->addJournalCheckPoint();
	track->setColor(buffer);
	Engine::getSong()->setModified();
}

void TrackOperationsWidget::resetClipColors()
{
	auto track = m_trackView->getTrack();
	track->addJournalCheckPoint();
	for (auto clip : track->getClips())
	{
		clip->setColor(std::nullopt);
	}
	Engine::getSong()->setModified();
}


/*! \brief Update the trackOperationsWidget context menu
 *
 *  For all track types, we have the Clone and Remove options.
 *  For instrument-tracks we also offer the MIDI-control-menu
 *  For automation tracks, extra options: turn on/off recording
 *  on all Clips (same should be added for sample tracks when
 *  sampletrack recording is implemented)
 */
void TrackOperationsWidget::updateMenu()
{
	QMenu * toMenu = m_trackOps->menu();
	toMenu->clear();
	toMenu->addAction( embed::getIconPixmap( "edit_copy", 16, 16 ),
						tr( "Clone this track" ),
						this, SLOT(cloneTrack()));
	toMenu->addAction( embed::getIconPixmap( "cancel", 16, 16 ),
						tr( "Remove this track" ),
						this, SLOT(removeTrack()));

	if( ! m_trackView->trackContainerView()->fixedClips() )
	{
		toMenu->addAction( tr( "Clear this track" ), this, SLOT(clearTrack()));
	}
	if (QMenu *mixerMenu = m_trackView->createMixerMenu(tr("Channel %1: %2"), tr("Assign to new Mixer Channel")))
	{
		toMenu->addMenu(mixerMenu);
	}

	if (auto trackView = dynamic_cast<InstrumentTrackView*>(m_trackView))
	{
		toMenu->addSeparator();
		toMenu->addMenu(trackView->midiMenu());
	}
	if( dynamic_cast<AutomationTrackView *>( m_trackView ) )
	{
		toMenu->addAction( tr( "Turn all recording on" ), this, SLOT(recordingOn()));
		toMenu->addAction( tr( "Turn all recording off" ), this, SLOT(recordingOff()));
	}

	toMenu->addSeparator();

	QMenu* colorMenu = toMenu->addMenu(tr("Track color"));
	colorMenu->setIcon(embed::getIconPixmap("colorize"));
	colorMenu->addAction(tr("Change"), this, SLOT(selectTrackColor()));
	colorMenu->addAction(tr("Reset"), this, SLOT(resetTrackColor()));
	colorMenu->addAction(tr("Pick random"), this, SLOT(randomizeTrackColor()));
	colorMenu->addSeparator();
	colorMenu->addAction(tr("Reset clip colors"), this, SLOT(resetClipColors()));
}


void TrackOperationsWidget::toggleRecording( bool on )
{
	auto atv = dynamic_cast<AutomationTrackView*>(m_trackView);
	if( atv )
	{
		for( Clip * clip : atv->getTrack()->getClips() )
		{
			auto ap = dynamic_cast<AutomationClip*>(clip);
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


} // namespace lmms::gui
