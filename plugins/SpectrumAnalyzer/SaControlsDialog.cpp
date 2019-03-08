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

#include <QLayout>
#include <QFormLayout>
#include <QWidget>
#include <QSplitter>
#include <QSizePolicy>

#include "embed.h"
#include "Engine.h"
#include "LedCheckbox.h"

#include "SaControls.h"
#include "SaSpectrumView.h"


SaControlsDialog::SaControlsDialog(SaControls *controls, SaProcessor *processor) :
	EffectControlDialog(controls),
	m_controls(controls)
{
	setAutoFillBackground(true);
//	setMinimumSize(500, 250);
//	setMaximumSize(600, 500);

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);


	QVBoxLayout * master_layout = new QVBoxLayout;		// master
	master_layout->setSizeConstraint(QLayout::SetNoConstraint);

	QHBoxLayout * config_layout = new QHBoxLayout;		// to hold 3 "form" columns
	QFormLayout * config_column1 = new QFormLayout;
	QFormLayout * config_column2 = new QFormLayout;
	QFormLayout * config_column3 = new QFormLayout;
	QSplitter * display_splitter = new QSplitter(Qt::Vertical);	// to hold spectrum display and waterfall

	master_layout->addLayout(config_layout);
	master_layout->addWidget(display_splitter);
	config_layout->addLayout(config_column1);
	config_layout->addLayout(config_column2);
	config_layout->addLayout(config_column3);

	// add control buttons
	LedCheckBox * stereoButton = new LedCheckBox(this);
	stereoButton->setCheckable(true);
	stereoButton->setModel(&controls->m_stereoModel);
	QLabel * stereoLabel = new QLabel("Stereo display", this);
	config_column1->addRow(stereoButton, stereoLabel);

	LedCheckBox * smoothButton = new LedCheckBox(this);
	smoothButton->setCheckable(true);
	smoothButton->setModel(&controls->m_smoothModel);
	QLabel * smoothLabel = new QLabel("Smooth decay", this);
	config_column2->addRow(smoothButton, smoothLabel);

	LedCheckBox * waterfallButton = new LedCheckBox(this);
	waterfallButton->setCheckable(true);
	waterfallButton->setModel(&controls->m_waterfallModel);
	QLabel * waterfallLabel = new QLabel("Waterfall graph", this);
	config_column3->addRow(waterfallButton, waterfallLabel);

	LedCheckBox * logXButton = new LedCheckBox(this);
	logXButton->setCheckable(true);
	logXButton->setModel(&controls->m_logXModel);
	QLabel * logXLabel = new QLabel("Log. frequency", this);
	config_column1->addRow(logXButton, logXLabel);

	LedCheckBox * logYButton = new LedCheckBox(this);
	logYButton->setCheckable(true);
	logYButton->setModel(&controls->m_logYModel);
	QLabel * logYLabel = new QLabel("Log. intensity", this);
	config_column2->addRow(logYButton, logYLabel);

	// add spectrum display
	SaSpectrumView * spectrum = new SaSpectrumView(controls, processor, this);
	spectrum->setColors(QColor(51, 148, 204, 160), QColor(8, 108, 166, 100), QColor(166, 66, 8, 100));

	display_splitter->addWidget(spectrum);

	QLabel * placeholderLabel = new QLabel("placeholder / test", this);
	display_splitter->addWidget(placeholderLabel);

	setLayout(master_layout);

	// make control connections
	
}

