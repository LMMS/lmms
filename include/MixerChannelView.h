/*
 * MixerChannelView.h
 *
 * Copyright (c) 2024 saker
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

#ifndef LMMS_GUI_MIXER_CHANNEL_VIEW_H
#define LMMS_GUI_MIXER_CHANNEL_VIEW_H

#include <QWidget>

class QGraphicsView;
class QLabel;
class QLineEdit;
class QStackedWidget;

namespace lmms {
class MixerChannel;
}

namespace lmms::gui {
class AutomatableButton;
class EffectRackView;
class Fader;
class Knob;
class LcdWidget;
class MixerView;
class PeakIndicator;
class SendButtonIndicator;


class MixerChannelView : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QBrush backgroundActive READ backgroundActive WRITE setBackgroundActive)
	Q_PROPERTY(QColor strokeOuterActive READ strokeOuterActive WRITE setStrokeOuterActive)
	Q_PROPERTY(QColor strokeOuterInactive READ strokeOuterInactive WRITE setStrokeOuterInactive)
	Q_PROPERTY(QColor strokeInnerActive READ strokeInnerActive WRITE setStrokeInnerActive)
	Q_PROPERTY(QColor strokeInnerInactive READ strokeInnerInactive WRITE setStrokeInnerInactive)
public:
	MixerChannelView(QWidget* parent, MixerView* mixerView, int channelIndex);
	void paintEvent(QPaintEvent* event) override;
	void contextMenuEvent(QContextMenuEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseDoubleClickEvent(QMouseEvent*) override;
	void keyPressEvent(QKeyEvent* ke) override;

	void reset();
	int channelIndex() const { return m_channelIndex; }
	void setChannelIndex(int index);

	QBrush backgroundActive() const { return m_backgroundActive; }
	void setBackgroundActive(const QBrush& c) { m_backgroundActive = c; }

	QColor strokeOuterActive() const { return m_strokeOuterActive; }
	void setStrokeOuterActive(const QColor& c) { m_strokeOuterActive = c; }

	QColor strokeOuterInactive() const { return m_strokeOuterInactive; }
	void setStrokeOuterInactive(const QColor& c) { m_strokeOuterInactive = c; }

	QColor strokeInnerActive() const { return m_strokeInnerActive; }
	void setStrokeInnerActive(const QColor& c) { m_strokeInnerActive = c; }

	QColor strokeInnerInactive() const { return m_strokeInnerInactive; }
	void setStrokeInnerInactive(const QColor& c) { m_strokeInnerInactive = c; }

	Fader* fader() const { return m_fader; }

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
	QLabel* m_receiveArrow;
	QStackedWidget* m_receiveArrowOrSendButton;
	int m_receiveArrowStackedIndex = -1;
	int m_sendButtonStackedIndex = -1;

	Knob* m_sendKnob;
	LcdWidget* m_channelNumberLcd;
	QLineEdit* m_renameLineEdit;
	QGraphicsView* m_renameLineEditView;
	QLabel* m_sendArrow;
	AutomatableButton* m_muteButton;
	AutomatableButton* m_soloButton;
	PeakIndicator* m_peakIndicator = nullptr;
	Fader* m_fader;
	EffectRackView* m_effectRackView;
	MixerView* m_mixerView;
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

#endif // LMMS_GUI_MIXER_CHANNEL_VIEW_H
