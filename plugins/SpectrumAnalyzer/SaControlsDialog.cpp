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

#include <QGridLayout>
#include <QLayout>
#include <QSizePolicy>
#include <QSplitter>
#include <QWidget>

#include "ComboBox.h"
#include "ComboBoxModel.h"
#include "embed.h"
#include "Engine.h"
#include "LedCheckbox.h"

#include "SaControls.h"
#include "SaSpectrumView.h"
#include "SaWaterfallView.h"


SaControlsDialog::SaControlsDialog(SaControls *controls, SaProcessor *processor) :
	EffectControlDialog(controls),
	m_controls(controls)
{
	// top level layout and 
	QBoxLayout * master_layout = new QHBoxLayout;
	QSplitter * display_splitter = new QSplitter(Qt::Vertical);
	master_layout->addWidget(display_splitter);
	window()->setLayout(master_layout);

	// config section
	QWidget * config_widget = new QWidget;				// wrapper for QSplitter
	QGridLayout * config_layout = new QGridLayout;
	config_widget->setLayout(config_layout);

	// populate config layout
	// display
	QLabel * displayLabel = new QLabel("Display", this);
	displayLabel->setStyleSheet("font-weight: bold");
	config_layout->addWidget(displayLabel, 0, 0);

	LedCheckBox * waterfallButton = new LedCheckBox("Waterfall diagram", this);
	waterfallButton->setCheckable(true);
	waterfallButton->setModel(&controls->m_waterfallModel);
	config_layout->addWidget(waterfallButton, 1, 0);

	LedCheckBox * smoothButton = new LedCheckBox("Averaging", this);
	smoothButton->setCheckable(true);
	smoothButton->setModel(&controls->m_smoothModel);
	config_layout->addWidget(smoothButton, 2, 0);

	LedCheckBox * logXButton = new LedCheckBox("Log. frequency", this);
	logXButton->setCheckable(true);
	logXButton->setModel(&controls->m_logXModel);
	config_layout->addWidget(logXButton, 3, 0);

	LedCheckBox * logYButton = new LedCheckBox("Log. amplitude", this);
	logYButton->setCheckable(true);
	logYButton->setModel(&controls->m_logYModel);
	config_layout->addWidget(logYButton, 4, 0);

	LedCheckBox * peakHoldButton = new LedCheckBox("Peak hold", this);
	peakHoldButton->setCheckable(true);
	peakHoldButton->setModel(&controls->m_peakHoldModel);
	config_layout->addWidget(peakHoldButton, 5, 0);

	LedCheckBox * pauseButton = new LedCheckBox("Pause", this);
	pauseButton->setCheckable(true);
	pauseButton->setModel(&controls->m_pauseModel);
	config_layout->addWidget(pauseButton, 6, 0);

	LedCheckBox * refFreezeButton = new LedCheckBox("Reference freeze", this, "", LedCheckBox::Red);
	refFreezeButton->setModel(&controls->m_refFreezeModel);
	config_layout->addWidget(refFreezeButton, 7, 0);

	// channels
	QLabel * channelsLabel = new QLabel("Channel", this);
	channelsLabel->setStyleSheet("font-weight: bold");
	config_layout->addWidget(channelsLabel, 0, 1);

	LedCheckBox * stereoButton = new LedCheckBox("Stereo", this);
	stereoButton->setCheckable(true);
	stereoButton->setModel(&controls->m_stereoModel);
	config_layout->addWidget(stereoButton, 1, 1);


	// FFT
	QLabel * fftLabel = new QLabel("FFT", this);
	fftLabel->setStyleSheet("font-weight: bold");
	config_layout->addWidget(fftLabel, 0, 2);

	ComboBox * blocksizeCombo = new ComboBox(this, "FFT Block Size");
	controls->m_blockSizeModel.addItem("256 (Fast, low-res)");
	controls->m_blockSizeModel.addItem("512");
	controls->m_blockSizeModel.addItem("1024");
	controls->m_blockSizeModel.addItem("2048 (Default)");
	controls->m_blockSizeModel.addItem("4096");
	controls->m_blockSizeModel.addItem("8192");
	controls->m_blockSizeModel.addItem("16384 (Slow, high-res)");
	controls->m_blockSizeModel.setValue(controls->m_blockSizeModel.findText("2048 (Default)"));
//	connect(&m_blocksizeModel, SIGNAL(dataChanged()), /*sa processor FFT blocksize updated*/this, SLOT( zoomingChanged() ) );
	blocksizeCombo->setModel(&controls->m_blockSizeModel);
	config_layout->addWidget(blocksizeCombo, 1, 2);

	// FFT window -- keep the same order as in the fft_helpers.h WINDOWS enum!
	ComboBox * windowCombo = new ComboBox(this, "FFT Window");
	controls->m_windowModel.addItem("Rectangular (Off)");
	controls->m_windowModel.addItem("Blackman-Harris (Default)");
	controls->m_windowModel.addItem("Hamming");
	controls->m_windowModel.addItem("Hanning");
	controls->m_windowModel.addItem("Kaiser");
	//
	windowCombo->setModel(&controls->m_windowModel);
	config_layout->addWidget(windowCombo, 2, 2);

	// create spectrum displays
	SaSpectrumView * spectrum = new SaSpectrumView(controls, processor, this);
	SaWaterfallView * waterfall = new SaWaterfallView(controls, processor, this);

	// add everything to top-level splitter
	display_splitter->addWidget(config_widget);
	display_splitter->addWidget(spectrum);
	display_splitter->addWidget(waterfall);

	window()->setBaseSize(500,500);
	window()->resize(500,500);
}

