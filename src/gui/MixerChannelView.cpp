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
#include "Mixer.h"
#include "MixerChannelView.h"
#include "MixerView.h"
#include "gui_templates.h"
#include "lmms_math.h"

#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMenu>
#include <QPainter>

namespace lmms::gui
{
    MixerChannelView::MixerChannelView(int channelIndex, MixerView* mixerView, QWidget* parent) : 
        QWidget(parent),
        m_mixerView(mixerView),
        m_channelIndex(channelIndex)
    {
        // assert(dynamic_cast<MixerView*>(parent) != nullptr);

        m_sendButton = new SendButtonIndicator{nullptr, this, mixerView}; 
        m_sendKnob = new Knob{knobBright_26, nullptr, tr("Channel send amount")};

        m_channelNumberLcd = new LcdWidget{2, nullptr};
        m_channelNumberLcd->setValue(channelIndex);

        auto mixerChannel = Engine::mixer()->mixerChannel(channelIndex);
        auto mixerName = mixerChannel->m_name;
    	setToolTip(mixerName);

        m_renameLineEdit = new QLineEdit{};
        m_renameLineEdit->setText(mixerName);
        m_renameLineEdit->setFont(pointSizeF(font(), 7.5f));
    	m_renameLineEdit->installEventFilter(this);

        auto renameLineEditScene = new QGraphicsScene{};        
        auto renameLineEditView = new QGraphicsView{};
        renameLineEditView->setStyleSheet("border-style: none; background: transparent;");
        renameLineEditView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        renameLineEditView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        renameLineEditView->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        renameLineEditView->setScene(renameLineEditScene);
        
        auto renameLineEditProxy = renameLineEditScene->addWidget(m_renameLineEdit);
        renameLineEditProxy->setRotation(-90);

        m_sendArrow = new QLabel{};
        m_sendArrow->setPixmap(embed::getIconPixmap("send_bg_arrow"));
        m_sendArrow->setVisible(m_sendReceiveState == SendReceiveState::Send);

        m_receiveArrow = new QLabel{};
        m_receiveArrow->setPixmap(embed::getIconPixmap("receive_bg_arrow"));
        m_receiveArrow->setVisible(m_sendReceiveState == SendReceiveState::Receive);

        m_muteButton = new PixmapButton(nullptr, tr("Mute"));
        m_muteButton->setModel(&mixerChannel->m_muteModel);
        m_muteButton->setActiveGraphic(embed::getIconPixmap("led_off"));
        m_muteButton->setInactiveGraphic(embed::getIconPixmap("led_green"));
        m_muteButton->setCheckable(true);
        m_muteButton->setToolTip(tr("Mute this channel"));

        m_soloButton = new PixmapButton(nullptr, tr("Solo"));
        m_soloButton->setModel(&mixerChannel->m_soloModel);
        m_soloButton->setActiveGraphic(embed::getIconPixmap("led_red"));
        m_soloButton->setInactiveGraphic(embed::getIconPixmap("led_off"));
        m_soloButton->setCheckable(true);
        m_soloButton->setToolTip(tr("Solo this channel"));
        connect(&mixerChannel->m_soloModel, SIGNAL(dataChanged()), mixerView, SLOT(toggledSolo()), Qt::DirectConnection);

        m_fader = new Fader{&mixerChannel->m_volumeModel, tr("Fader %1").arg(channelIndex), nullptr};
        m_fader->setLevelsDisplayedInDBFS();
        m_fader->setMinPeak(dbfsToAmp(-42));
        m_fader->setMaxPeak(dbfsToAmp(9));
        
        m_effectRackView = new EffectRackView{&mixerChannel->m_fxChain, mixerView->m_racksWidget};
        m_effectRackView->setFixedSize(EffectRackView::DEFAULT_WIDTH, MIXER_CHANNEL_HEIGHT);

        auto mainLayout = new QVBoxLayout{this};
        mainLayout->addWidget(m_receiveArrow, 0, Qt::AlignHCenter);
        mainLayout->addWidget(m_sendButton, 0, Qt::AlignHCenter);
        mainLayout->addWidget(m_sendKnob, 0, Qt::AlignHCenter);
        mainLayout->addWidget(m_sendArrow, 0, Qt::AlignHCenter);
        mainLayout->addWidget(m_channelNumberLcd, 0, Qt::AlignHCenter);
        mainLayout->addStretch();
        mainLayout->addWidget(renameLineEditView, 0, Qt::AlignHCenter);
        mainLayout->addStretch();
        mainLayout->addWidget(m_soloButton, 0, Qt::AlignHCenter);
        mainLayout->addWidget(m_muteButton, 0, Qt::AlignHCenter);
        mainLayout->addWidget(m_fader, 0, Qt::AlignHCenter);

        connect(m_renameLineEdit, &QLineEdit::editingFinished, this, &MixerChannelView::renameFinished);
        connect(&Engine::mixer()->mixerChannel(m_channelIndex)->m_muteModel, SIGNAL(dataChanged()), this, SLOT(update()));
    }

    void MixerChannelView::contextMenuEvent(QContextMenuEvent*)
    {
        QPointer<CaptionMenu> contextMenu = new CaptionMenu( Engine::mixer()->mixerChannel( m_channelIndex )->m_name, this );
        if( m_channelIndex != 0 ) // no move-options in master
        {
            contextMenu->addAction( tr( "Move &left" ),	this, SLOT(moveChannelLeft()));
            contextMenu->addAction( tr( "Move &right" ), this, SLOT(moveChannelRight()));
        }
        contextMenu->addAction( tr( "Rename &channel" ), this, SLOT(renameChannel()));
        contextMenu->addSeparator();

        if( m_channelIndex != 0 ) // no remove-option in master
        {
            contextMenu->addAction( embed::getIconPixmap( "cancel" ), tr( "R&emove channel" ), this, SLOT(removeChannel()));
            contextMenu->addSeparator();
        }
        contextMenu->addAction( embed::getIconPixmap( "cancel" ), tr( "Remove &unused channels" ), this, SLOT(removeUnusedChannels()));
        contextMenu->addSeparator();

        QMenu colorMenu(tr("Color"), this);
        colorMenu.setIcon(embed::getIconPixmap("colorize"));
        colorMenu.addAction(tr("Change"), this, SLOT(selectColor()));
        colorMenu.addAction(tr("Reset"), this, SLOT(resetColor()));
        colorMenu.addAction(tr("Pick random"), this, SLOT(randomizeColor()));
        contextMenu->addMenu(&colorMenu);

        contextMenu->exec( QCursor::pos() );
        delete contextMenu;
    }

    void MixerChannelView::mousePressEvent(QMouseEvent*) 
    {

    }
    
    void MixerChannelView::mouseDoubleClickEvent(QMouseEvent*) 
    {

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
        MixerChannel* mixerChannel = Engine::mixer()->mixerChannel( index );
        m_fader->setModel( &mixerChannel->m_volumeModel );
        m_muteButton->setModel(&mixerChannel->m_muteModel);
        m_soloButton->setModel(&mixerChannel->m_soloModel);
        m_effectRackView->setModel(&mixerChannel->m_fxChain);
        m_channelIndex = index;
    }

    MixerChannelView::SendReceiveState MixerChannelView::sendReceiveState() const 
    {
        return m_sendReceiveState;
    }

    void MixerChannelView::setSendReceiveState(const SendReceiveState& state) 
    {
        m_sendReceiveState = state;        
        switch (state) 
        {
            case SendReceiveState::Send:
                break;
            case SendReceiveState::Receive:
                break;
        }
    }
    
    QSize MixerChannelView::sizeHint() const
    {
        return QSize{MIXER_CHANNEL_WIDTH, MIXER_CHANNEL_HEIGHT};
    }

    void MixerChannelView::renameChannel() 
    {

    }

    void MixerChannelView::renameFinished() 
    {

    }

    void MixerChannelView::resetColor() 
    {

    }

    void MixerChannelView::selectColor() 
    {

    }

    void MixerChannelView::randomizeColor() 
    {

    }

    void MixerChannelView::removeChannel() 
    {

    }

    void MixerChannelView::removeUnusedChannels() 
    {

    }

    void MixerChannelView::moveChannelLeft() 
    {

    }

    void MixerChannelView::moveChannelRight() 
    {

    }
    
} // namespace lmms::gui