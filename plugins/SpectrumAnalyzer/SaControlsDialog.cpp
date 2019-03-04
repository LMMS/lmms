/*
 * SaControlsDialog.cpp - definition of SaControlsDialog class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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


#include "SaControlsDialog.h"

#include <QGraphicsView>
#include <QLayout>
#include <QWidget>

#include "embed.h"
#include "Engine.h"
#include "LedCheckbox.h"

#include "SaControls.h"
#include "SaSpectrumView.h"


SaControlsDialog::SaControlsDialog(SaControls *controls) :
	EffectControlDialog(controls),
	m_controls(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(500, 500);

	// add spectrum display
	SaSpectrumView * spectrum = new SaSpectrumView(&controls->m_fftBands, this);
	spectrum->move(26, 17);
	spectrum->setColors(QColor(51, 148, 204, 160), QColor(8, 108, 166, 100), QColor(166, 66, 8, 100));

	// add control buttons
	LedCheckBox * stereoButton = new LedCheckBox(this);
	stereoButton->setCheckable(true);
	stereoButton->setModel(&controls->m_stereo);
	stereoButton->move(20, 240);

	LedCheckBox * smoothButton = new LedCheckBox(this);
	smoothButton->setCheckable(true);
	smoothButton->setModel(&controls->m_smooth);
	smoothButton->move(40, 240);

	LedCheckBox * waterfallButton = new LedCheckBox(this);
	waterfallButton->setCheckable(true);
	waterfallButton->setModel(&controls->m_waterfall);
	waterfallButton->move(60, 240);

	LedCheckBox * logXButton = new LedCheckBox(this);
	logXButton->setCheckable(true);
	logXButton->setModel(&controls->m_log_x);
	logXButton->move(80, 240);

	LedCheckBox * logYButton = new LedCheckBox(this);
	logYButton->setCheckable(true);
	logYButton->setModel(&controls->m_log_y);
	logYButton->move(100, 240);
}

