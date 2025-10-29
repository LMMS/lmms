/*
 * EnvelopeAndLfoView.h - declaration of class EnvelopeAndLfoView which
 *                        is used by envelope/lfo/filter-tab of instrument track
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_ENVELOPE_AND_LFO_VIEW_H
#define LMMS_GUI_ENVELOPE_AND_LFO_VIEW_H

#include <QWidget>

#include "ModelView.h"

namespace lmms
{

class EnvelopeAndLfoParameters;

namespace gui
{

class AutomatableButtonGroup;
class Knob;
class LedCheckBox;
class PixmapButton;
class TempoSyncKnob;
class EnvelopeGraph;
class LfoGraph;



class EnvelopeAndLfoView : public QWidget, public ModelView
{
	Q_OBJECT
public:
	EnvelopeAndLfoView( QWidget * _parent );
	~EnvelopeAndLfoView() override;


protected:
	void modelChanged() override;

	void dragEnterEvent( QDragEnterEvent * _dee ) override;
	void dropEvent( QDropEvent * _de ) override;


protected slots:
	void lfoUserWaveChanged();


private:
	EnvelopeAndLfoParameters * m_params;

	// envelope stuff
	EnvelopeGraph* m_envelopeGraph;
	Knob * m_predelayKnob;
	Knob * m_attackKnob;
	Knob * m_holdKnob;
	Knob * m_decayKnob;
	Knob * m_sustainKnob;
	Knob * m_releaseKnob;
	Knob * m_amountKnob;

	// LFO stuff
	LfoGraph* m_lfoGraph;
	Knob * m_lfoPredelayKnob;
	Knob * m_lfoAttackKnob;
	TempoSyncKnob * m_lfoSpeedKnob;
	Knob * m_lfoAmountKnob;
	PixmapButton * m_userLfoBtn;
	AutomatableButtonGroup * m_lfoWaveBtnGrp;

	LedCheckBox * m_x100Cb;
	LedCheckBox * m_controlEnvAmountCb;
} ;

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_ENVELOPE_AND_LFO_VIEW_H
