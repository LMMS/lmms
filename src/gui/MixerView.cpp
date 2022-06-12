/*
 * MixerView.cpp - effect-mixer-view for LMMS
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QMenu>
#include <QLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QKeyEvent>

#include "lmms_math.h"

#include "MixerView.h"
#include "Knob.h"
#include "MixerLine.h"
#include "Mixer.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "AudioEngine.h"
#include "InstrumentTrack.h"
#include "PatternStore.h"
#include "SampleTrack.h"
#include "SendButtonIndicator.h"
#include "Song.h"
#include "SubWindow.h"
#include "TrackContainer.h" // For TrackContainer::TrackList typedef
#include "gui_templates.h"

namespace lmms::gui
{


MixerView::MixerView() :
	QWidget(),
	ModelView( nullptr, this ),
	SerializingObjectHook()
{
	Mixer * m = Engine::mixer();
	m->setHook( this );

	//QPalette pal = palette();
	//pal.setColor( QPalette::Background, QColor( 72, 76, 88 ) );
	//setPalette( pal );

	setAutoFillBackground( true );
	setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

	setWindowTitle( tr( "Mixer" ) );
	setWindowIcon( embed::getIconPixmap( "mixer" ) );

	// main-layout
	QHBoxLayout * ml = new QHBoxLayout;

	// Set margins
	ml->setContentsMargins( 0, 4, 0, 0 );

	// Channel area
	m_channelAreaWidget = new QWidget;
	chLayout = new QHBoxLayout( m_channelAreaWidget );
	chLayout->setSizeConstraint( QLayout::SetMinimumSize );
	chLayout->setSpacing( 0 );
	chLayout->setMargin( 0 );
	m_channelAreaWidget->setLayout(chLayout);

	// create rack layout before creating the first channel
	m_racksWidget = new QWidget;
	m_racksLayout = new QStackedLayout( m_racksWidget );
	m_racksLayout->setContentsMargins( 0, 0, 0, 0 );
	m_racksWidget->setLayout( m_racksLayout );

	// add master channel
	m_mixerChannelViews.resize( m->numChannels() );
	m_mixerChannelViews[0] = new MixerChannelView( this, this, 0 );

	m_racksLayout->addWidget( m_mixerChannelViews[0]->m_rackView );

	MixerChannelView * masterView = m_mixerChannelViews[0];
	ml->addWidget( masterView->m_mixerLine, 0, Qt::AlignTop );

	QSize mixerLineSize = masterView->m_mixerLine->size();

	// add mixer channels
	for( int i = 1; i < m_mixerChannelViews.size(); ++i )
	{
		m_mixerChannelViews[i] = new MixerChannelView(m_channelAreaWidget, this, i);
		chLayout->addWidget( m_mixerChannelViews[i]->m_mixerLine );
	}

	// add the scrolling section to the main layout
	// class solely for scroll area to pass key presses down
	class ChannelArea : public QScrollArea
	{
		public:
			ChannelArea( QWidget * parent, MixerView * mv ) :
				QScrollArea( parent ), m_mv( mv ) {}
			~ChannelArea() override = default;
			void keyPressEvent( QKeyEvent * e ) override
			{
				m_mv->keyPressEvent( e );
			}
		private:
			MixerView * m_mv;
	};
	channelArea = new ChannelArea( this, this );
	channelArea->setWidget( m_channelAreaWidget );
	channelArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	channelArea->setFrameStyle( QFrame::NoFrame );
	channelArea->setMinimumWidth( mixerLineSize.width() * 6 );
	channelArea->setFixedHeight( mixerLineSize.height() +
			style()->pixelMetric( QStyle::PM_ScrollBarExtent ) );
	ml->addWidget( channelArea, 1, Qt::AlignTop );

	QWidget * buttonAreaWidget = new QWidget;
	buttonAreaWidget->setFixedSize(mixerLineSize);
	QVBoxLayout * bl = new QVBoxLayout( buttonAreaWidget );
	bl->setSizeConstraint( QLayout::SetMinimumSize );
	bl->setSpacing( 0 );
	bl->setMargin( 0 );
	ml->addWidget(buttonAreaWidget);

	// show the add new mixer channel button
	QPushButton * newChannelBtn = new QPushButton( embed::getIconPixmap( "new_channel" ), QString(), this );
	newChannelBtn->setObjectName( "newChannelBtn" );
	connect( newChannelBtn, SIGNAL(clicked()), this, SLOT(addNewChannel()));
	//ml->addWidget( newChannelBtn, 0, Qt::AlignTop );
	bl->addWidget(newChannelBtn, 0, Qt::AlignTop );

	QMenu * toMenu = new QMenu(this );
	toMenu->setFont( pointSize<9>( toMenu->font() ) );
	connect( toMenu, SIGNAL( aboutToShow() ), this, SLOT( updateMenu() ) );

	m_autoLinkTrackSettingsBtn = new QPushButton(embed::getIconPixmap( "trackop" ),QString(), this);
	m_autoLinkTrackSettingsBtn->move( 12, 1 );
	m_autoLinkTrackSettingsBtn->setFocusPolicy( Qt::NoFocus );
	m_autoLinkTrackSettingsBtn->setMenu( toMenu );
	m_autoLinkTrackSettingsBtn->setToolTip(tr("Auto track link settings"));
	bl->addWidget(m_autoLinkTrackSettingsBtn, 0, Qt::AlignTop );

	m_toogleAutoLinkTrackConfigBtn = new QPushButton(this);
	m_toogleAutoLinkTrackConfigBtn->setToolTip(tr("Enable/Disable auto track link"));
	connect( m_toogleAutoLinkTrackConfigBtn, SIGNAL(clicked()), this, SLOT(toogleAutoLinkTrackConfig()));
	updateAutoLinkTrackConfigBtn(Engine::mixer()->getAutoLinkTrackSettings().enabled);
	bl->addWidget(m_toogleAutoLinkTrackConfigBtn, 0, Qt::AlignTop );


	// add the stacked layout for the effect racks of mixer channels
	ml->addWidget( m_racksWidget, 0, Qt::AlignTop | Qt::AlignRight );

	setCurrentMixerLine( m_mixerChannelViews[0]->m_mixerLine );

	setLayout( ml );
	updateGeometry();

	// timer for updating faders
	connect( getGUI()->mainWindow(), SIGNAL(periodicUpdate()),
					this, SLOT(updateFaders()));


	// add ourself to workspace
	QMdiSubWindow * subWin = getGUI()->mainWindow()->addWindowedWidget( this );
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );
	layout()->setSizeConstraint( QLayout::SetMinimumSize );
	subWin->layout()->setSizeConstraint( QLayout::SetMinAndMaxSize );

	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->move( 5, 310 );

	// we want to receive dataChanged-signals in order to update
	setModel( m );

	setAutoTrackConstraints();
}

MixerView::~MixerView()
{
	for (int i=0; i<m_mixerChannelViews.size(); i++)
	{
		delete m_mixerChannelViews.at(i);
	}
}


void MixerView::updateMenu()
{
	QMenu* toMenu = m_autoLinkTrackSettingsBtn->menu();
	toMenu->clear();

	// no widget but same approach as for CaptionMenu

	QAction * captionMain = toMenu->addAction( "Auto track linking" );
	captionMain->setEnabled( false );
	QMenu* sortMenu = toMenu->addMenu(tr("Sort"));

	QMenu* settingsMenu = toMenu->addMenu(tr("Settings"));
	QMenu* linkModeMenu = settingsMenu->addMenu(tr("Link mode"));
	QMenu* linkStyleMenu = settingsMenu->addMenu(tr("Link style type"));
	QMenu* sortAutoMenu = settingsMenu->addMenu(tr("Automatic sorting"));
	QMenu* addAutoMenu = settingsMenu->addMenu(tr("Automatic add"));
	QMenu* deleteAutoMenu = settingsMenu->addMenu(tr("Automatic delete"));
	//QMenu* patternEditorMenu = settingsMenu->addMenu(tr("Pattern Editor"));

	auto mix = Engine::mixer();
	// remark: capturing "settings" (also as pointer) does not work as expected
	auto settings = mix->getAutoLinkTrackSettings();

	linkStyleMenu->addAction( tr("Name and color") +
		(settings.linkNameAndColor() ? " *" :""),
		this, [mix]()
	{
		auto settings = mix->getAutoLinkTrackSettings();
		settings.linkName = true;
		settings.linkColor = true;
		mix->saveAutoLinkTrackSettings(settings);
	});
	linkStyleMenu->addAction( tr("Color only") +
		(settings.linkColorOnly() ? " *" :""),
		this, [mix]()
	{
		auto settings = mix->getAutoLinkTrackSettings();
		settings.linkName = false;
		settings.linkColor = true;
		mix->saveAutoLinkTrackSettings(settings);
	});

	linkModeMenu->addAction( tr("One track to one channel") +
		(settings.linkMode ==  Mixer::autoTrackLinkSettings::LinkMode::OneToOne ? " *" :""),
		this, [this, mix]()
	{
		auto settings = mix->getAutoLinkTrackSettings();
		settings.linkMode = Mixer::autoTrackLinkSettings::LinkMode::OneToOne;
		mix->saveAutoLinkTrackSettings(settings);
		setAutoTrackConstraints();
	});
	linkModeMenu->addAction( tr("Multiple tracks to one channel") +
		(settings.linkMode ==  Mixer::autoTrackLinkSettings::LinkMode::OneToMany ? " *" :""),
		this, [this, mix]()
	{
		auto settings = mix->getAutoLinkTrackSettings();
		settings.linkMode = Mixer::autoTrackLinkSettings::LinkMode::OneToMany;
		mix->saveAutoLinkTrackSettings(settings);
		setAutoTrackConstraints();
	});


	sortMenu->addAction( tr("Song Editor -> Pattern Editor"), this, [this]()
	{
		updateAutoTrackSortOrder(false);
	});
	sortMenu->addAction( tr("Pattern Editor -> Song Editor"), this, [this]()
	{
		updateAutoTrackSortOrder(false);
	});

	sortAutoMenu->addAction(tr("Disabled") +
		(settings.sort == Mixer::autoTrackLinkSettings::AutoSort::Disabled ? " *" :""),
		this, [mix]()
	{
		auto settings = mix->getAutoLinkTrackSettings();
		settings.sort = Mixer::autoTrackLinkSettings::AutoSort::Disabled;
		mix->saveAutoLinkTrackSettings(settings);
	});
	sortAutoMenu->addAction( tr("Song Editor -> Pattern Editor") +
		(settings.sort == Mixer::autoTrackLinkSettings::AutoSort::LinkedPattern ? " *" :""),
		this, [this, mix]()
	{
		auto settings = mix->getAutoLinkTrackSettings();
		settings.sort = Mixer::autoTrackLinkSettings::AutoSort::LinkedPattern;
		mix->saveAutoLinkTrackSettings(settings);
		updateAutoTrackSortOrder();
	});
	sortAutoMenu->addAction( tr("Pattern Editor - > Song Editor") +
		(settings.sort == Mixer::autoTrackLinkSettings::AutoSort::PatternLinked ? " *" :""),
		this, [this, mix]()
	{
		auto settings = mix->getAutoLinkTrackSettings();
		settings.sort = Mixer::autoTrackLinkSettings::AutoSort::PatternLinked;
		mix->saveAutoLinkTrackSettings(settings);
		updateAutoTrackSortOrder();
	});

	QAction * captionPatternEditor = addAutoMenu->addAction( "Pattern Editor" );
	captionPatternEditor->setEnabled( false );

	addAutoMenu->addAction( tr("Disabled") +
		(settings.autoAddPatternEditor == Mixer::autoTrackLinkSettings::AutoAdd::Disabled ? " *" :""),
		this, [mix]()
	{
		auto settings = mix->getAutoLinkTrackSettings();
		settings.autoAddPatternEditor = Mixer::autoTrackLinkSettings::AutoAdd::Disabled;
		mix->saveAutoLinkTrackSettings(settings);
	});
	addAutoMenu->addAction( tr("Each separate") +
		(settings.autoAddPatternEditor == Mixer::autoTrackLinkSettings::AutoAdd::Separate ? " *" :""),
		this, [mix]()
	{
		auto settings = mix->getAutoLinkTrackSettings();
		settings.autoAddPatternEditor = Mixer::autoTrackLinkSettings::AutoAdd::Separate;
		mix->saveAutoLinkTrackSettings(settings);
	});
	addAutoMenu->addAction( tr("Use first track's channel") +
		(settings.autoAddPatternEditor == Mixer::autoTrackLinkSettings::AutoAdd::UseFirstTrackOnly ? " *" :""),
		this, [mix]()
	{
		auto settings = mix->getAutoLinkTrackSettings();
		settings.autoAddPatternEditor = Mixer::autoTrackLinkSettings::AutoAdd::UseFirstTrackOnly;
		mix->saveAutoLinkTrackSettings(settings);
	});

	QAction * captionSongEditor = addAutoMenu->addAction( "Song Editor" );
	captionSongEditor->setEnabled( false );

	addAutoMenu->addAction( tr("Disabled") +
		(settings.autoAddSongEditor == Mixer::autoTrackLinkSettings::AutoAdd::Disabled ? " *" :""),
		this, [mix]()
	{
		auto settings = mix->getAutoLinkTrackSettings();
		settings.autoAddSongEditor = Mixer::autoTrackLinkSettings::AutoAdd::Disabled;
		mix->saveAutoLinkTrackSettings(settings);
	});
	addAutoMenu->addAction( tr("Each separate") +
		(settings.autoAddSongEditor == Mixer::autoTrackLinkSettings::AutoAdd::Separate ? " *" :""),
		this, [mix]()
	{
		auto settings = mix->getAutoLinkTrackSettings();
		settings.autoAddSongEditor = Mixer::autoTrackLinkSettings::AutoAdd::Separate;
		mix->saveAutoLinkTrackSettings(settings);
	});

	deleteAutoMenu->addAction( tr("Disabled") +
		(!settings.autoDelete ? " *" :""),
		this, [mix]()
	{
		auto settings = mix->getAutoLinkTrackSettings();
		settings.autoDelete = false;
		mix->saveAutoLinkTrackSettings(settings);
	});
	deleteAutoMenu->addAction( tr("Enabled") +
		(settings.autoDelete ? " *" :""),
		this, [mix]()
	{
		auto settings = mix->getAutoLinkTrackSettings();
		settings.autoDelete = true;
		mix->saveAutoLinkTrackSettings(settings);
	});

}

void MixerView::updateAfterTrackAdd(Track * track, QString name, bool isPatternEditor)
{
	auto mix = Engine::mixer();
	auto settings =mix->getAutoLinkTrackSettings();
	if (!settings.enabled || !settings.autoAdd()) return;
	IntModel * model = mix->getChannelModelByTrack(track);
	if ( model != nullptr)
	{

		if (isPatternEditor)
		{
			if (settings.autoAddPatternEditor == Mixer::autoTrackLinkSettings::AutoAdd::Disabled) return;

			if (settings.autoAddPatternEditor == Mixer::autoTrackLinkSettings::AutoAdd::UseFirstTrackOnly)
			{
				auto trackList = Engine::patternStore()->tracks();
				if (!trackList.empty())
				{
					auto channel = mix->getCustomChannelByTrack(trackList.first());
					if (channel != nullptr)
					{
						model->setValue(channel->m_channelIndex);
					}
				}
				return;
			}
		}

		int channelIndex = addNewChannel();
		model->setValue( channelIndex );
		mix->mixerChannel(channelIndex)->m_autoTrackLinkModel.setValue(true);
		m_mixerChannelViews[channelIndex]->m_mixerLine->autoTrackLinkChanged();

		// it may be that the track name is not available yet because of async loading
		if (name != "") track->setName(name);
		updateAfterTrackStyleModify(track);

		setCurrentMixerLine( channelIndex );
	}
}

void MixerView::trackStyleToChannel(Track * track, MixerChannel * channel)
{
	auto mix = Engine::mixer();
	auto settings =mix->getAutoLinkTrackSettings();
	if (settings.linkName)
	{
		channel->m_name = track->name();
	}
	if (settings.linkColor)
	{
		channel->setColor (track->color());
		channel->m_hasColor = track->useColor();
	}
}

void MixerView::channelStyleToTrack(MixerChannel * channel, Track * track)
{
	auto mix = Engine::mixer();
	auto settings =mix->getAutoLinkTrackSettings();
	if (settings.linkName)
	{
		track->setName(channel->m_name);
	}
	if (settings.linkColor)
	{
		track->setColor(channel->m_color);
		if (!channel->m_hasColor) track->resetColor();
	}
}

void MixerView::updateAfterTrackStyleModify(Track * track)
{
	auto mix = Engine::mixer();
	auto settings =mix->getAutoLinkTrackSettings();	
	if (!settings.enabled || !settings.linkStyles()) return;
	auto channel = mix->getCustomChannelByTrack(track);
	if (channel != nullptr && channel->m_autoTrackLinkModel.value())
	{
		trackStyleToChannel(track, channel);
		mix->processAssignedTracks([this, channel](Track * otherTrack, IntModel * model, MixerChannel *)
		mutable {
			if (model->value() == channel->m_channelIndex)
			{
				channelStyleToTrack(channel, otherTrack);
			}
		});
		setCurrentMixerLine(channel->m_channelIndex);
	}
}

void MixerView::updateAfterTrackMixerLineModify(Track * track)
{
	auto mix = Engine::mixer();
	auto settings =mix->getAutoLinkTrackSettings();
	if (!settings.enabled) return;
	IntModel * model = mix->getChannelModelByTrack(track);
	if (model != nullptr)
	{
		setAutoTrackConstraints();
	}
}

void MixerView::setAutoTrackConstraints()
{
	auto mix = Engine::mixer();
	auto settings =mix->getAutoLinkTrackSettings();
	std::vector<int> usedChannelCounts = mix->getUsedChannelCounts();
	bool wasModified = false;
	for(unsigned long i = 0; i < usedChannelCounts.size(); i++)
	{
		if (settings.enabled)
		{
			// no more linked tracks or too many linked tracks
			if (usedChannelCounts[i] == 0 || (usedChannelCounts[i] > 1 &&
				settings.linkMode == Mixer::autoTrackLinkSettings::LinkMode::OneToOne))
			{
				mix->mixerChannel(i)->m_autoTrackLinkModel.setValue(false);
				m_mixerChannelViews[i]->m_mixerLine->autoTrackLinkChanged();
				wasModified = true;
			}
		}
		// update mixer lines in both cases (enabling/disabling)
		m_mixerChannelViews[i]->m_mixerLine->autoTrackLinkChanged();
	}
	if (!settings.enabled) return;

	// make sure that the tracks are updated according to the auto link settings
	mix->processAssignedTracks([this](Track * track, IntModel *, MixerChannel * channel)
	mutable {
		if (channel != nullptr)
		{
			if (channel->m_autoTrackLinkModel.value())
			{
				channelStyleToTrack(channel, track);
			}
		}
	});
	if (wasModified) updateAutoTrackSortOrder();
}

void MixerView::updateAfterTrackMove(Track * track)
{
	auto mix = Engine::mixer();
	auto settings =mix->getAutoLinkTrackSettings();
	if (!settings.enabled || settings.sort == Mixer::autoTrackLinkSettings::AutoSort::Disabled) return;
	auto channel = mix->getCustomChannelByTrack(track);
	if (channel != nullptr && channel->m_autoTrackLinkModel.value())
	{
		updateAutoTrackSortOrder();
	}
}

void MixerView::updateAfterTrackDelete(Track * track)
{
	auto mix = Engine::mixer();
	auto settings =mix->getAutoLinkTrackSettings();
	if (!settings.enabled) return;
	if (!settings.autoDelete)
	{
		setAutoTrackConstraints();
		return;
	}
	auto channel = mix->getCustomChannelByTrack(track);
	if (channel != nullptr && channel->m_autoTrackLinkModel.value())
	{
		bool shouldDelete = true;
		if (settings.linkMode == Mixer::autoTrackLinkSettings::LinkMode::OneToMany)
		{
			std::vector<int> usedChannelCounts = mix->getUsedChannelCounts();
			shouldDelete = usedChannelCounts[channel->m_channelIndex] == 0;
		}
		if (shouldDelete)
		{
			deleteChannel(channel->m_channelIndex);
			updateAutoTrackSortOrder();
		}
	}
}

void MixerView::setAutoLinkTrackConfig(bool enabled)
{
	auto mix = Engine::mixer();
	auto settings =mix->getAutoLinkTrackSettings();
	updateAutoLinkTrackConfigBtn(enabled);
	settings.enabled = enabled;
	mix->saveAutoLinkTrackSettings(settings);
	setAutoTrackConstraints();
}

void MixerView::updateAutoLinkTrackConfigBtn(bool enabled)
{
	m_toogleAutoLinkTrackConfigBtn->setIcon(enabled ? embed::getIconPixmap( "auto_link_active" ) : embed::getIconPixmap( "auto_link_inactive" ));
}

void MixerView::toogleAutoLinkTrackConfig()
{
	auto mix = Engine::mixer();
	auto settings =mix->getAutoLinkTrackSettings();
	setAutoLinkTrackConfig(!settings.enabled);
}

int MixerView::addNewChannel()
{
	// add new mixer channel and redraw the form.
	Mixer * mix = Engine::mixer();

	int newChannelIndex = mix->createChannel();
	m_mixerChannelViews.push_back(new MixerChannelView(m_channelAreaWidget, this,
												 newChannelIndex));
	chLayout->addWidget( m_mixerChannelViews[newChannelIndex]->m_mixerLine );
	m_racksLayout->addWidget( m_mixerChannelViews[newChannelIndex]->m_rackView );

	updateMixerLine(newChannelIndex);

	updateAutoTrackSortOrder();
	updateMaxChannelSelector();

	return newChannelIndex;
}

void MixerView::updateAutoTrackSortOrder(bool autoSort)
{
	Mixer * mix = Engine::mixer();
	auto settings =mix->getAutoLinkTrackSettings();
	if (!settings.enabled) return;
	if (autoSort && settings.sort == Mixer::autoTrackLinkSettings::AutoSort::Disabled) return;

	std::vector<int> list(m_mixerChannelViews.size(), 0);

	int c = 0;
	// add all non auto track first
	for( int i = 1; i<m_mixerChannelViews.size(); ++i )
	{
		if (!mix->mixerChannel(i)->m_autoTrackLinkModel.value())
		{
			list[c++] = i;
		}
	}

	// add auto tracks in the order of the song tracks
	mix->processAssignedTracks([&, list](Track *, IntModel * model, MixerChannel * channel)
	mutable {
		if (channel == nullptr) return;
		if (channel->m_autoTrackLinkModel.value())
		{
			list[c++] = model->value();
		}
	});

	// bubblesort here because the list is normally almost ordered
	bool swapped = false;
	do
	{
		swapped = false;
		for (int i=0; i<c-1; ++i)
		{
			if (list[i] > list[i+1])
			{
				// a (+1) because we didn't include master track in our list
				swapChannels(list[i+1], list[i+2]);
				int t = list[i];
				list[i] = list[i+1];
				list[i+1] = t;
				swapped = true;
			}
		}
		c = c-1;
	} while (swapped);

	// TODO: think about focus
	// setCurrentMixerLine( index - 1 );
}


void MixerView::refreshDisplay()
{
	// delete all views and re-add them
	for( int i = 1; i<m_mixerChannelViews.size(); ++i )
	{
		chLayout->removeWidget(m_mixerChannelViews[i]->m_mixerLine);
		m_racksLayout->removeWidget( m_mixerChannelViews[i]->m_rackView );
		delete m_mixerChannelViews[i]->m_fader;
		delete m_mixerChannelViews[i]->m_muteBtn;
		delete m_mixerChannelViews[i]->m_soloBtn;
		delete m_mixerChannelViews[i]->m_mixerLine;
		delete m_mixerChannelViews[i]->m_rackView;
		delete m_mixerChannelViews[i];
	}
	m_channelAreaWidget->adjustSize();

	// re-add the views
	m_mixerChannelViews.resize(Engine::mixer()->numChannels());
	for( int i = 1; i < m_mixerChannelViews.size(); ++i )
	{
		m_mixerChannelViews[i] = new MixerChannelView(m_channelAreaWidget, this, i);
		chLayout->addWidget(m_mixerChannelViews[i]->m_mixerLine);
		m_racksLayout->addWidget( m_mixerChannelViews[i]->m_rackView );
	}

	// set selected mixer line to 0
	setCurrentMixerLine( 0 );

	// update all mixer lines
	for( int i = 0; i < m_mixerChannelViews.size(); ++i )
	{
		updateMixerLine( i );
	}

	updateMaxChannelSelector();
}


// update the and max. channel number for every instrument
void MixerView::updateMaxChannelSelector()
{
	Mixer * mix = Engine::mixer();
	mix->processAssignedTracks([this](Track *, IntModel * model, MixerChannel *)
	{
		model->setRange(0,m_mixerChannelViews.size()-1,1);
	});
}



void MixerView::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	MainWindow::saveWidgetState( this, _this );
}




void MixerView::loadSettings( const QDomElement & _this )
{
	MainWindow::restoreWidgetState( this, _this );
}


MixerView::MixerChannelView::MixerChannelView(QWidget * _parent, MixerView * _mv,
										  int channelIndex )
{
	m_mixerLine = new MixerLine(_parent, _mv, channelIndex);

	MixerChannel *mixerChannel = Engine::mixer()->mixerChannel(channelIndex);

	m_fader = new Fader( &mixerChannel->m_volumeModel,
					tr( "Fader %1" ).arg( channelIndex ), m_mixerLine );
	m_fader->setLevelsDisplayedInDBFS();
	m_fader->setMinPeak(dbfsToAmp(-42));
	m_fader->setMaxPeak(dbfsToAmp(9));

	m_fader->move( 16-m_fader->width()/2,
					m_mixerLine->height()-
					m_fader->height()-5 );

	m_muteBtn = new PixmapButton( m_mixerLine, tr( "Mute" ) );
	m_muteBtn->setModel( &mixerChannel->m_muteModel );
	m_muteBtn->setActiveGraphic(
				embed::getIconPixmap( "led_off" ) );
	m_muteBtn->setInactiveGraphic(
				embed::getIconPixmap( "led_green" ) );
	m_muteBtn->setCheckable( true );
	m_muteBtn->move( 9,  m_fader->y()-11);
	m_muteBtn->setToolTip(tr("Mute this channel"));

	m_soloBtn = new PixmapButton( m_mixerLine, tr( "Solo" ) );
	m_soloBtn->setModel( &mixerChannel->m_soloModel );
	m_soloBtn->setActiveGraphic(
				embed::getIconPixmap( "led_red" ) );
	m_soloBtn->setInactiveGraphic(
				embed::getIconPixmap( "led_off" ) );
	m_soloBtn->setCheckable( true );
	m_soloBtn->move( 9,  m_fader->y()-21);
	connect(&mixerChannel->m_soloModel, SIGNAL(dataChanged()),
			_mv, SLOT ( toggledSolo() ), Qt::DirectConnection );
	m_soloBtn->setToolTip(tr("Solo this channel"));

	// Create EffectRack for the channel
	m_rackView = new EffectRackView( &mixerChannel->m_fxChain, _mv->m_racksWidget );
	m_rackView->setFixedSize( EffectRackView::DEFAULT_WIDTH, MixerLine::MixerLineHeight );
}


void MixerView::MixerChannelView::setChannelIndex( int index )
{
	MixerChannel* mixerChannel = Engine::mixer()->mixerChannel( index );

	m_fader->setModel( &mixerChannel->m_volumeModel );
	m_muteBtn->setModel( &mixerChannel->m_muteModel );
	m_soloBtn->setModel( &mixerChannel->m_soloModel );
	m_rackView->setModel( &mixerChannel->m_fxChain );
}


void MixerView::toggledSolo()
{
	Engine::mixer()->toggledSolo();
}



void MixerView::setCurrentMixerLine( MixerLine * _line )
{
	// select
	m_currentMixerLine = _line;
	m_racksLayout->setCurrentWidget( m_mixerChannelViews[ _line->channelIndex() ]->m_rackView );

	// set up send knob
	for(int i = 0; i < m_mixerChannelViews.size(); ++i)
	{
		updateMixerLine(i);
	}
}


void MixerView::updateMixerLine(int index)
{
	Mixer * mix = Engine::mixer();

	// does current channel send to this channel?
	int selIndex = m_currentMixerLine->channelIndex();
	MixerLine * thisLine = m_mixerChannelViews[index]->m_mixerLine;
	thisLine->setToolTip( mix->mixerChannel( index )->m_name );

	FloatModel * sendModel = mix->channelSendModel(selIndex, index);
	if( sendModel == nullptr )
	{
		// does not send, hide send knob
		thisLine->m_sendKnob->setVisible(false);
	}
	else
	{
		// it does send, show knob and connect
		thisLine->m_sendKnob->setVisible(true);
		thisLine->m_sendKnob->setModel(sendModel);
	}

	// disable the send button if it would cause an infinite loop
	thisLine->m_sendBtn->setVisible(! mix->isInfiniteLoop(selIndex, index));
	thisLine->m_sendBtn->updateLightStatus();
	thisLine->update();
}

void MixerView::deleteChannel(int index)
{
	if( index == 0 ) return;

	// remember selected line
	int selLine = m_currentMixerLine->channelIndex();

	deleteChannelInternal(index);

	// select the next channel
	if( selLine >= m_mixerChannelViews.size() )
	{
		selLine = m_mixerChannelViews.size()-1;
	}
	setCurrentMixerLine(selLine);

	updateMaxChannelSelector();
}


void MixerView::deleteChannelInternal(int index)
{
	// can't delete master
	if( index == 0 ) return;

	// in case the deleted channel is soloed or the remaining
	// channels will be left in a muted state
	Engine::mixer()->clearChannel(index);

	// delete the real channel
	Engine::mixer()->deleteChannel(index);

	// delete the view
	chLayout->removeWidget(m_mixerChannelViews[index]->m_mixerLine);
	m_racksLayout->removeWidget( m_mixerChannelViews[index]->m_rackView );
	delete m_mixerChannelViews[index]->m_fader;
	delete m_mixerChannelViews[index]->m_muteBtn;
	delete m_mixerChannelViews[index]->m_soloBtn;
	// delete mixerLine later to prevent a crash when deleting from context menu
	m_mixerChannelViews[index]->m_mixerLine->hide();
	m_mixerChannelViews[index]->m_mixerLine->deleteLater();
	delete m_mixerChannelViews[index]->m_rackView;
	delete m_mixerChannelViews[index];
	m_channelAreaWidget->adjustSize();

	// make sure every channel knows what index it is
	for(int i=index + 1; i<m_mixerChannelViews.size(); ++i)
	{
		m_mixerChannelViews[i]->m_mixerLine->setChannelIndex(i-1);
		m_mixerChannelViews[i]->m_mixerLine->autoTrackLinkChanged();
	}
	m_mixerChannelViews.remove(index);
}



void MixerView::deleteUnusedChannels()
{
	Mixer * mix = Engine::mixer();
	std::vector<int> inUse = mix->getUsedChannelCounts();

	bool needUpdateMax = false;

	//Check all channels except master, delete those with no incoming sends
	for(int i = m_mixerChannelViews.size()-1; i > 0; --i)
	{
		if ((inUse[i]==0) && mix->mixerChannel(i)->m_receives.isEmpty())
		{
			deleteChannelInternal(i);
			needUpdateMax = true;
		}
	}
	if (needUpdateMax) updateMaxChannelSelector();
}


void MixerView::toggleAutoTrackLink(int index)
{
	auto mix = Engine::mixer();
	auto settings =mix->getAutoLinkTrackSettings();
	if (!settings.enabled) return;
	mix->toggleAutoTrackLink(index);
	m_mixerChannelViews[index]->m_mixerLine->autoTrackLinkChanged();
	MixerChannel *  channel = mix->mixerChannel(index);
	if (!channel->m_autoTrackLinkModel.value()) return;

	Track * trackFound = nullptr;
	mix->processAssignedTracks([&trackFound, index](Track * track, IntModel * model, MixerChannel *)
	mutable {
		if (model->value() == index)
		{
			trackFound = track;
		}
	});

	if (trackFound != nullptr)
	{
		updateAutoTrackSortOrder();
		updateAfterTrackStyleModify(trackFound);
	}
}


void MixerView::swapChannels(int indexA, int indexB)
{
	if (( indexA == indexB ) ||
		( indexA < 1 || indexA >= m_mixerChannelViews.size() ) ||
		( indexB < 1 || indexB >= m_mixerChannelViews.size() ))
	{
		return;
	}

	Mixer *m = Engine::mixer();

	m->swapChannels(indexA,indexB);

	// Update widgets models
	m_mixerChannelViews[indexA]->setChannelIndex(indexA);
	m_mixerChannelViews[indexB]->setChannelIndex(indexB );

	m_mixerChannelViews[indexA]->m_mixerLine->autoTrackLinkChanged();
	m_mixerChannelViews[indexB]->m_mixerLine->autoTrackLinkChanged();
}


void MixerView::moveChannelLeft(int index)
{
	swapChannels( index, index - 1);
	setCurrentMixerLine( index - 1 );
}



void MixerView::moveChannelRight(int index)
{
	swapChannels( index , index + 1 );
	setCurrentMixerLine( index + 1 );
}


void MixerView::renameChannel(int index)
{
	m_mixerChannelViews[index]->m_mixerLine->renameChannel();
}



void MixerView::keyPressEvent(QKeyEvent * e)
{
	switch(e->key())
	{
		case Qt::Key_Delete:
			deleteChannel(m_currentMixerLine->channelIndex());
			break;
		case Qt::Key_Left:
			if( e->modifiers() & Qt::AltModifier )
			{
				moveChannelLeft( m_currentMixerLine->channelIndex() );
			}
			else
			{
				// select channel to the left
				setCurrentMixerLine( m_currentMixerLine->channelIndex()-1 );
			}
			break;
		case Qt::Key_Right:
			if( e->modifiers() & Qt::AltModifier )
			{
				moveChannelRight( m_currentMixerLine->channelIndex() );
			}
			else
			{
				// select channel to the right
				setCurrentMixerLine( m_currentMixerLine->channelIndex()+1 );
			}
			break;
		case Qt::Key_Insert:
			if ( e->modifiers() & Qt::ShiftModifier )
			{
				addNewChannel();
			}
			break;
		case Qt::Key_Enter:
		case Qt::Key_Return:
		case Qt::Key_F2:
			renameChannel( m_currentMixerLine->channelIndex() );
			break;
	}
}



void MixerView::closeEvent( QCloseEvent * _ce )
 {
	if( parentWidget() )
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
	_ce->ignore();
 }



void MixerView::setCurrentMixerLine( int _line )
{
	if( _line >= 0 && _line < m_mixerChannelViews.size() )
	{
		setCurrentMixerLine( m_mixerChannelViews[_line]->m_mixerLine );
	}
}



void MixerView::clear()
{
	Engine::mixer()->clear();

	refreshDisplay();
}




void MixerView::updateFaders()
{
	Mixer * m = Engine::mixer();

	// apply master gain
	m->mixerChannel(0)->m_peakLeft *= Engine::audioEngine()->masterGain();
	m->mixerChannel(0)->m_peakRight *= Engine::audioEngine()->masterGain();

	for( int i = 0; i < m_mixerChannelViews.size(); ++i )
	{
		const float opl = m_mixerChannelViews[i]->m_fader->getPeak_L();
		const float opr = m_mixerChannelViews[i]->m_fader->getPeak_R();
		const float fallOff = 1.25;
		if( m->mixerChannel(i)->m_peakLeft >= opl/fallOff )
		{
			m_mixerChannelViews[i]->m_fader->setPeak_L( m->mixerChannel(i)->m_peakLeft );
			// Set to -1 so later we'll know if this value has been refreshed yet.
			m->mixerChannel(i)->m_peakLeft = -1;
		}
		else if( m->mixerChannel(i)->m_peakLeft != -1 )
		{
			m_mixerChannelViews[i]->m_fader->setPeak_L( opl/fallOff );
		}

		if( m->mixerChannel(i)->m_peakRight >= opr/fallOff )
		{
			m_mixerChannelViews[i]->m_fader->setPeak_R( m->mixerChannel(i)->m_peakRight );
			// Set to -1 so later we'll know if this value has been refreshed yet.
			m->mixerChannel(i)->m_peakRight = -1;
		}
		else if( m->mixerChannel(i)->m_peakRight != -1 )
		{
			m_mixerChannelViews[i]->m_fader->setPeak_R( opr/fallOff );
		}
	}
}


} // namesapce lmms::gui
