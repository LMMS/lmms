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

#include "CaptionMenu.h"
#include "ColorChooser.h"
#include "GuiApplication.h"
#include "Mixer.h"
#include "MixerChannelView.h"
#include "MixerView.h"
#include "PeakIndicator.h"
#include "Song.h"
#include "ConfigManager.h"

#include "gui_templates.h"
#include "lmms_math.h"

#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QMenu>
#include <QPainter>
#include <QFont>
#include <QMessageBox>
#include <QCheckBox>

#include <cassert>

namespace lmms::gui
{
    MixerChannelView::MixerChannelView(QWidget* parent, MixerView* mixerView, int channelIndex) :
        QWidget(parent),
        m_mixerView(mixerView),
        m_channelIndex(channelIndex)
    {
        auto retainSizeWhenHidden = [](QWidget* widget)
        {
            auto sizePolicy = widget->sizePolicy();
            sizePolicy.setRetainSizeWhenHidden(true);
            widget->setSizePolicy(sizePolicy);
        };

        m_sendButton = new SendButtonIndicator{this, this, mixerView};
        retainSizeWhenHidden(m_sendButton);

        m_sendKnob = new Knob{KnobType::Bright26, this, tr("Channel send amount")};
        retainSizeWhenHidden(m_sendKnob);

        m_channelNumberLcd = new LcdWidget{2, this};
        m_channelNumberLcd->setValue(channelIndex);
        retainSizeWhenHidden(m_channelNumberLcd);

        const auto mixerChannel = Engine::mixer()->mixerChannel(channelIndex);
        const auto mixerName = mixerChannel->m_name;
        setToolTip(mixerName);

        m_renameLineEdit = new QLineEdit{mixerName, nullptr};
        m_renameLineEdit->setFixedWidth(65);
        m_renameLineEdit->setFont(adjustedToPixelSize(font(), 12));
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

        m_sendArrow = new QLabel{};
        m_sendArrow->setPixmap(embed::getIconPixmap("send_bg_arrow"));
        retainSizeWhenHidden(m_sendArrow);
        m_sendArrow->setVisible(m_sendReceiveState == SendReceiveState::SendToThis);

        m_receiveArrow = new QLabel{};
        m_receiveArrow->setPixmap(embed::getIconPixmap("receive_bg_arrow"));
        retainSizeWhenHidden(m_receiveArrow);
        m_receiveArrow->setVisible(m_sendReceiveState == SendReceiveState::ReceiveFromThis);

        m_muteButton = new PixmapButton(this, tr("Mute"));
        m_muteButton->setModel(&mixerChannel->m_muteModel);
        m_muteButton->setActiveGraphic(embed::getIconPixmap("led_off"));
        m_muteButton->setInactiveGraphic(embed::getIconPixmap("led_green"));
        m_muteButton->setCheckable(true);
        m_muteButton->setToolTip(tr("Mute this channel"));

        m_soloButton = new PixmapButton(this, tr("Solo"));
        m_soloButton->setModel(&mixerChannel->m_soloModel);
        m_soloButton->setActiveGraphic(embed::getIconPixmap("led_red"));
        m_soloButton->setInactiveGraphic(embed::getIconPixmap("led_off"));
        m_soloButton->setCheckable(true);
        m_soloButton->setToolTip(tr("Solo this channel"));        

        QVBoxLayout* soloMuteLayout = new QVBoxLayout();
        soloMuteLayout->setContentsMargins(0, 0, 0, 0);
        soloMuteLayout->setSpacing(0);
        soloMuteLayout->addWidget(m_soloButton, 0, Qt::AlignHCenter);
        soloMuteLayout->addWidget(m_muteButton, 0, Qt::AlignHCenter);

        m_fader = new Fader{&mixerChannel->m_volumeModel, tr("Fader %1").arg(channelIndex), this};

        m_peakIndicator = new PeakIndicator(this);
        connect(m_fader, &Fader::peakChanged, m_peakIndicator, &PeakIndicator::updatePeak);

        m_effectRackView = new EffectRackView{&mixerChannel->m_fxChain, mixerView->m_racksWidget};
        m_effectRackView->setFixedWidth(EffectRackView::DEFAULT_WIDTH);

        auto mainLayout = new QVBoxLayout{this};
        mainLayout->setContentsMargins(4, 4, 4, 4);
        mainLayout->addWidget(m_receiveArrow, 0, Qt::AlignHCenter);
        mainLayout->addWidget(m_sendButton, 0, Qt::AlignHCenter);
        mainLayout->addWidget(m_sendKnob, 0, Qt::AlignHCenter);
        mainLayout->addWidget(m_sendArrow, 0, Qt::AlignHCenter);
        mainLayout->addWidget(m_channelNumberLcd, 0, Qt::AlignHCenter);
        mainLayout->addWidget(m_renameLineEditView, 0, Qt::AlignHCenter);
        mainLayout->addLayout(soloMuteLayout, 0);
        mainLayout->addWidget(m_peakIndicator);
        mainLayout->addWidget(m_fader, 1, Qt::AlignHCenter);

        connect(m_renameLineEdit, &QLineEdit::editingFinished, this, &MixerChannelView::renameFinished);
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
            contextMenu->addAction(embed::getIconPixmap("cancel"), tr("R&emove channel"), this, &MixerChannelView::removeChannel);
            contextMenu->addSeparator();
        }

        contextMenu->addAction(embed::getIconPixmap("cancel"), tr("Remove &unused channels"), this, &MixerChannelView::removeUnusedChannels);
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

    void MixerChannelView::paintEvent(QPaintEvent* event)
    {
        auto * mixer = Engine::mixer();
        const auto channel = mixerChannel();
        const bool muted = channel->m_muteModel.value();
        const auto name = channel->m_name;
        const auto elidedName = elideName(name);
        const auto * mixerChannelView = m_mixerView->currentMixerChannel();
        const auto isActive = mixerChannelView == this;

        if (!m_inRename && m_renameLineEdit->text() != elidedName)
        {
            m_renameLineEdit->setText(elidedName);
        }

        const auto width = rect().width();
        const auto height = rect().height();
        auto painter = QPainter{this};

        if (channel->color().has_value() && !muted)
        {
            painter.fillRect(rect(), channel->color()->darker(isActive ? 120 : 150));
        }
        else
        {
            painter.fillRect(rect(), isActive ? backgroundActive().color() : painter.background().color());
        }

        // inner border
        painter.setPen(isActive ? strokeInnerActive() : strokeInnerInactive());
        painter.drawRect(1, 1, width - MIXER_CHANNEL_INNER_BORDER_SIZE, height - MIXER_CHANNEL_INNER_BORDER_SIZE);

        // outer border
        painter.setPen(isActive ? strokeOuterActive() : strokeOuterInactive());
        painter.drawRect(0, 0, width - MIXER_CHANNEL_OUTER_BORDER_SIZE, height - MIXER_CHANNEL_OUTER_BORDER_SIZE);

        const auto & currentMixerChannelIndex = mixerChannelView->m_channelIndex;
        const auto sendToThis = mixer->channelSendModel(currentMixerChannelIndex, m_channelIndex) != nullptr;
        const auto receiveFromThis = mixer->channelSendModel(m_channelIndex, currentMixerChannelIndex) != nullptr;
        const auto sendReceiveStateNone = !sendToThis && !receiveFromThis;

        // Only one or none of them can be on
        assert(sendToThis ^ receiveFromThis || sendReceiveStateNone);

        m_sendArrow->setVisible(sendToThis);
        m_receiveArrow->setVisible(receiveFromThis);

        if (sendReceiveStateNone)
        {
            setSendReceiveState(SendReceiveState::None);
        }
        else
        {
            setSendReceiveState(sendToThis ? SendReceiveState::SendToThis : SendReceiveState::ReceiveFromThis);
        }

        QWidget::paintEvent(event);
    }

    void MixerChannelView::mousePressEvent(QMouseEvent*)
    {
        if (m_mixerView->currentMixerChannel() != this)
        {
            m_mixerView->setCurrentMixerChannel(this);
        }
    }

    void MixerChannelView::mouseDoubleClickEvent(QMouseEvent*)
    {
        renameChannel();
    }

    bool MixerChannelView::eventFilter(QObject* dist, QEvent* event)
    {
        // If we are in a rename, capture the enter/return events and handle them
        if (event->type() == QEvent::KeyPress)
        {
            auto keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
            {
                if (m_inRename)
                {
                    renameFinished();
                    event->accept(); // Stop the event from propagating
                    return true;
                }
            }
        }
        return false;
    }

    int MixerChannelView::channelIndex() const
    {
        return m_channelIndex;
    }

    void MixerChannelView::setChannelIndex(int index)
    {
        MixerChannel* mixerChannel = Engine::mixer()->mixerChannel(index);
        m_fader->setModel(&mixerChannel->m_volumeModel);
        m_muteButton->setModel(&mixerChannel->m_muteModel);
        m_soloButton->setModel(&mixerChannel->m_soloModel);
        m_effectRackView->setModel(&mixerChannel->m_fxChain);
        m_channelNumberLcd->setValue(index);
        m_channelIndex = index;
    }

    MixerChannelView::SendReceiveState MixerChannelView::sendReceiveState() const
    {
        return m_sendReceiveState;
    }

    void MixerChannelView::setSendReceiveState(const SendReceiveState& state)
    {
        m_sendReceiveState = state;
        m_sendArrow->setVisible(state == SendReceiveState::SendToThis);
        m_receiveArrow->setVisible(state == SendReceiveState::ReceiveFromThis);
    }

    QBrush MixerChannelView::backgroundActive() const
    {
        return m_backgroundActive;
    }

    void MixerChannelView::setBackgroundActive(const QBrush& c)
    {
        m_backgroundActive = c;
    }

    QColor MixerChannelView::strokeOuterActive() const
    {
        return m_strokeOuterActive;
    }

    void MixerChannelView::setStrokeOuterActive(const QColor& c)
    {
        m_strokeOuterActive = c;
    }

    QColor MixerChannelView::strokeOuterInactive() const
    {
        return m_strokeOuterInactive;
    }

    void MixerChannelView::setStrokeOuterInactive(const QColor& c)
    {
        m_strokeOuterInactive = c;
    }

    QColor MixerChannelView::strokeInnerActive() const
    {
        return m_strokeInnerActive;
    }

    void MixerChannelView::setStrokeInnerActive(const QColor& c)
    {
        m_strokeInnerActive = c;
    }

    QColor MixerChannelView::strokeInnerInactive() const
    {
        return m_strokeInnerInactive;
    }

    void MixerChannelView::setStrokeInnerInactive(const QColor& c)
    {
        m_strokeInnerInactive = c;
    }

    void MixerChannelView::reset()
    {
        m_peakIndicator->resetPeakToMinusInf();
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
        const auto * colorChooser = ColorChooser{this}.withPalette(ColorChooser::Palette::Mixer);
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

} // namespace lmms::gui