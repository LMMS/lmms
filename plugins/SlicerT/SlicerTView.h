/*
 * SlicerTView.h - declaration of class SlicerTView
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

#ifndef LMMS_GUI_SLICERT_VIEW_H
#define LMMS_GUI_SLICERT_VIEW_H


#include "InstrumentView.h"

class QPushButton;

namespace lmms {

class SlicerT;

namespace gui {

class ComboBox;
class Knob;
class LcdSpinBox;
class PixmapButton;
class SlicerTWaveform;

class SlicerTView : public InstrumentView
{
	Q_OBJECT

public slots:
	void exportMidi();
	void openFiles();
	void clearSlices();

public:
	SlicerTView(SlicerT* instrument, QWidget* parent);

	static constexpr int s_textBoxHeight = 20;
	static constexpr int s_textBoxWidth = 50;

	static constexpr int s_topBarHeight = 50;
	static constexpr int s_bottomBoxHeight = 97;
	static constexpr int s_bottomBoxOffset = 65;
	static constexpr int s_sampleBoxHeight = 14;
	static constexpr int s_folderButtonWidth = 15;
	static constexpr int s_leftBoxWidth = 400;


	static constexpr int s_x1 = 35;
	static constexpr int s_x2 = 85;
	static constexpr int s_x3 = 160;
	static constexpr int s_x4 = 190;
	static constexpr int s_x5 = 275;
	static constexpr int s_x6 = 325;
protected:
	void dragEnterEvent(QDragEnterEvent* dee) override;
	void dropEvent(QDropEvent* de) override;

	void paintEvent(QPaintEvent* pe) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	bool isResizable() const override { return true; }

	SlicerT* m_slicerTParent;

	Knob* m_noteThresholdKnob;
	Knob* m_fadeOutKnob;
	LcdSpinBox* m_bpmBox;
	ComboBox* m_snapSetting;
	PixmapButton* m_syncToggle;
	PixmapButton* m_clearButton;
	PixmapButton* m_folderButton;

	QPushButton* m_resetButton;
	QPushButton* m_midiExportButton;

	SlicerTWaveform* m_wf;

	Knob* createStyledKnob();

	QPixmap m_fullLogo;
	QPixmap m_background;


	int m_y1;
	int m_y2;
};
} // namespace gui
} // namespace lmms
#endif // LMMS_GUI_SLICERT_VIEW_H
