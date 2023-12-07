/*
 * LadspaMatrixControlDialog.h - Dialog for displaying and editing control port
 *                               values for LADSPA plugins in a matrix display
 *
 * Copyright (c) 2015 Michael Gregorius <michaelgregorius/at/web[dot]de>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef LADSPA_MATRIX_CONTROL_DIALOG_H
#define LADSPA_MATRIX_CONTROL_DIALOG_H

#include "EffectControlDialog.h"
#include "LadspaControl.h"

#include "lmms_basics.h"


class QGridLayout;
class QScrollArea;

namespace lmms
{

class LadspaControls;

namespace gui
{

class LedCheckBox;


// see tap_reverb_presets.h
const int MaxTAPReverbTypes = 43;
const QString TAPReverbTypes[] {
	"AfterBurn",
	"AfterBurn(Long)",
	"Ambience",
	"Ambience(Thick)",
	"Ambience(Thick) HD",
	"Cathedral",
	"Cathedral HD",
	"Drum Chamber",
	"Garage",
	"Garage(Bright)",
	"Gymnasium",
	"Gymnasium(Bright)",
	"Gymnasium(Bright) HD",
	"Hall(Small)",
	"Hall(Medium)",
	"Hall(Large)",
	"Hall(Large) HD",
	"Plate(Small)",
	"Plate(Medium)",
	"Plate(Large)",
	"Plate(Large) HD",
	"PulseChamber",
	"PulseChamber(Reverse)",
	"Resonator(96 ms)",
	"Resonator(152 ms)",
	"Resonator(208 ms)",
	"Room(Small)",
	"Room(Medium)",
	"Room(Large)",
	"Room(Large) HD",
	"Slap Chamber",
	"Slap Chamber HD",
	"Slap Chamber(Bright)",
	"Slap Chamber(Bright) HD",
	"Smooth Hall(Small)",
	"Smooth Hall(Medium)",
	"Smooth Hall(Large)",
	"Smooth Hall(Large) HD",
	"Vocal Plate",
	"Vocal Plate HD",
	"Warble Chamber",
	"Warehouse",
	"Warehouse HD"
};
static_assert (std::end(TAPReverbTypes) - std::begin(TAPReverbTypes) == MaxTAPReverbTypes , "TAPReverbTypes should match NUM_MODES in tap_reverb_presets.h");


// see tap_dynamics_presets.h
const int MaxTAPDynamicsModes = 15;
const QString TAPDynamicsModes[] {
	"2:1 compression starting at -6 dB",
	"2:1 compression starting at -9 dB",
	"2:1 compression starting at -12 dB",
	"2:1 compression starting at -18 dB",
	"2.5:1 compression starting at -12 dB",
	"3:1 compression starting at -12 dB",
	"3:1 compression starting at -15 dB",
	"Compressor/Gate",
	"Expander",
	"Hard limiter at -6 dB",
	"Hard limiter at -12 dB",
	"Hard noise gate at -35 dB",
	"Soft limiter",
	"Soft knee comp/gate (-24 dB threshold)",
	"Soft noise gate below -36 dB",
};
static_assert (std::end(TAPDynamicsModes) - std::begin(TAPDynamicsModes) == MaxTAPDynamicsModes , "TAPDynamicsModes should match NUM_MODES in tap_dynamics_presets.h");



class LadspaMatrixControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	LadspaMatrixControlDialog(LadspaControls* ctl);
	bool isResizable() const override;

private slots:
	void updateEffectView(LadspaControls* ctl);

private:
	/**
	 * @brief Checks if a link column is needed for the current effect controls.
	 * @return true if a link column is needed.
	 */
	bool needsLinkColumn() const;

	/**
	 * @brief Arranges widgets for the current controls in a grid/matrix layout.
	 * @param parent The parent of all created widgets
	 * @param gridLayout The layout into which the controls are organized
	 */
	void arrangeControls(QWidget * parent, QGridLayout* gridLayout);

	/**
	 * @brief Creates a widget that holds the widgets of the current controls in a matrix arrangement.
	 * @param ladspaControls
	 * @return
	 */
	QWidget * createMatrixWidget();

	LadspaControls * getLadspaControls() const;
	ch_cnt_t getChannelCount() const;

	/**
	 * @brief for enum types show text values in the UI if known
	 */
	void addTextHints( QWidget * modelEditor, LadspaControl * ladspaControl );

private:
	QScrollArea* m_scrollArea;
	LedCheckBox* m_stereoLink;
};

} // namespace gui

} // namespace lmms

#endif
