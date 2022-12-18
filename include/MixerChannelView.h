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

#ifndef MIXER_CHANNEL_VIEW_H
#define MIXER_CHANNEL_VIEW_H

#include "EffectRackView.h"
#include "Fader.h"
#include "Knob.h"
#include "LcdWidget.h"
#include "PixmapButton.h"
#include "SendButtonIndicator.h"

#include <QWidget>
#include <QPixmap>
#include <QLabel>
#include <QLineEdit>

namespace lmms::gui 
{
    constexpr int MIXER_CHANNEL_WIDTH = 53;
    constexpr int MIXER_CHANNEL_HEIGHT = 287;
    constexpr int MIXER_CHANNEL_INNER_BORDER_SIZE = 3;
    constexpr int MIXER_CHANNEL_OUTER_BORDER_SIZE = 1;
    
    class MixerChannelView : public QWidget
    {
        Q_OBJECT
    public:
        enum class SendReceiveState 
        {
            None, Send, Receive
        };

        MixerChannelView(int channelIndex, MixerView* mixerView, QWidget* parent = nullptr);
        void contextMenuEvent(QContextMenuEvent*) override;
        void mousePressEvent(QMouseEvent*) override;
	    void mouseDoubleClickEvent(QMouseEvent*) override;

        int channelIndex() const;
        void setChannelIndex(int index);

        SendReceiveState sendReceiveState() const;
        void setSendReceiveState(const SendReceiveState& state);
        
        QSize sizeHint() const override;
        bool eventFilter (QObject *dist, QEvent *event) override;
    
    public slots:
        void renameChannel();
        void resetColor();
        void selectColor();
        void randomizeColor();

    private slots:
        void renameFinished();
        void removeChannel();
        void removeUnusedChannels();
        void moveChannelLeft();
        void moveChannelRight();

    private:
        QString elideName(const QString& name);

    private:
        SendButtonIndicator* m_sendButton;
        Knob* m_sendKnob;
        LcdWidget* m_channelNumberLcd;
        QLineEdit* m_renameLineEdit;
        QLabel* m_sendArrow;
        QLabel* m_receiveArrow;
        PixmapButton* m_muteButton;
        PixmapButton* m_soloButton;
        Fader* m_fader;
        EffectRackView* m_effectRackView;
        MixerView* m_mixerView;
        SendReceiveState m_sendReceiveState = SendReceiveState::None;
        int m_channelIndex = 0;
        bool m_inRename = false;
        friend class MixerView;
    };
} // namespace lmms::gui

#endif