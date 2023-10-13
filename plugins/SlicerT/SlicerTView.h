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

#ifndef LMMS_SLICERT_VIEW_H
#define LMMS_SLICERT_VIEW_H

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

// style knob, defined in data/themes/default/style.css#L949
class SlicerTKnob : public Knob
{
public:
	SlicerTKnob(QWidget* _parent)
		: Knob(KnobType::Styled, _parent)
	{
		setFixedSize(50, 40);
		setCenterPointX(24.0);
		setCenterPointY(15.0);
	}
};

class SlicerTView : public InstrumentViewFixedSize
{
	Q_OBJECT

protected slots:
	void exportMidi();

public:
	SlicerTView(SlicerT* instrument, QWidget* parent);

protected:
	virtual void dragEnterEvent(QDragEnterEvent* _dee);
	virtual void dropEvent(QDropEvent* _de);

	virtual void paintEvent(QPaintEvent* pe);

private:
	SlicerT* m_slicerTParent;

	// lmms UI
	SlicerTKnob m_noteThresholdKnob;
	SlicerTKnob m_fadeOutKnob;
	LcdSpinBox m_bpmBox;
	ComboBox m_snapSetting;
	LedCheckBox m_syncToggle;

	// buttons
	PixmapButton m_resetButton;
	PixmapButton m_midiExportButton;

	SlicerTWaveform m_wf;
};
} // namespace gui
} // namespace lmms
#endif // LMMS_SLICERT_VIEW_H
