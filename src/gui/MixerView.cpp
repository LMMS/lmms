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


#include <QCheckBox>
#include <QLayout>
#include <QMessageBox>
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

namespace lmms::gui
{


MixerView::MixerView() :
	QWidget(),
	ModelView( nullptr, this ),
	SerializingObjectHook()
{
#if QT_VERSION < 0x50C00
	// Workaround for a bug in Qt versions below 5.12,
	// where argument-dependent-lookup fails for QFlags operators
	// declared inside a namepsace.
	// This affects the Q_DECLARE_OPERATORS_FOR_FLAGS macro in Instrument.h
	// See also: https://codereview.qt-project.org/c/qt/qtbase/+/225348

	using ::operator|;
#endif

	Mixer * m = Engine::mixer();
	m->setHook( this );

	//QPalette pal = palette();
	//pal.setColor( QPalette::Window, QColor( 72, 76, 88 ) );
	//setPalette( pal );

	setAutoFillBackground( true );
	setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

	setWindowTitle( tr( "Mixer" ) );
	setWindowIcon( embed::getIconPixmap( "mixer" ) );

	// main-layout
	auto ml = new QHBoxLayout;

	// Set margins
	ml->setContentsMargins( 0, 4, 0, 0 );

	// Channel area
	m_channelAreaWidget = new QWidget;
	chLayout = new QHBoxLayout( m_channelAreaWidget );
	chLayout->setSizeConstraint( QLayout::SetMinimumSize );
	chLayout->setSpacing( 0 );
	chLayout->setContentsMargins(0, 0, 0, 0);
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

	// show the add new mixer channel button
	auto newChannelBtn = new QPushButton(embed::getIconPixmap("new_channel"), QString(), this);
	newChannelBtn->setObjectName( "newChannelBtn" );
	newChannelBtn->setFixedSize( mixerLineSize );
	connect( newChannelBtn, SIGNAL(clicked()), this, SLOT(addNewChannel()));
	ml->addWidget( newChannelBtn, 0, Qt::AlignTop );


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
}

MixerView::~MixerView()
{
	for (auto mixerChannelView : m_mixerChannelViews)
	{
		delete mixerChannelView;
	}
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

	updateMaxChannelSelector();

	return newChannelIndex;
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
	const TrackContainer::TrackList& songTracks = Engine::getSong()->tracks();
	const TrackContainer::TrackList& patternStoreTracks = Engine::patternStore()->tracks();

	for (const auto& trackList : {songTracks, patternStoreTracks})
	{
		for (const auto& track : trackList)
		{
			if (track->type() == Track::Type::Instrument)
			{
				auto inst = (InstrumentTrack*)track;
				inst->mixerChannelModel()->setRange(0,
					m_mixerChannelViews.size()-1,1);
			}
			else if (track->type() == Track::Type::Sample)
			{
				auto strk = (SampleTrack*)track;
				strk->mixerChannelModel()->setRange(0,
					m_mixerChannelViews.size()-1,1);
			}
		}
	}
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
	thisLine->setToolTip( Engine::mixer()->mixerChannel( index )->m_name );

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
	// can't delete master
	if( index == 0 ) return;

	// if there is no user confirmation, do nothing
	if (!confirmRemoval(index))
	{
		return;
	}

	// remember selected line
	int selLine = m_currentMixerLine->channelIndex();

	// in case the deleted channel is soloed or the remaining
	// channels will be left in a muted state
	Engine::mixer()->clearChannel(index);

	// delete the real channel
	Engine::mixer()->deleteChannel(index);

	// delete the view
	chLayout->removeWidget(m_mixerChannelViews[index]->m_mixerLine);
	m_racksLayout->removeWidget(m_mixerChannelViews[index]->m_rackView);
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
	for (int i = index + 1; i < m_mixerChannelViews.size(); ++i)
	{
		m_mixerChannelViews[i]->m_mixerLine->setChannelIndex(i - 1);
	}
	m_mixerChannelViews.remove(index);

	// select the next channel
	if (selLine >= m_mixerChannelViews.size())
	{
		selLine = m_mixerChannelViews.size() - 1;
	}
	setCurrentMixerLine(selLine);

	updateMaxChannelSelector();
}

bool MixerView::confirmRemoval(int index)
{
	// if config variable is set to false, there is no need for user confirmation
	bool needConfirm = ConfigManager::inst()->value("ui", "mixerchanneldeletionwarning", "1").toInt();
	if (!needConfirm) { return true; }

	Mixer* mix = Engine::mixer();

	if (!mix->isChannelInUse(index))
	{
		// is the channel is not in use, there is no need for user confirmation
		return true;
	}

	QString messageRemoveTrack = tr("This Mixer Channel is being used.\n"
									"Are you sure you want to remove this channel?\n\n"
									"Warning: This operation can not be undone.");

	QString messageTitleRemoveTrack = tr("Confirm removal");
	QString askAgainText = tr("Don't ask again");
	auto askAgainCheckBox = new QCheckBox(askAgainText, nullptr);
	connect(askAgainCheckBox, &QCheckBox::stateChanged, [this](int state) {
		// Invert button state, if it's checked we *shouldn't* ask again
		ConfigManager::inst()->setValue("ui", "mixerchanneldeletionwarning", state ? "0" : "1");
	});

	QMessageBox mb(this);
	mb.setText(messageRemoveTrack);
	mb.setWindowTitle(messageTitleRemoveTrack);
	mb.setIcon(QMessageBox::Warning);
	mb.addButton(QMessageBox::Cancel);
	mb.addButton(QMessageBox::Ok);
	mb.setCheckBox(askAgainCheckBox);
	mb.setDefaultButton(QMessageBox::Cancel);

	int answer = mb.exec();

	return answer == QMessageBox::Ok;
}


void MixerView::deleteUnusedChannels()
{
	Mixer* mix = Engine::mixer();

	// Check all channels except master, delete those with no incoming sends
	for (int i = m_mixerChannelViews.size() - 1; i > 0; --i)
	{
		if (!mix->isChannelInUse(i))
		{
			deleteChannel(i);
		}
	}
}



void MixerView::moveChannelLeft(int index, int focusIndex)
{
	// can't move master or first channel left or last channel right
	if( index <= 1 || index >= m_mixerChannelViews.size() ) return;

	Mixer *m = Engine::mixer();

	// Move instruments channels
	m->moveChannelLeft( index );

	// Update widgets models
	m_mixerChannelViews[index]->setChannelIndex( index );
	m_mixerChannelViews[index - 1]->setChannelIndex( index - 1 );

	// Focus on new position
	setCurrentMixerLine( focusIndex );
}



void MixerView::moveChannelLeft(int index)
{
	moveChannelLeft( index, index - 1 );
}



void MixerView::moveChannelRight(int index)
{
	moveChannelLeft( index + 1, index + 1 );
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


} // namespace lmms::gui
