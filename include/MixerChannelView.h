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

#include <QGraphicsView>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QWidget>

namespace lmms
{
    class MixerChannel;
}

namespace lmms::gui
{
    constexpr int MIXER_CHANNEL_INNER_BORDER_SIZE = 3;
    constexpr int MIXER_CHANNEL_OUTER_BORDER_SIZE = 1;

    class MixerChannelView : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(QBrush backgroundActive READ backgroundActive WRITE setBackgroundActive)
        Q_PROPERTY(QColor strokeOuterActive READ strokeOuterActive WRITE setStrokeOuterActive)
        Q_PROPERTY(QColor strokeOuterInactive READ strokeOuterInactive WRITE setStrokeOuterInactive)
        Q_PROPERTY(QColor strokeInnerActive READ strokeInnerActive WRITE setStrokeInnerActive)
        Q_PROPERTY(QColor strokeInnerInactive READ strokeInnerInactive WRITE setStrokeInnerInactive)
    public:
        enum class SendReceiveState
        {
            None, SendToThis, ReceiveFromThis
        };

        MixerChannelView(QWidget* parent, MixerView* mixerView, int channelIndex);
        void paintEvent(QPaintEvent* event) override;
        void contextMenuEvent(QContextMenuEvent*) override;
        void mousePressEvent(QMouseEvent*) override;
        void mouseDoubleClickEvent(QMouseEvent*) override;
        bool eventFilter(QObject* dist, QEvent* event) override;

        int channelIndex() const;
        void setChannelIndex(int index);

        SendReceiveState sendReceiveState() const;
        void setSendReceiveState(const SendReceiveState& state);

        QBrush backgroundActive() const;
        void setBackgroundActive(const QBrush& c);

        QColor strokeOuterActive() const;
        void setStrokeOuterActive(const QColor& c);

        QColor strokeOuterInactive() const;
        void setStrokeOuterInactive(const QColor& c);

        QColor strokeInnerActive() const;
        void setStrokeInnerActive(const QColor& c);

        QColor strokeInnerInactive() const;
        void setStrokeInnerInactive(const QColor& c);

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
        bool confirmRemoval(int index);
        QString elideName(const QString& name);
        MixerChannel* mixerChannel() const;
        auto isMasterChannel() const -> bool { return m_channelIndex == 0; }

    private:
        SendButtonIndicator* m_sendButton;
        Knob* m_sendKnob;
        LcdWidget* m_channelNumberLcd;
        QLineEdit* m_renameLineEdit;
        QGraphicsView* m_renameLineEditView;
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

        QBrush m_backgroundActive;
        QColor m_strokeOuterActive;
        QColor m_strokeOuterInactive;
        QColor m_strokeInnerActive;
        QColor m_strokeInnerInactive;

        friend class MixerView;
    };
} // namespace lmms::gui

#endif