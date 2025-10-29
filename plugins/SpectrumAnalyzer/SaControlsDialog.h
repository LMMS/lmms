/*
 * SaControlsDialog.h - declatation of SaControlsDialog class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
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

#ifndef SACONTROLSDIALOG_H
#define SACONTROLSDIALOG_H

#include "EffectControlDialog.h"

namespace lmms
{
class SaControls;
class SaProcessor;
}


namespace lmms::gui
{

class SaSpectrumView;
class SaWaterfallView;

//! Top-level widget holding the configuration GUI and spectrum displays
class SaControlsDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	explicit SaControlsDialog(SaControls *controls, SaProcessor *processor);
	~SaControlsDialog() override = default;

	bool isResizable() const override {return true;}
	QSize sizeHint() const override;

private:
	SaControls *m_controls;
	SaProcessor *m_processor;

	// Pointers to created widgets are needed to keep track of their sizeHint() changes.
	// Config widget is a plain QWidget so it has just a fixed height instead.
	const int m_configHeight = 75;
	SaSpectrumView *m_spectrum;
	SaWaterfallView *m_waterfall;
};


} // namespace lmms::gui

#endif // SACONTROLSDIALOG_H
