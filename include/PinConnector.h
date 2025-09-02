/*
 * PinConnector.h - View for AudioPortsModel
 *
 * Copyright (c) 2025 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_GUI_PIN_CONNECTOR_H
#define LMMS_GUI_PIN_CONNECTOR_H

#include <optional>
#include <utility>
#include <QWidget>

#include "embed.h"
#include "lmms_export.h"
#include "AudioPortsModel.h"
#include "ModelView.h"

class QScrollArea;

namespace lmms
{

namespace gui
{

class SubWindow;

class LMMS_EXPORT PinConnector
	: public QWidget
	, public ModelView
{
	Q_OBJECT

	Q_PROPERTY(QColor backgroundColor MEMBER m_backgroundColor)
	Q_PROPERTY(QColor passthroughLineColor MEMBER m_passthroughLineColor)

public:
	explicit PinConnector(AudioPortsModel* model);
	~PinConnector() override;

	auto sizeHint() const -> QSize override;
	auto minimumSizeHint() const -> QSize override;

	void toggleVisibility();
	void closeWindow();

protected:
	void paintEvent(QPaintEvent* pe) override;

private:
	class MatrixView;

	auto trackChannelName(const AudioPortsModel& model, track_ch_t channel) const -> QString;
	auto getSpacerWidth() const -> int;
	auto getMaximumWindowSize() const -> QSize;
	void updateProperties();

	SubWindow* m_subWindow = nullptr;
	QScrollArea* m_scrollArea = nullptr;
	QWidget* m_spacer = nullptr;

	MatrixView* m_inView = nullptr;
	MatrixView* m_outView = nullptr;

	QColor m_backgroundColor;
	QColor m_passthroughLineColor;
};


class PinConnector::MatrixView : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(QColor lineColor MEMBER m_lineColor)
	Q_PROPERTY(QColor enabledColor MEMBER m_enabledColor)
	Q_PROPERTY(QColor disabledColor MEMBER m_disabledColor)

public:
	MatrixView(const PinConnector* view, AudioPortsModel::Matrix& matrix);
	~MatrixView() override = default;

	auto sizeHint() const -> QSize override;
	auto minimumSizeHint() const -> QSize override;
	void paintEvent(QPaintEvent*) override;
	void mouseMoveEvent(QMouseEvent* me) override;
	void mousePressEvent(QMouseEvent* me) override;

	//! Side length of square cells
	static constexpr auto cellSize() -> int
	{
		return s_pinSize + (s_pinInnerMargin * 2) + s_lineThickness;
	}

	static constexpr int BorderWidth = 1;

public slots:
	void updateProperties(const PinConnector* view);

private:
	auto getPin(const QPoint& mousePos) -> std::optional<std::pair<track_ch_t, proc_ch_t>>;
	auto getColor(track_ch_t trackChannel, proc_ch_t processorChannel) -> QColor;

	AudioPortsModel::Matrix* m_matrix = nullptr;

	QColor m_lineColor;
	QColor m_enabledColor;
	QColor m_disabledColor;

	static constexpr int s_pinSize = 13;
	static constexpr int s_pinInnerMargin = 1;
	static constexpr int s_lineThickness = 1;
};

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_PIN_CONNECTOR_H
