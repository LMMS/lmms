/*
 * InstrumentTrackView.cpp - implementation of InstrumentTrackView class
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

#include "InstrumentTrackView.h"

#include <QAction>
#include <QApplication>
#include <QDragEnterEvent>
#include <QHBoxLayout>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QSpacerItem>
#include <QVBoxLayout>

#include "AudioEngine.h"
#include "ConfigManager.h"
#include "Engine.h"
#include "FadeButton.h"
#include "GuiApplication.h"
#include "Instrument.h"
#include "InstrumentTrackWindow.h"
#include "Knob.h"
#include "MainWindow.h"
#include "MidiClient.h"
#include "MidiCCRackView.h"
#include "MidiPortMenu.h"
#include "Mixer.h"
#include "MixerChannelLcdSpinBox.h"
#include "MixerView.h"
#include "TrackLabelButton.h"


namespace lmms::gui
{


InstrumentTrackView::InstrumentTrackView( InstrumentTrack * _it, TrackContainerView* tcv ) :
	TrackView( _it, tcv ),
	m_window( nullptr ),
	m_lastPos( -1, -1 )
{
	setAcceptDrops( true );
	setFixedHeight( 32 );

	m_tlb = new TrackLabelButton( this, getTrackSettingsWidget() );
	m_tlb->setCheckable( true );
	m_tlb->setIcon(determinePixmap(_it));
	m_tlb->show();

	connect( m_tlb, SIGNAL(toggled(bool)),
			this, SLOT(toggleInstrumentWindow(bool)));

	connect( _it, SIGNAL(nameChanged()),
			m_tlb, SLOT(update()));

	connect(ConfigManager::inst(), SIGNAL(valueChanged(QString,QString,QString)),
			this, SLOT(handleConfigChange(QString,QString,QString)));

	m_mixerChannelNumber = new MixerChannelLcdSpinBox(2, getTrackSettingsWidget(), tr("Mixer channel"), this);
	m_mixerChannelNumber->show();

	m_volumeKnob = new Knob(KnobType::Small17, tr("VOL"), getTrackSettingsWidget(), Knob::LabelRendering::LegacyFixedFontSize, tr("VOL"));
	m_volumeKnob->setVolumeKnob( true );
	m_volumeKnob->setModel( &_it->m_volumeModel );
	m_volumeKnob->setHintText( tr( "Volume:" ), "%" );
	m_volumeKnob->show();

	m_panningKnob = new Knob(KnobType::Small17, tr("PAN"), getTrackSettingsWidget(), Knob::LabelRendering::LegacyFixedFontSize, tr("Panning"));
	m_panningKnob->setModel( &_it->m_panningModel );
	m_panningKnob->setHintText(tr("Panning:"), "%");
	m_panningKnob->show();

	m_midiMenu = new QMenu( tr( "MIDI" ), this );

	// sequenced MIDI?
	if( !Engine::audioEngine()->midiClient()->isRaw() )
	{
		_it->m_midiPort.m_readablePortsMenu = new MidiPortMenu(
							MidiPort::Mode::Input );
		_it->m_midiPort.m_writablePortsMenu = new MidiPortMenu(
							MidiPort::Mode::Output );
		_it->m_midiPort.m_readablePortsMenu->setModel(
							&_it->m_midiPort );
		_it->m_midiPort.m_writablePortsMenu->setModel(
							&_it->m_midiPort );
		m_midiInputAction = m_midiMenu->addMenu(
					_it->m_midiPort.m_readablePortsMenu );
		m_midiOutputAction = m_midiMenu->addMenu(
					_it->m_midiPort.m_writablePortsMenu );
	}
	else
	{
		m_midiInputAction = m_midiMenu->addAction( "" );
		m_midiOutputAction = m_midiMenu->addAction( "" );
		m_midiInputAction->setCheckable( true );
		m_midiOutputAction->setCheckable( true );
		connect( m_midiInputAction, SIGNAL(changed()), this,
						SLOT(midiInSelected()));
		connect( m_midiOutputAction, SIGNAL(changed()), this,
					SLOT(midiOutSelected()));
		connect( &_it->m_midiPort, SIGNAL(modeChanged()),
				this, SLOT(midiConfigChanged()));
	}

	m_midiInputAction->setText( tr( "Input" ) );
	m_midiOutputAction->setText( tr( "Output" ) );

	QAction *midiRackAction = m_midiMenu->addAction(tr("Open/Close MIDI CC Rack"));
	midiRackAction->setIcon(embed::getIconPixmap("midi_cc_rack"));
	connect(midiRackAction, SIGNAL(triggered()),
		this, SLOT(toggleMidiCCRack()));

	m_activityIndicator = new FadeButton( QApplication::palette().color( QPalette::Active,
							QPalette::Window),
						QApplication::palette().color( QPalette::Active,
							QPalette::BrightText ),
						QApplication::palette().color( QPalette::Active,
							QPalette::BrightText).darker(),
						getTrackSettingsWidget() );
	m_activityIndicator->setFixedSize(8, 28);
	m_activityIndicator->show();

	auto masterLayout = new QVBoxLayout(getTrackSettingsWidget());
	masterLayout->setContentsMargins(0, 1, 0, 0);
	auto layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(m_tlb);
	layout->addWidget(m_mixerChannelNumber);
	layout->addWidget(m_activityIndicator);
	layout->addWidget(m_volumeKnob);
	layout->addWidget(m_panningKnob);
	masterLayout->addLayout(layout);
	masterLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

	connect( m_activityIndicator, SIGNAL(pressed()),
				this, SLOT(activityIndicatorPressed()));
	connect( m_activityIndicator, SIGNAL(released()),
				this, SLOT(activityIndicatorReleased()));
	connect( _it, SIGNAL(newNote()),
			 m_activityIndicator, SLOT(activate()));
	connect( _it, SIGNAL(endNote()),
	 		m_activityIndicator, SLOT(noteEnd()));

	setModel( _it );
}




InstrumentTrackView::~InstrumentTrackView()
{
	delete m_window;
	m_window = nullptr;

	delete model()->m_midiPort.m_readablePortsMenu;
	delete model()->m_midiPort.m_writablePortsMenu;
}




void InstrumentTrackView::toggleMidiCCRack()
{
	// Lazy creation: midiCCRackView is only created when accessed the first time.
	// this->model() returns pointer to the InstrumentTrack who owns this InstrumentTrackView.
	if (!m_midiCCRackView)
	{
		m_midiCCRackView = std::unique_ptr<MidiCCRackView>(new MidiCCRackView(this->model()));
	}

	if (m_midiCCRackView->parentWidget()->isVisible())
	{
		m_midiCCRackView->parentWidget()->hide();
	}
	else
	{
		m_midiCCRackView->parentWidget()->show();
		m_midiCCRackView->show();
	}
}




InstrumentTrackWindow * InstrumentTrackView::topLevelInstrumentTrackWindow()
{
	InstrumentTrackWindow * w = nullptr;
	for( const QMdiSubWindow * sw :
				getGUI()->mainWindow()->workspace()->subWindowList(
											QMdiArea::ActivationHistoryOrder ) )
	{
		if( sw->isVisible() && sw->widget()->inherits( "lmms::gui::InstrumentTrackWindow" ) )
		{
			w = qobject_cast<InstrumentTrackWindow *>( sw->widget() );
		}
	}

	return w;
}




/*! \brief Create and assign a new mixer Channel for this track */
void InstrumentTrackView::createMixerLine()
{
	int channelIndex = getGUI()->mixerView()->addNewChannel();
	auto channel = Engine::mixer()->mixerChannel(channelIndex);

	channel->m_name = getTrack()->name();
	channel->setColor(getTrack()->color());

	assignMixerLine(channelIndex);
}




/*! \brief Assign a specific mixer Channel for this track */
void InstrumentTrackView::assignMixerLine(int channelIndex)
{
	model()->mixerChannelModel()->setValue( channelIndex );

	getGUI()->mixerView()->setCurrentMixerChannel(channelIndex);
}



InstrumentTrackWindow * InstrumentTrackView::getInstrumentTrackWindow()
{
	if (!m_window)
	{
		m_window = new InstrumentTrackWindow(this);
	}

	return m_window;
}

void InstrumentTrackView::handleConfigChange(QString cls, QString attr, QString value)
{
	// When one instrument track window mode is turned on,
	// close windows except last opened one.
	if (cls == "ui" && attr == "oneinstrumenttrackwindow" && value.toInt())
	{
		m_tlb->setChecked(m_window && m_window == topLevelInstrumentTrackWindow());
	}
}

void InstrumentTrackView::modelChanged()
{
	TrackView::modelChanged();
	auto st = castModel<InstrumentTrack>();
	m_mixerChannelNumber->setModel(&st->m_mixerChannelModel);
}

void InstrumentTrackView::dragEnterEvent( QDragEnterEvent * _dee )
{
	InstrumentTrackWindow::dragEnterEventGeneric( _dee );
	if( !_dee->isAccepted() )
	{
		TrackView::dragEnterEvent( _dee );
	}
}




void InstrumentTrackView::dropEvent( QDropEvent * _de )
{
	getInstrumentTrackWindow()->dropEvent( _de );
	TrackView::dropEvent( _de );
}




void InstrumentTrackView::toggleInstrumentWindow( bool _on )
{
	if (_on && ConfigManager::inst()->value("ui", "oneinstrumenttrackwindow").toInt())
	{
		if (topLevelInstrumentTrackWindow())
		{
			topLevelInstrumentTrackWindow()->m_itv->m_tlb->setChecked(false);
		}
	}

	getInstrumentTrackWindow()->toggleVisibility( _on );
}




void InstrumentTrackView::activityIndicatorPressed()
{
	model()->processInEvent( MidiEvent( MidiNoteOn, 0, DefaultKey, MidiDefaultVelocity ) );
}




void InstrumentTrackView::activityIndicatorReleased()
{
	model()->processInEvent( MidiEvent( MidiNoteOff, 0, DefaultKey, 0 ) );
}





void InstrumentTrackView::midiInSelected()
{
	if( model() )
	{
		model()->m_midiPort.setReadable( m_midiInputAction->isChecked() );
	}
}




void InstrumentTrackView::midiOutSelected()
{
	if( model() )
	{
		model()->m_midiPort.setWritable( m_midiOutputAction->isChecked() );
	}
}




void InstrumentTrackView::midiConfigChanged()
{
	m_midiInputAction->setChecked( model()->m_midiPort.isReadable() );
	m_midiOutputAction->setChecked( model()->m_midiPort.isWritable() );
}




//FIXME: This is identical to SampleTrackView::createMixerMenu
QMenu * InstrumentTrackView::createMixerMenu(QString title, QString newMixerLabel)
{
	int channelIndex = model()->mixerChannelModel()->value();

	MixerChannel *mixerChannel = Engine::mixer()->mixerChannel( channelIndex );

	// If title allows interpolation, pass channel index and name
	if ( title.contains( "%2" ) )
	{
		title = title.arg( channelIndex ).arg( mixerChannel->m_name );
	}

	auto mixerMenu = new QMenu(title);

	mixerMenu->addAction( newMixerLabel, this, SLOT(createMixerLine()));
	mixerMenu->addSeparator();

	for (int i = 0; i < Engine::mixer()->numChannels(); ++i)
	{
		MixerChannel * currentChannel = Engine::mixer()->mixerChannel( i );

		if ( currentChannel != mixerChannel )
		{
			auto index = currentChannel->index();
			QString label = tr( "%1: %2" ).arg(index).arg(currentChannel->m_name);
			mixerMenu->addAction(label, [this, index](){
				assignMixerLine(index);
			});
		}
	}

	return mixerMenu;
}

QPixmap InstrumentTrackView::determinePixmap(InstrumentTrack* instrumentTrack)
{
	if (instrumentTrack)
	{
		Instrument* instrument = instrumentTrack->instrument();

		if (instrument && instrument->descriptor())
		{
			const PixmapLoader* pl = instrument->key().isValid()
				? instrument->key().logo()
				: instrument->descriptor()->logo;

			if (pl)
			{
				return pl->pixmap();
			}
		}
	}

	return embed::getIconPixmap("instrument_track");
}


} // namespace lmms::gui
