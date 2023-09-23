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

#ifndef SLICERT_UI_H
#define SLICERT_UI_H

#include "WaveForm.h"

#include <QPushButton>

#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "PixmapButton.h"
#include "LcdSpinBox.h"


namespace lmms
{

class SlicerT;

namespace gui
{

class SlicerTKnob : public Knob {
	public:
		SlicerTKnob( QWidget * _parent ) :
				Knob( KnobType::Styled, _parent )
		{
		setFixedSize( 46, 40 );
		setCenterPointX( 23.0 );
		setCenterPointY( 15.0 );
		setInnerRadius( 3 );
		setOuterRadius( 11 );
		setLineWidth( 3 );
		setOuterColor( QColor(178, 115, 255) );
		}
};

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

	virtual void paintEvent(QPaintEvent * pe);

private:
	SlicerT * m_slicerTParent;

	SlicerTKnob m_noteThresholdKnob;
	SlicerTKnob m_fadeOutKnob;
	LcdSpinBox m_bpmBox;

	PixmapButton m_resetButton;
	PixmapButton m_midiExportButton;

	WaveForm m_wf;
};
} // namespace gui
} // namespace lmms
#endif // SLICERT_UI_H