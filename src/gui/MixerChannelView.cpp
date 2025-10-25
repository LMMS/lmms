/*
 * MixerChannelView.h - the mixer channel view
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
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

#include "MixerChannelView.h"

#include <QCheckBox>
#include <QFont>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "AutomatableButton.h"
#include "CaptionMenu.h"
#include "ColorChooser.h"
#include "ConfigManager.h"
#include "EffectRackView.h"
#include "Fader.h"
#include "FontHelper.h"
#include "GuiApplication.h"
#include "Knob.h"
#include "LcdWidget.h"
#include "Mixer.h"
#include "MixerView.h"
#include "PeakIndicator.h"
#include "SendButtonIndicator.h"
#include "Song.h"

namespace lmms::gui {
MixerChannelView::MixerChannelView(QWidget* parent, MixerView* mixerView, int channelIndex)
	: QWidget(parent)
	, m_mixerView(mixerView)
	, m_channelIndex(channelIndex)
{
	auto retainSizeWhenHidden = [](QWidget* widget) {
		auto sizePolicy = widget->sizePolicy();
		sizePolicy.setRetainSizeWhenHidden(true);
		widget->setSizePolicy(sizePolicy);
	};

	auto receiveArrowContainer = new QWidget{};
	auto receiveArrowLayout = new QVBoxLayout{receiveArrowContainer};
	m_receiveArrow = new QLabel{};
	m_receiveArrow->setPixmap(embed::getIconPixmap("receive_bg_arrow"));
	receiveArrowLayout->setContentsMargins(0, 0, 0, 0);
	receiveArrowLayout->setSpacing(0);
	receiveArrowLayout->addWidget(m_receiveArrow, 0, Qt::AlignHCenter);

	auto sendButtonContainer = new QWidget{};
	auto sendButtonLayout = new QVBoxLayout{sendButtonContainer};
	m_sendButton = new SendButtonIndicator{this, this, mixerView};
	sendButtonLayout->setContentsMargins(0, 0, 0, 0);
	sendButtonLayout->setSpacing(0);
	sendButtonLayout->addWidget(m_sendButton, 0, Qt::AlignHCenter);

	m_receiveArrowOrSendButton = new QStackedWidget{this};
	m_receiveArrowStackedIndex = m_receiveArrowOrSendButton->addWidget(receiveArrowContainer);
	m_sendButtonStackedIndex = m_receiveArrowOrSendButton->addWidget(sendButtonContainer);
	retainSizeWhenHidden(m_receiveArrowOrSendButton);

	m_sendKnob = new Knob{KnobType::Bright26, this, tr("Channel send amount")};
	retainSizeWhenHidden(m_sendKnob);

	m_sendArrow = new QLabel{};
	m_sendArrow->setPixmap(embed::getIconPixmap("send_bg_arrow"));
	retainSizeWhenHidden(m_sendArrow);

	m_channelNumberLcd = new LcdWidget{2, this};
	m_channelNumberLcd->setValue(channelIndex);
	retainSizeWhenHidden(m_channelNumberLcd);

	const auto mixerChannel = Engine::mixer()->mixerChannel(channelIndex);
	const auto mixerName = mixerChannel->m_name;
	setToolTip(mixerName);

	m_renameLineEdit = new QLineEdit{mixerName, nullptr};
	m_renameLineEdit->setFixedWidth(65);
	m_renameLineEdit->setFont(adjustedToPixelSize(font(), LARGE_FONT_SIZE));
	m_renameLineEdit->setReadOnly(true);
	m_renameLineEdit->installEventFilter(this);

	auto renameLineEditScene = new QGraphicsScene{};
	m_renameLineEditView = new QGraphicsView{};
	m_renameLineEditView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_renameLineEditView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_renameLineEditView->setAttribute(Qt::WA_TransparentForMouseEvents, true);
	m_renameLineEditView->setScene(renameLineEditScene);

	auto renameLineEditProxy = renameLineEditScene->addWidget(m_renameLineEdit);
	renameLineEditProxy->setRotation(-90);
	m_renameLineEditView->setFixedSize(m_renameLineEdit->height() + 5, m_renameLineEdit->width() + 5);

	m_muteButton = new AutomatableButton(this, tr("Mute"));
	m_muteButton->setModel(&mixerChannel->m_muteModel);
	m_muteButton->setCheckable(true);
	m_muteButton->setObjectName("btn-mute");
	m_muteButton->setToolTip(tr("Mute this channel"));

	m_soloButton = new AutomatableButton(this, tr("Solo"));
	m_soloButton->setModel(&mixerChannel->m_soloModel);
	m_soloButton->setCheckable(true);
	m_soloButton->setObjectName("btn-solo");
	m_soloButton->setToolTip(tr("Solo this channel"));

	auto soloMuteLayout = new QVBoxLayout();
	soloMuteLayout->setContentsMargins(0, 2, 0, 2);
	soloMuteLayout->setSpacing(2);
	soloMuteLayout->addWidget(m_soloButton, 0, Qt::AlignHCenter);
	soloMuteLayout->addWidget(m_muteButton, 0, Qt::AlignHCenter);

	m_fader = new Fader{&mixerChannel->m_volumeModel, tr("Fader %1").arg(channelIndex), this};

	m_peakIndicator = new PeakIndicator(this);
	connect(m_fader, &Fader::peakChanged, m_peakIndicator, &PeakIndicator::updatePeak);

	m_effectRackView = new EffectRackView{&mixerChannel->m_fxChain, mixerView->m_racksWidget};
	m_effectRackView->setFixedWidth(EffectRackView::DEFAULT_WIDTH);

	auto mainLayout = new QVBoxLayout{this};
	mainLayout->setContentsMargins(4, 4, 4, 4);
	mainLayout->setSpacing(2);

	mainLayout->addWidget(m_receiveArrowOrSendButton, 0, Qt::AlignHCenter);
	mainLayout->addWidget(m_sendKnob, 0, Qt::AlignHCenter);
	mainLayout->addWidget(m_sendArrow, 0, Qt::AlignHCenter);
	mainLayout->addWidget(m_channelNumberLcd, 0, Qt::AlignHCenter);
	mainLayout->addWidget(m_renameLineEditView, 0, Qt::AlignHCenter);
	mainLayout->addLayout(soloMuteLayout);
	mainLayout->addWidget(m_peakIndicator);
	mainLayout->addWidget(m_fader, 1, Qt::AlignHCenter);

	connect(m_renameLineEdit, &QLineEdit::editingFinished, this, &MixerChannelView::renameFinished);

	setFocusPolicy(Qt::StrongFocus);
}

void MixerChannelView::contextMenuEvent(QContextMenuEvent*)
{
	auto contextMenu = new CaptionMenu(mixerChannel()->m_name, this);

	if (!isMasterChannel()) // no move-options in master
	{
		contextMenu->addAction(tr("Move &left"), this, &MixerChannelView::moveChannelLeft);
		contextMenu->addAction(tr("Move &right"), this, &MixerChannelView::moveChannelRight);
	}

	contextMenu->addAction(tr("Rename &channel"), this, &MixerChannelView::renameChannel);
	contextMenu->addSeparator();

	if (!isMasterChannel()) // no remove-option in master
	{
		contextMenu->addAction(
			embed::getIconPixmap("cancel"), tr("R&emove channel"), this, &MixerChannelView::removeChannel);
		contextMenu->addSeparator();
	}

	contextMenu->addAction(
		embed::getIconPixmap("cancel"), tr("Remove &unused channels"), this, &MixerChannelView::removeUnusedChannels);
	contextMenu->addSeparator();

	auto colorMenu = QMenu{tr("Color"), this};
	colorMenu.setIcon(embed::getIconPixmap("colorize"));
	colorMenu.addAction(tr("Change"), this, &MixerChannelView::selectColor);
	colorMenu.addAction(tr("Reset"), this, &MixerChannelView::resetColor);
	colorMenu.addAction(tr("Pick random"), this, &MixerChannelView::randomizeColor);
	contextMenu->addMenu(&colorMenu);

	contextMenu->exec(QCursor::pos());
	delete contextMenu;
}

void MixerChannelView::paintEvent(QPaintEvent*)
{
	static constexpr auto innerBorderSize = 3;
	static constexpr auto outerBorderSize = 1;

	const auto channel = mixerChannel();
	const auto isActive = m_mixerView->currentMixerChannel() == this;
	const auto width = rect().width();
	const auto height = rect().height();
	auto painter = QPainter{this};

	if (channel->color().has_value() && !channel->m_muteModel.value())
	{
		painter.fillRect(rect(), channel->color()->darker(isActive ? 120 : 150));
	}
	else { painter.fillRect(rect(), isActive ? backgroundActive().color() : painter.background().color()); }

	// inner border
	painter.setPen(isActive ? strokeInnerActive() : strokeInnerInactive());
	painter.drawRect(1, 1, width - innerBorderSize, height - innerBorderSize);

	// outer border
	painter.setPen(isActive ? strokeOuterActive() : strokeOuterInactive());
	painter.drawRect(0, 0, width - outerBorderSize, height - outerBorderSize);
}

void MixerChannelView::mousePressEvent(QMouseEvent*)
{
	if (m_mixerView->currentMixerChannel() != this) { m_mixerView->setCurrentMixerChannel(this); }
}

void MixerChannelView::mouseDoubleClickEvent(QMouseEvent*)
{
	renameChannel();
}

void MixerChannelView::keyPressEvent(QKeyEvent* ke)
{
	if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return)
	{
		if (m_inRename)
		{
			renameFinished();
		}
	}
	else
	{
		ke->ignore();
	}
}

void MixerChannelView::setChannelIndex(int index)
{
	MixerChannel* mixerChannel = Engine::mixer()->mixerChannel(index);
	m_fader->setModel(&mixerChannel->m_volumeModel);
	m_muteButton->setModel(&mixerChannel->m_muteModel);
	m_soloButton->setModel(&mixerChannel->m_soloModel);
	m_effectRackView->setModel(&mixerChannel->m_fxChain);
	m_channelNumberLcd->setValue(index);
	m_renameLineEdit->setText(elideName(mixerChannel->m_name));
	m_channelIndex = index;
}

void MixerChannelView::renameChannel()
{
	m_inRename = true;
	setToolTip("");
	m_renameLineEdit->setReadOnly(false);

	m_channelNumberLcd->hide();
	m_renameLineEdit->setFixedWidth(m_renameLineEdit->width());
	m_renameLineEdit->setText(mixerChannel()->m_name);

	m_renameLineEditView->setFocus();
	m_renameLineEdit->selectAll();
	m_renameLineEdit->setFocus();
}

void MixerChannelView::renameFinished()
{
	m_inRename = false;

	m_renameLineEdit->deselect();
	m_renameLineEdit->setReadOnly(true);
	m_renameLineEdit->setFixedWidth(m_renameLineEdit->width());
	m_channelNumberLcd->show();

	auto newName = m_renameLineEdit->text();
	setFocus();

	const auto mc = mixerChannel();
	if (!newName.isEmpty() && mc->m_name != newName)
	{
		mc->m_name = newName;
		m_renameLineEdit->setText(elideName(newName));
		Engine::getSong()->setModified();
	}

	setToolTip(mc->m_name);
}

void MixerChannelView::resetColor()
{
	mixerChannel()->setColor(std::nullopt);
	Engine::getSong()->setModified();
	update();
}

void MixerChannelView::selectColor()
{
	const auto channel = mixerChannel();

	const auto initialColor = channel->color().value_or(backgroundActive().color());
	const auto* colorChooser = ColorChooser{this}.withPalette(ColorChooser::Palette::Mixer);
	const auto newColor = colorChooser->getColor(initialColor);

	if (!newColor.isValid()) { return; }

	channel->setColor(newColor);

	Engine::getSong()->setModified();
	update();
}

void MixerChannelView::randomizeColor()
{
	auto channel = mixerChannel();
	channel->setColor(ColorChooser::getPalette(ColorChooser::Palette::Mixer)[rand() % 48]);
	Engine::getSong()->setModified();
	update();
}

bool MixerChannelView::confirmRemoval(int index)
{
	// if config variable is set to false, there is no need for user confirmation
	bool needConfirm = ConfigManager::inst()->value("ui", "mixerchanneldeletionwarning", "1").toInt();
	if (!needConfirm) { return true; }

	// is the channel is not in use, there is no need for user confirmation
	if (!getGUI()->mixerView()->getMixer()->isChannelInUse(index)) { return true; }

	QString messageRemoveTrack = tr("This Mixer Channel is being used.\n"
									"Are you sure you want to remove this channel?\n\n"
									"Warning: This operation can not be undone.");

	QString messageTitleRemoveTrack = tr("Confirm removal");
	QString askAgainText = tr("Don't ask again");
	auto askAgainCheckBox = new QCheckBox(askAgainText, nullptr);
	connect(askAgainCheckBox, &QCheckBox::stateChanged, [](int state) {
		// Invert button state, if it's checked we *shouldn't* ask again
		ConfigManager::inst()->setValue("ui", "mixerchanneldeletionwarning", state ? "0" : "1");
	});

	QMessageBox mb;
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

void MixerChannelView::removeChannel()
{
	if (!confirmRemoval(m_channelIndex)) { return; }
	auto mix = getGUI()->mixerView();
	mix->deleteChannel(m_channelIndex);
}

void MixerChannelView::removeUnusedChannels()
{
	auto mix = getGUI()->mixerView();
	mix->deleteUnusedChannels();
}

void MixerChannelView::moveChannelLeft()
{
	auto mix = getGUI()->mixerView();
	mix->moveChannelLeft(m_channelIndex);
}

void MixerChannelView::moveChannelRight()
{
	auto mix = getGUI()->mixerView();
	mix->moveChannelRight(m_channelIndex);
}

QString MixerChannelView::elideName(const QString& name)
{
	const auto maxTextHeight = m_renameLineEdit->width();
	const auto metrics = QFontMetrics{m_renameLineEdit->font()};
	const auto elidedName = metrics.elidedText(name, Qt::ElideRight, maxTextHeight);
	return elidedName;
}

MixerChannel* MixerChannelView::mixerChannel() const
{
	return Engine::mixer()->mixerChannel(m_channelIndex);
}

void MixerChannelView::reset()
{
	m_peakIndicator->resetPeakToMinusInf();
}

} // namespace lmms::gui
