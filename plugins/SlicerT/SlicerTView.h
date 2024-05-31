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

#include <QPushButton>

#include "ComboBox.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "LcdSpinBox.h"
#include "LedCheckBox.h"
#include "PixmapButton.h"
#include "SlicerTWaveform.h"

namespace lmms {

class SlicerT;

namespace gui {

class SlicerTView : public InstrumentViewFixedSize
{
	Q_OBJECT

public slots:
	void exportMidi();
	void openFiles();

public:
	SlicerTView(SlicerT* instrument, QWidget* parent);

	static constexpr int s_textBoxHeight = 20;
	static constexpr int s_textBoxWidth = 50;
	static constexpr int s_topTextY = 170;
	static constexpr int s_bottomTextY = 220;

protected:
	virtual void dragEnterEvent(QDragEnterEvent* dee);
	virtual void dropEvent(QDropEvent* de);

	virtual void paintEvent(QPaintEvent* pe);

private:
	SlicerT* m_slicerTParent;

	Knob* m_noteThresholdKnob;
	Knob* m_fadeOutKnob;
	LcdSpinBox* m_bpmBox;
	ComboBox* m_snapSetting;
	LedCheckBox* m_syncToggle;

	QPushButton* m_resetButton;
	QPushButton* m_midiExportButton;

	SlicerTWaveform* m_wf;

	Knob* createStyledKnob();
};
} // namespace gui
} // namespace lmms
#endif // LMMS_GUI_SLICERT_VIEW_H
