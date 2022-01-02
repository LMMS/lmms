/*
 * SampleTrackView.cpp
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 
#include "SampleTrackView.h"

#include <QMenu>

#include "embed.h"
#include "Engine.h"
#include "MixerView.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "Knob.h"
#include "MainWindow.h"
#include "SampleTrackWindow.h"
#include "StringPairDrag.h"
#include "TrackContainerView.h"
#include "TrackLabelButton.h"


SampleTrackView::SampleTrackView( SampleTrack * _t, TrackContainerView* tcv ) :
	TrackView( _t, tcv )
{
	setFixedHeight( 32 );

	m_tlb = new TrackLabelButton(this, getTrackSettingsWidget());
	m_tlb->setCheckable(true);
	connect(m_tlb, SIGNAL(clicked( bool )),
			this, SLOT(showEffects()));
	m_tlb->setIcon(embed::getIconPixmap("sample_track"));
	m_tlb->move(3, 1);
	m_tlb->show();

	m_volumeKnob = new Knob( knobSmall_17, getTrackSettingsWidget(),
						    tr( "Track volume" ) );
	m_volumeKnob->setVolumeKnob( true );
	m_volumeKnob->setModel( &_t->m_volumeModel );
	m_volumeKnob->setHintText( tr( "Channel volume:" ), "%" );

	int settingsWidgetWidth = ConfigManager::inst()->
					value( "ui", "compacttrackbuttons" ).toInt()
				? DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT
				: DEFAULT_SETTINGS_WIDGET_WIDTH;
	m_volumeKnob->move( settingsWidgetWidth - 2 * 24, 2 );
	m_volumeKnob->setLabel( tr( "VOL" ) );
	m_volumeKnob->show();

	m_panningKnob = new Knob( knobSmall_17, getTrackSettingsWidget(),
							tr( "Panning" ) );
	m_panningKnob->setModel( &_t->m_panningModel );
	m_panningKnob->setHintText( tr( "Panning:" ), "%" );
	m_panningKnob->move( settingsWidgetWidth - 24, 2 );
	m_panningKnob->setLabel( tr( "PAN" ) );
	m_panningKnob->show();

	m_activityIndicator = new FadeButton(
		QApplication::palette().color(QPalette::Active, QPalette::Background),
		QApplication::palette().color(QPalette::Active, QPalette::BrightText),
		QApplication::palette().color(QPalette::Active, QPalette::BrightText).darker(),
		getTrackSettingsWidget()
	);
	m_activityIndicator->setGeometry(settingsWidgetWidth - 2 * 24 - 11, 2, 8, 28);
	m_activityIndicator->show();
	connect(_t, SIGNAL(playingChanged()), this, SLOT(updateIndicator()));

	setModel( _t );

	m_window = new SampleTrackWindow(this);
	m_window->toggleVisibility(false);
}




void SampleTrackView::updateIndicator()
{
	if (model()->isPlaying()) { m_activityIndicator->activateOnce(); }
	else { m_activityIndicator->noteEnd(); }
}




SampleTrackView::~SampleTrackView()
{
	if(m_window != nullptr)
	{
		m_window->setSampleTrackView(nullptr);
		m_window->parentWidget()->hide();
	}
	m_window = nullptr;
}



//FIXME: This is identical to InstrumentTrackView::createFxMenu
QMenu * SampleTrackView::createFxMenu(QString title, QString newFxLabel)
{
	int channelIndex = model()->effectChannelModel()->value();

	MixerChannel *mixerChannel = Engine::mixer()->effectChannel(channelIndex);

	// If title allows interpolation, pass channel index and name
	if (title.contains("%2"))
	{
		title = title.arg(channelIndex).arg(mixerChannel->m_name);
	}

	QMenu *fxMenu = new QMenu(title);

	fxMenu->addAction(newFxLabel, this, SLOT(createMixerLine()));
	fxMenu->addSeparator();

	for (int i = 0; i < Engine::mixer()->numChannels(); ++i)
	{
		MixerChannel * currentChannel = Engine::mixer()->effectChannel(i);

		if (currentChannel != mixerChannel)
		{
			const auto index = currentChannel->m_channelIndex;
			QString label = tr("FX %1: %2").arg(currentChannel->m_channelIndex).arg(currentChannel->m_name);
			fxMenu->addAction(label, [this, index](){
				assignMixerLine(index);
			});
		}
	}

	return fxMenu;
}




void SampleTrackView::showEffects()
{
	m_window->toggleVisibility(m_window->parentWidget()->isHidden());
}



void SampleTrackView::modelChanged()
{
	SampleTrack * st = castModel<SampleTrack>();
	m_volumeKnob->setModel(&st->m_volumeModel);

	TrackView::modelChanged();
}




void SampleTrackView::dragEnterEvent(QDragEnterEvent *dee)
{
	StringPairDrag::processDragEnterEvent(dee, QString("samplefile"));
}




void SampleTrackView::dropEvent(QDropEvent *de)
{
	QString type  = StringPairDrag::decodeKey(de);
	QString value = StringPairDrag::decodeValue(de);

	if (type == "samplefile")
	{
		int trackHeadWidth = ConfigManager::inst()->value("ui", "compacttrackbuttons").toInt()==1
				? DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT + TRACK_OP_WIDTH_COMPACT
				: DEFAULT_SETTINGS_WIDGET_WIDTH + TRACK_OP_WIDTH;

		int xPos = de->pos().x() < trackHeadWidth
				? trackHeadWidth
				: de->pos().x();

		TimePos tcoPos = trackContainerView()->fixedTCOs()
				? TimePos(0)
				: TimePos(((xPos - trackHeadWidth) / trackContainerView()->pixelsPerBar()
							* TimePos::ticksPerBar()) + trackContainerView()->currentPosition()
						).quantize(1.0);

		SampleTCO * sTco = static_cast<SampleTCO*>(getTrack()->createTCO(tcoPos));
		if (sTco) { sTco->setSampleFile(value); }
	}
}




/*! \brief Create and assign a new mixer Channel for this track */
void SampleTrackView::createMixerLine()
{
	int channelIndex = getGUI()->mixerView()->addNewChannel();
	auto channel = Engine::mixer()->effectChannel(channelIndex);

	channel->m_name = getTrack()->name();
	if (getTrack()->useColor()) { channel->setColor (getTrack()->color()); }

	assignMixerLine(channelIndex);
}




/*! \brief Assign a specific mixer Channel for this track */
void SampleTrackView::assignMixerLine(int channelIndex)
{
	model()->effectChannelModel()->setValue(channelIndex);

	getGUI()->mixerView()->setCurrentMixerLine(channelIndex);
}