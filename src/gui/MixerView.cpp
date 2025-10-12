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

#include "MixerView.h"

#include <QHBoxLayout>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QKeyEvent>
#include <QStackedLayout>
#include <QStackedWidget>

#include "EffectRackView.h"
#include "Engine.h"
#include "Fader.h"
#include "GuiApplication.h"
#include "Knob.h"
#include "InstrumentTrack.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "MixerChannelView.h"
#include "PatternStore.h"
#include "SampleTrack.h"
#include "SendButtonIndicator.h"
#include "Song.h"
#include "SubWindow.h"
#include "TrackContainer.h" // For TrackContainer::TrackList typedef
#include "embed.h"

namespace lmms::gui
{


MixerView::MixerView(Mixer* mixer) :
	QWidget(),
	ModelView(nullptr, this),
	SerializingObjectHook(),
	m_mixer(mixer)
{
#if QT_VERSION < 0x50C00
	// Workaround for a bug in Qt versions below 5.12,
	// where argument-dependent-lookup fails for QFlags operators
	// declared inside a namepsace.
	// This affects the Q_DECLARE_OPERATORS_FOR_FLAGS macro in Instrument.h
	// See also: https://codereview.qt-project.org/c/qt/qtbase/+/225348

	using ::operator|;
#endif

	mixer->setHook(this);

	//QPalette pal = palette();
	//pal.setColor(QPalette::Window, QColor(72, 76, 88));
	//setPalette(pal);

	setAutoFillBackground(true);

	setWindowTitle(tr("Mixer"));
	setWindowIcon(embed::getIconPixmap("mixer"));

	// main-layout
	auto ml = new QHBoxLayout{this};

	// Set margins
	ml->setContentsMargins(0, 4, 0, 0);

	// Channel area
	m_channelAreaWidget = new QWidget;
	chLayout = new QHBoxLayout(m_channelAreaWidget);
	chLayout->setSizeConstraint(QLayout::SetMinimumSize);
	chLayout->setSpacing(0);
	chLayout->setContentsMargins(0, 0, 0, 0);
	chLayout->setAlignment(Qt::AlignLeft);
	m_channelAreaWidget->setLayout(chLayout);

	// create rack layout before creating the first channel
	m_racksWidget = new QWidget;
	m_racksLayout = new QStackedLayout(m_racksWidget);
	m_racksLayout->setContentsMargins(0, 0, 0, 0);
	m_racksWidget->setLayout(m_racksLayout);

	// add master channel
	m_mixerChannelViews.resize(mixer->numChannels());
	MixerChannelView * masterView = new MixerChannelView(this, this, 0);
	connectToSoloAndMute(0);
	m_mixerChannelViews[0] = masterView;

	m_racksLayout->addWidget(m_mixerChannelViews[0]->m_effectRackView);

	ml->addWidget(masterView, 0);

	auto mixerChannelSize = masterView->sizeHint();

	// add mixer channels
	for (int i = 1; i < m_mixerChannelViews.size(); ++i)
	{
		m_mixerChannelViews[i] = new MixerChannelView(m_channelAreaWidget, this, i);
		connectToSoloAndMute(i);
		chLayout->addWidget(m_mixerChannelViews[i]);
	}

	// add the scrolling section to the main layout
	// class solely for scroll area to pass key presses down
	class ChannelArea : public QScrollArea
	{
		public:
			ChannelArea(QWidget* parent, MixerView* mv) :
				QScrollArea(parent), m_mv(mv) {}
			~ChannelArea() override = default;
			void keyPressEvent(QKeyEvent* e) override
			{
				m_mv->keyPressEvent(e);
			}
		private:
			MixerView* m_mv;
	};
	channelArea = new ChannelArea(this, this);
	channelArea->setWidget(m_channelAreaWidget);
	channelArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	channelArea->setFrameStyle(QFrame::NoFrame);
	channelArea->setMinimumWidth(mixerChannelSize.width() * 6);
	channelArea->setWidgetResizable(true);

	int const scrollBarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
	channelArea->setMinimumHeight(mixerChannelSize.height() + scrollBarExtent);

	ml->addWidget(channelArea, 1);

	// show the add new mixer channel button
	auto newChannelBtn = new QPushButton(embed::getIconPixmap("new_channel"), QString(), this);
	newChannelBtn->setObjectName("newChannelBtn");
	newChannelBtn->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
	newChannelBtn->setFixedWidth(mixerChannelSize.width());
	newChannelBtn->setFocusPolicy(Qt::NoFocus);
	connect(newChannelBtn, SIGNAL(clicked()), this, SLOT(addNewChannel()));
	ml->addWidget(newChannelBtn, 0);


	// add the stacked layout for the effect racks of mixer channels
	ml->addWidget(m_racksWidget);

	setCurrentMixerChannel(m_mixerChannelViews[0]);

	updateGeometry();

	auto* mainWindow = getGUI()->mainWindow();

	// timer for updating faders
	connect(mainWindow, &MainWindow::periodicUpdate, this, &MixerView::updateFaders);

	// add ourself to workspace
	QMdiSubWindow* subWin = mainWindow->addWindowedWidget(this);
	layout()->setSizeConstraint(QLayout::SetMinimumSize);
	subWin->layout()->setSizeConstraint(QLayout::SetMinAndMaxSize);

	parentWidget()->setAttribute(Qt::WA_DeleteOnClose, false);
	parentWidget()->move(5, 310);

	// we want to receive dataChanged-signals in order to update
	setModel(mixer);
}




int MixerView::addNewChannel()
{
	// add new mixer channel and redraw the form.
	Mixer * mix = getMixer();

	int newChannelIndex = mix->createChannel();
	m_mixerChannelViews.push_back(new MixerChannelView(m_channelAreaWidget, this, newChannelIndex));
	connectToSoloAndMute(newChannelIndex);
	chLayout->addWidget(m_mixerChannelViews[newChannelIndex]);
	m_racksLayout->addWidget(m_mixerChannelViews[newChannelIndex]->m_effectRackView);

	updateMixerChannel(newChannelIndex);

	updateMaxChannelSelector();

	return newChannelIndex;
}


void MixerView::refreshDisplay()
{
	// delete all views and re-add them
	for (int i = 1; i<m_mixerChannelViews.size(); ++i)
	{
		// First disconnect from the solo/mute models.
		disconnectFromSoloAndMute(i);

		auto * mixerChannelView = m_mixerChannelViews[i];
		chLayout->removeWidget(mixerChannelView);
		m_racksLayout->removeWidget(mixerChannelView->m_effectRackView);

		delete mixerChannelView;
	}
	m_channelAreaWidget->adjustSize();

	// re-add the views
	m_mixerChannelViews.resize(getMixer()->numChannels());
	for (int i = 1; i < m_mixerChannelViews.size(); ++i)
	{
		m_mixerChannelViews[i] = new MixerChannelView(m_channelAreaWidget, this, i);
		connectToSoloAndMute(i);

		chLayout->addWidget(m_mixerChannelViews[i]);
		m_racksLayout->addWidget(m_mixerChannelViews[i]->m_effectRackView);
	}

	// set selected mixer channel to 0
	setCurrentMixerChannel(0);

	// update all mixer lines
	for (int i = 0; i < m_mixerChannelViews.size(); ++i)
	{
		updateMixerChannel(i);
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


void MixerView::saveSettings(QDomDocument& doc, QDomElement& domElement)
{
	MainWindow::saveWidgetState(this, domElement);
}




void MixerView::loadSettings(const QDomElement& domElement)
{
	MainWindow::restoreWidgetState(this, domElement);
}





void MixerView::toggledSolo()
{
	getMixer()->toggledSolo();

	updateAllMixerChannels();
}


void MixerView::toggledMute()
{
	updateAllMixerChannels();
}

Mixer* MixerView::getMixer() const
{
	return m_mixer;
}

void MixerView::updateAllMixerChannels()
{
	for (int i = 0; i < m_mixerChannelViews.size(); ++i)
	{
		m_mixerChannelViews[i]->update();
	}
}

void MixerView::connectToSoloAndMute(int channelIndex)
{
	auto * mixerChannel = getMixer()->mixerChannel(channelIndex);

	connect(&mixerChannel->m_muteModel, &BoolModel::dataChanged, this, &MixerView::toggledMute, Qt::DirectConnection);
	connect(&mixerChannel->m_soloModel, &BoolModel::dataChanged, this, &MixerView::toggledSolo, Qt::DirectConnection);
}

void MixerView::disconnectFromSoloAndMute(int channelIndex)
{
	auto * mixerChannel = getMixer()->mixerChannel(channelIndex);

	disconnect(&mixerChannel->m_muteModel, &BoolModel::dataChanged, this, &MixerView::toggledMute);
	disconnect(&mixerChannel->m_soloModel, &BoolModel::dataChanged, this, &MixerView::toggledSolo);
}


void MixerView::setCurrentMixerChannel(MixerChannelView* channel)
{
	// select
	m_currentMixerChannel = channel;
	m_racksLayout->setCurrentWidget(m_mixerChannelViews[channel->channelIndex()]->m_effectRackView);

	// set up send knob
	for (int i = 0; i < m_mixerChannelViews.size(); ++i)
	{
		updateMixerChannel(i);
	}
}


void MixerView::updateMixerChannel(int index)
{
	const auto mixer = getMixer();

	const auto currentIndex = m_currentMixerChannel->channelIndex();
	const auto thisLine = m_mixerChannelViews[index];
	thisLine->setToolTip(getMixer()->mixerChannel(index)->m_name);

	const auto sendModelCurrentToThis = mixer->channelSendModel(currentIndex, index);
	if (sendModelCurrentToThis == nullptr)
	{
		thisLine->m_sendKnob->setVisible(false);
		thisLine->m_sendArrow->setVisible(false);
	}
	else
	{
		thisLine->m_sendKnob->setVisible(true);
		thisLine->m_sendKnob->setModel(sendModelCurrentToThis);
		thisLine->m_sendArrow->setVisible(true);
	}

	const auto sendModelThisToCurrent = mixer->channelSendModel(index, currentIndex);
	if (sendModelThisToCurrent)
	{
		thisLine->m_receiveArrowOrSendButton->setVisible(true);
		thisLine->m_receiveArrowOrSendButton->setCurrentIndex(thisLine->m_receiveArrowStackedIndex);
	}
	else
	{
		thisLine->m_receiveArrowOrSendButton->setVisible(!mixer->isInfiniteLoop(currentIndex, index));
		thisLine->m_receiveArrowOrSendButton->setCurrentIndex(thisLine->m_sendButtonStackedIndex);
	}

	thisLine->m_sendButton->updateLightStatus();
	thisLine->m_renameLineEdit->setText(thisLine->elideName(thisLine->mixerChannel()->m_name));
	thisLine->update();
}


void MixerView::deleteChannel(int index)
{
	// can't delete master
	if (index == 0) return;

	// Disconnect from the solo/mute models of the channel we are about to delete
	disconnectFromSoloAndMute(index);

	// remember selected line
	int selLine = m_currentMixerChannel->channelIndex();

	Mixer* mixer = getMixer();
	// in case the deleted channel is soloed or the remaining
	// channels will be left in a muted state
	mixer->clearChannel(index);

	// delete the real channel
	mixer->deleteChannel(index);

	chLayout->removeWidget(m_mixerChannelViews[index]);
	m_racksLayout->removeWidget(m_mixerChannelViews[index]);
	// delete MixerChannelView later to prevent a crash when deleting from context menu
	m_mixerChannelViews[index]->hide();
	m_mixerChannelViews[index]->deleteLater();
	m_channelAreaWidget->adjustSize();

	// make sure every channel knows what index it is
	for (int i = index + 1; i < m_mixerChannelViews.size(); ++i)
	{
		m_mixerChannelViews[i]->setChannelIndex(i - 1);
	}
	m_mixerChannelViews.remove(index);

	// select the next channel
	if (selLine >= m_mixerChannelViews.size())
	{
		selLine = m_mixerChannelViews.size() - 1;
	}
	setCurrentMixerChannel(selLine);

	updateMaxChannelSelector();
}

void MixerView::deleteUnusedChannels()
{
	Mixer* mix = getMixer();

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
	if (index <= 1 || index >= m_mixerChannelViews.size()) return;

	Mixer *m = getMixer();

	// Move instruments channels
	m->moveChannelLeft(index);

	// Update widgets models
	m_mixerChannelViews[index]->setChannelIndex(index);
	m_mixerChannelViews[index - 1]->setChannelIndex(index - 1);

	// Focus on new position
	setCurrentMixerChannel(focusIndex);
}



void MixerView::moveChannelLeft(int index)
{
	moveChannelLeft(index, index - 1);
}



void MixerView::moveChannelRight(int index)
{
	moveChannelLeft(index + 1, index + 1);
}


void MixerView::renameChannel(int index)
{
	m_mixerChannelViews[index]->renameChannel();
}



void MixerView::keyPressEvent(QKeyEvent * e)
{
	auto adjustCurrentFader = [this](const Qt::KeyboardModifiers& modifiers, Fader::AdjustmentDirection direction)
	{
		auto* mixerChannel = currentMixerChannel();

		if (mixerChannel)
		{
			mixerChannel->fader()->adjust(modifiers, direction);
		}
	};

	switch(e->key())
	{
		case Qt::Key_Delete:
			deleteChannel(m_currentMixerChannel->channelIndex());
			break;
		case Qt::Key_Left:
			if (e->modifiers() & Qt::AltModifier)
			{
				moveChannelLeft(m_currentMixerChannel->channelIndex());
			}
			else
			{
				// select channel to the left
				setCurrentMixerChannel(m_currentMixerChannel->channelIndex() - 1);
			}
			break;
		case Qt::Key_Right:
			if (e->modifiers() & Qt::AltModifier)
			{
				moveChannelRight(m_currentMixerChannel->channelIndex());
			}
			else
			{
				// select channel to the right
				setCurrentMixerChannel(m_currentMixerChannel->channelIndex() + 1);
			}
			break;
		case Qt::Key_Up:
		case Qt::Key_Plus:
			adjustCurrentFader(e->modifiers(), Fader::AdjustmentDirection::Up);
			break;
		case Qt::Key_Down:
		case Qt::Key_Minus:
			adjustCurrentFader(e->modifiers(), Fader::AdjustmentDirection::Down);
			break;
		case Qt::Key_Insert:
			if (e->modifiers() & Qt::ShiftModifier)
			{
				addNewChannel();
			}
			break;
		case Qt::Key_Enter:
		case Qt::Key_Return:
		case Qt::Key_F2:
			renameChannel(m_currentMixerChannel->channelIndex());
			break;
		default:
			e->ignore();
			break;
	}
}



void MixerView::closeEvent(QCloseEvent * ce)
 {
	if (parentWidget())
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
	ce->ignore();
 }



void MixerView::setCurrentMixerChannel(int channel)
{
	if (channel >= 0 && channel < m_mixerChannelViews.size())
	{
		setCurrentMixerChannel(m_mixerChannelViews[channel]);
	}
}



void MixerView::clear()
{
	for (auto i = m_mixerChannelViews.size() - 1; i > 0; --i) { deleteChannel(i); }
	getMixer()->clearChannel(0);

	m_mixerChannelViews[0]->reset();

	refreshDisplay();
}




void MixerView::updateFaders()
{
	Mixer * m = getMixer();

	for (int i = 0; i < m_mixerChannelViews.size(); ++i)
	{
		const float opl = m_mixerChannelViews[i]->m_fader->getPeak_L();
		const float opr = m_mixerChannelViews[i]->m_fader->getPeak_R();
		const float fallOff = 1.25;
		if (m->mixerChannel(i)->m_peakLeft >= opl/fallOff)
		{
			m_mixerChannelViews[i]->m_fader->setPeak_L(m->mixerChannel(i)->m_peakLeft);
			// Set to -1 so later we'll know if this value has been refreshed yet.
			m->mixerChannel(i)->m_peakLeft = -1;
		}
		else if (m->mixerChannel(i)->m_peakLeft != -1)
		{
			m_mixerChannelViews[i]->m_fader->setPeak_L(opl/fallOff);
		}

		if (m->mixerChannel(i)->m_peakRight >= opr/fallOff)
		{
			m_mixerChannelViews[i]->m_fader->setPeak_R(m->mixerChannel(i)->m_peakRight);
			// Set to -1 so later we'll know if this value has been refreshed yet.
			m->mixerChannel(i)->m_peakRight = -1;
		}
		else if (m->mixerChannel(i)->m_peakRight != -1)
		{
			m_mixerChannelViews[i]->m_fader->setPeak_R(opr/fallOff);
		}
	}
}


} // namespace lmms::gui
