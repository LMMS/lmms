/*
 * SlicerTUI.h - declaration of class SlicerTUI
 *
 * Copyright (c) 2006-2008 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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

#include "WaveForm.h"

#include <QPushButton>

#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "LcdSpinBox.h"


#ifndef SLICERT_UI_H
#define SLICERT_UI_H

namespace lmms
{

class SlicerT;

namespace gui
{


class SlicerTUI : public InstrumentViewFixedSize
{
	Q_OBJECT
	
public:
	SlicerTUI( SlicerT * instrument,
					QWidget * parent );
	~SlicerTUI() override = default;

protected slots:
	void exportMidi();
	//void sampleSizeChanged( float _new_sample_length );

protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );

private:
	SlicerT * m_slicerTParent;

	Knob m_noteThresholdKnob;
	Knob m_fadeOutKnob;
	LcdSpinBox m_bpmBox;

	QPushButton m_resetButton;
	QPushButton m_timeShiftButton;
	QPushButton m_midiExportButton;

	WaveForm m_wf;
};


} // namespace gui

} // namespace lmms

#endif