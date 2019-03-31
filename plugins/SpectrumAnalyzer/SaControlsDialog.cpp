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
#include "PixmapButton.h"

#include "SaControls.h"
#include "SaSpectrumView.h"
#include "SaProcessor.h"
#include "SaWaterfallView.h"


SaControlsDialog::SaControlsDialog(SaControls *controls, SaProcessor *processor) :
	EffectControlDialog(controls),
	m_controls(controls),
	m_processor(processor)
{
	// top level layout and 
	QBoxLayout *master_layout = new QHBoxLayout;
	QSplitter *display_splitter = new QSplitter(Qt::Vertical);
	master_layout->addWidget(display_splitter);
	window()->setLayout(master_layout);

	// config section
	QWidget *config_widget = new QWidget;				// wrapper for QSplitter
	QGridLayout *config_layout = new QGridLayout;
	config_widget->setLayout(config_layout);

	// populate config layout
	float iconSize = 22.0 * window()->devicePixelRatio();

	// pause and freeze
	PixmapButton *pauseButton = new PixmapButton(this, tr("Pause"));
	QPixmap *pauseOnPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("play").scaled(1.2 * iconSize, 1.2 * iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	QPixmap *pauseOffPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("pause").scaled(1.2 * iconSize, 1.2 * iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	pauseOnPixmap->setDevicePixelRatio(window()->devicePixelRatio());
	pauseOffPixmap->setDevicePixelRatio(window()->devicePixelRatio());
	pauseButton->setActiveGraphic(*pauseOnPixmap);
	pauseButton->setInactiveGraphic(*pauseOffPixmap);
	pauseButton->setCheckable(true);
	pauseButton->setModel(&controls->m_pauseModel);
	config_layout->addWidget(pauseButton, 0, 0, 2, 1);

	PixmapButton *refFreezeButton = new PixmapButton(this, tr("Reference freeze"));
	QPixmap *freezeOnPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("freeze").scaled(1.2 * iconSize, 1.2 * iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	QPixmap *freezeOffPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("freeze_off").scaled(1.2 * iconSize, 1.2 * iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	freezeOnPixmap->setDevicePixelRatio(window()->devicePixelRatio());
	freezeOffPixmap->setDevicePixelRatio(window()->devicePixelRatio());
	refFreezeButton->setActiveGraphic(*freezeOnPixmap);
	refFreezeButton->setInactiveGraphic(*freezeOffPixmap);
	refFreezeButton->setCheckable(true);
	refFreezeButton->setModel(&controls->m_refFreezeModel);
	config_layout->addWidget(refFreezeButton, 2, 0, 2, 1);


	// display
	LedCheckBox *waterfallButton = new LedCheckBox(tr("Waterfall"), this);
	waterfallButton->setCheckable(true);
	waterfallButton->setMinimumSize(70, 12);
	waterfallButton->setModel(&controls->m_waterfallModel);
	config_layout->addWidget(waterfallButton, 0, 1);

	LedCheckBox *smoothButton = new LedCheckBox(tr("Averaging"), this);
	smoothButton->setCheckable(true);
	smoothButton->setMinimumSize(70, 12);
	smoothButton->setModel(&controls->m_smoothModel);
	config_layout->addWidget(smoothButton, 1, 1);

	LedCheckBox *stereoButton = new LedCheckBox("Stereo", this);
	stereoButton->setCheckable(true);
	stereoButton->setMinimumSize(70, 12);
	stereoButton->setModel(&controls->m_stereoModel);
	config_layout->addWidget(stereoButton, 2, 1);

	LedCheckBox *peakHoldButton = new LedCheckBox(tr("Peak hold"), this);
	peakHoldButton->setCheckable(true);
	peakHoldButton->setMinimumSize(70, 12);
	peakHoldButton->setModel(&controls->m_peakHoldModel);
	config_layout->addWidget(peakHoldButton, 3, 1);


	// ranges
	PixmapButton *logXButton = new PixmapButton(this, tr("Log. frequency"));
	QPixmap *logXOnPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("x_log").scaled(iconSize, iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	QPixmap *logXOffPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("x_linear").scaled(iconSize, iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	logXOnPixmap->setDevicePixelRatio(window()->devicePixelRatio());
	logXOffPixmap->setDevicePixelRatio(window()->devicePixelRatio());
	logXButton->setActiveGraphic(*logXOnPixmap);
	logXButton->setInactiveGraphic(*logXOffPixmap);
	logXButton->setCheckable(true);
	logXButton->setModel(&controls->m_logXModel);
	config_layout->addWidget(logXButton, 0, 2, 2, 1, Qt::AlignRight);

	ComboBox *freqRangeCombo = new ComboBox(this, tr("Frequency range"));
	freqRangeCombo->setMinimumSize(100, 22);
	freqRangeCombo->setMaximumSize(200, 22);
	freqRangeCombo->setModel(&controls->m_freqRangeModel);
	controls->m_freqRangeModel.addItem(tr("Full (auto)"));
	controls->m_freqRangeModel.addItem(tr("Audible"));
	controls->m_freqRangeModel.addItem(tr("Bass"));
	controls->m_freqRangeModel.addItem(tr("Mids"));
	controls->m_freqRangeModel.addItem(tr("High"));
	if (!controls->m_loaded) {controls->m_freqRangeModel.setValue(controls->m_freqRangeModel.findText(tr("Full (auto)")));}
	config_layout->addWidget(freqRangeCombo, 0, 3, 2, 1);

	PixmapButton *logYButton = new PixmapButton(this, tr("Log. amplitude"));
	QPixmap *logYOnPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("y_log").scaled(iconSize, iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	QPixmap *logYOffPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("y_linear").scaled(iconSize, iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	logYOnPixmap->setDevicePixelRatio(window()->devicePixelRatio());
	logYOffPixmap->setDevicePixelRatio(window()->devicePixelRatio());
	logYButton->setActiveGraphic(*logYOnPixmap);
	logYButton->setInactiveGraphic(*logYOffPixmap);
	logYButton->setCheckable(true);
	logYButton->setModel(&controls->m_logYModel);
	config_layout->addWidget(logYButton, 2, 2, 2, 1, Qt::AlignRight);

	ComboBox *ampRangeCombo = new ComboBox(this, tr("Amplitude range"));
	ampRangeCombo->setMinimumSize(100, 22);
	ampRangeCombo->setMaximumSize(200, 22);
	ampRangeCombo->setModel(&controls->m_ampRangeModel);
	controls->m_ampRangeModel.addItem(tr("Extended"));
	controls->m_ampRangeModel.addItem(tr("Standard"));
	controls->m_ampRangeModel.addItem(tr("Loud focus"));
	controls->m_ampRangeModel.addItem(tr("Silent focus"));
	if (!controls->m_loaded) {controls->m_ampRangeModel.setValue(controls->m_ampRangeModel.findText(tr("Standard")));}
	config_layout->addWidget(ampRangeCombo, 2, 3, 2, 1);


	// FFT block size
	QLabel *blockSizeLabel = new QLabel("", this);
	QPixmap *blockSizeIcon = new QPixmap(PLUGIN_NAME::getIconPixmap("block_size"));
	blockSizeIcon->setDevicePixelRatio(window()->devicePixelRatio());
	blockSizeLabel->setPixmap(blockSizeIcon->scaled(iconSize, iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	config_layout->addWidget(blockSizeLabel, 0, 4, 2, 1, Qt::AlignRight);

	ComboBox *blockSizeCombo = new ComboBox(this, tr("FFT Block Size"));
	blockSizeCombo->setMinimumSize(100, 22);
	blockSizeCombo->setMaximumSize(200, 22);
	blockSizeCombo->setModel(&controls->m_blockSizeModel);
	for (int i = 0; i < FFT_BLOCK_SIZES.size(); i++){
		if (i == 0){
			controls->m_blockSizeModel.addItem((std::to_string(FFT_BLOCK_SIZES[i]) + " ").c_str() + tr("(Fast, low-res.)"));
		} else if (i == FFT_BLOCK_SIZES.size() - 1){
			controls->m_blockSizeModel.addItem((std::to_string(FFT_BLOCK_SIZES[i]) + " ").c_str() + tr("(Slow, high-res.)"));
		} else {
			controls->m_blockSizeModel.addItem(std::to_string(FFT_BLOCK_SIZES[i]).c_str());
		}
	}
	if (!controls->m_loaded) {controls->m_blockSizeModel.setValue(controls->m_blockSizeModel.findText("2048"));}
	config_layout->addWidget(blockSizeCombo, 0, 5, 2, 1);
	processor->reallocateBuffers();
	connect(&controls->m_blockSizeModel, &ComboBoxModel::dataChanged, [=] {processor->reallocateBuffers();});

	// FFT window -- keep the same order as in the fft_helpers.h WINDOWS enum!
	QLabel *windowLabel = new QLabel("", this);
	QPixmap *windowIcon = new QPixmap(PLUGIN_NAME::getIconPixmap("window"));
	windowIcon->setDevicePixelRatio(window()->devicePixelRatio());
	windowLabel->setPixmap(windowIcon->scaled(iconSize, iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	config_layout->addWidget(windowLabel, 2, 4, 2, 1, Qt::AlignRight);

	ComboBox *windowCombo = new ComboBox(this, tr("FFT Window"));
	windowCombo->setMinimumSize(100, 22);
	windowCombo->setMaximumSize(200, 22);
	windowCombo->setModel(&controls->m_windowModel);
	controls->m_windowModel.addItem(tr("Rectangular (Off)"));
	controls->m_windowModel.addItem(tr("Blackman-Harris (Default)"));
	controls->m_windowModel.addItem(tr("Hamming"));
	controls->m_windowModel.addItem(tr("Hanning"));
	if (!controls->m_loaded) {controls->m_windowModel.setValue(controls->m_windowModel.findText(tr("Blackman-Harris (Default)")));}
	config_layout->addWidget(windowCombo, 2, 5, 2, 1);
	processor->rebuildWindow();
	connect(&controls->m_windowModel, &ComboBoxModel::dataChanged, [=] {processor->rebuildWindow();});


	// create spectrum displays
	SaSpectrumView *spectrum = new SaSpectrumView(controls, processor, this);
	SaWaterfallView *waterfall = new SaWaterfallView(controls, processor, this);

	// add everything to top-level splitter
	display_splitter->addWidget(config_widget);
	display_splitter->addWidget(spectrum);
	display_splitter->addWidget(waterfall);

//	windowLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
//	blockSizeLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	window()->setBaseSize(500,500);
	window()->resize(500,500);
}

