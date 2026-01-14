/*
 * SfzSamplerView.h - Declaration of class SfzSamplerView
 *
 * Copyright (c) 2023 Daniel Kauss Serna <daniel.kauss.serna@gmail.com>
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

#ifndef LMMS_GUI_SFZSAMPLER_VIEW_H
#define LMMS_GUI_SFZSAMPLER_VIEW_H

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

#include "InstrumentView.h"

class QPushButton;

namespace lmms {

class SfzSampler;

namespace gui {

class ComboBox;
class Knob;
class LcdSpinBox;
class PixmapButton;

class SfzControlsWidget : public QWidget
{
public:
	using QWidget::QWidget;
protected:
	void paintEvent(QPaintEvent* pe) override;
};

class SfzSidebarWidget : public QWidget
{
public:
	using QWidget::QWidget;
protected:
	void paintEvent(QPaintEvent* pe) override;
};

class SfzPianoWidget : public QWidget
{
public:
	using QWidget::QWidget;
protected:
	void paintEvent(QPaintEvent* pe) override;
};

class SfzSidebarGroupBox : public QFrame
{
public:
	SfzSidebarGroupBox(QWidget* parent = nullptr);
protected:
	void paintEvent(QPaintEvent* pe) override;
};

class SfzDividerLine : public QWidget
{
public:
	SfzDividerLine(QWidget* parent = nullptr, float lineWidth = 3, float dividerWidth = 11, bool horizontal = true);
protected:
	void paintEvent(QPaintEvent* pe) override;
	float m_lineWidth = 3;
	float m_dividerWidth = 11;
	bool m_horizontal = true;
};



class SfzSamplerView : public InstrumentView
{
	Q_OBJECT

public:
	SfzSamplerView(SfzSampler* instrument, QWidget* parent);

	static constexpr int s_controlsAreaWidth = 775; // Once parsing aria xml gui configurations is implemented, these hardcoded values will not be as needed
	static constexpr int s_controlsAreaHeight = 335;

	static constexpr int s_sidebarMinWidth = 200;

	static constexpr int s_keyboardAreaMinHeight = 50;

protected:
	void dragEnterEvent(QDragEnterEvent* dee) override;
	void dropEvent(QDropEvent* de) override;

	void paintEvent(QPaintEvent* pe) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	bool isResizable() const override { return true; }

	QPixmap m_logo;

	SfzSampler* m_instrument;

	QHBoxLayout* m_hBoxLayout;
	QVBoxLayout* m_vBoxLayout;
	SfzSidebarWidget* m_sidebarWidget;
	SfzControlsWidget* m_controlsWidget;
	SfzPianoWidget* m_pianoWidget;

	QBoxLayout* m_controlsLayout;

	QVBoxLayout* m_sidebarLayout;

	QLabel* m_emptyFileLabel;
};

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_SFZSAMPLER_VIEW_H
