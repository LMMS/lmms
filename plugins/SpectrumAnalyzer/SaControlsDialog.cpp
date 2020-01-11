/*
 * SaControlsDialog.cpp - definition of SaControlsDialog class.
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

#include "SaControlsDialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QSplitter>
#include <QWidget>

#include "ComboBox.h"
#include "ComboBoxModel.h"
#include "embed.h"
#include "Engine.h"
#include "Knob.h"
#include "LedCheckbox.h"
#include "PixmapButton.h"
#include "SaControls.h"
#include "SaProcessor.h"


// The entire GUI layout is built here.
SaControlsDialog::SaControlsDialog(SaControls *controls, SaProcessor *processor) :
	EffectControlDialog(controls),
	m_controls(controls),
	m_processor(processor)
{
	// Top level placement of sections is handled by QSplitter widget.
	QHBoxLayout *master_layout = new QHBoxLayout;
	QSplitter *display_splitter = new QSplitter(Qt::Vertical);
	master_layout->addWidget(display_splitter);
	master_layout->setContentsMargins(2, 6, 2, 8);
	setLayout(master_layout);

	// Display splitter top: controls section
	QWidget *controls_widget = new QWidget;
	QHBoxLayout *controls_layout = new QHBoxLayout;
	controls_layout->setContentsMargins(0, 0, 0, 0);
	controls_widget->setLayout(controls_layout);
	controls_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
	controls_widget->setMaximumHeight(m_configHeight);
	display_splitter->addWidget(controls_widget);


	// Basic configuration
	QWidget *config_widget = new QWidget;
	QGridLayout *config_layout = new QGridLayout;
	config_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	config_widget->setMaximumHeight(m_configHeight);
	config_widget->setLayout(config_layout);
	controls_layout->addWidget(config_widget);
	controls_layout->setStretchFactor(config_widget, 10);

	// Pre-compute target pixmap size based on monitor DPI.
	// Using setDevicePixelRatio() on pixmap allows the SVG image to be razor
	// sharp on High-DPI screens, but the desired size must be manually
	// enlarged. No idea how to make Qt do it in a more reasonable way.
	QSize iconSize = QSize(22.0 * devicePixelRatio(), 22.0 * devicePixelRatio());
	QSize buttonSize = 1.2 * iconSize;
	QSize advButtonSize = QSize((m_configHeight * devicePixelRatio()) / 3, m_configHeight * devicePixelRatio());


	// pause and freeze buttons
	PixmapButton *pauseButton = new PixmapButton(this, tr("Pause"));
	pauseButton->setToolTip(tr("Pause data acquisition"));
	QPixmap *pauseOnPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("play").scaled(buttonSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	QPixmap *pauseOffPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("pause").scaled(buttonSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	pauseOnPixmap->setDevicePixelRatio(devicePixelRatio());
	pauseOffPixmap->setDevicePixelRatio(devicePixelRatio());
	pauseButton->setActiveGraphic(*pauseOnPixmap);
	pauseButton->setInactiveGraphic(*pauseOffPixmap);
	pauseButton->setCheckable(true);
	pauseButton->setModel(&controls->m_pauseModel);
	config_layout->addWidget(pauseButton, 0, 0, 2, 1, Qt::AlignHCenter);

	PixmapButton *refFreezeButton = new PixmapButton(this, tr("Reference freeze"));
	refFreezeButton->setToolTip(tr("Freeze current input as a reference / disable falloff in peak-hold mode."));
	QPixmap *freezeOnPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("freeze").scaled(buttonSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	QPixmap *freezeOffPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("freeze_off").scaled(buttonSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	freezeOnPixmap->setDevicePixelRatio(devicePixelRatio());
	freezeOffPixmap->setDevicePixelRatio(devicePixelRatio());
	refFreezeButton->setActiveGraphic(*freezeOnPixmap);
	refFreezeButton->setInactiveGraphic(*freezeOffPixmap);
	refFreezeButton->setCheckable(true);
	refFreezeButton->setModel(&controls->m_refFreezeModel);
	config_layout->addWidget(refFreezeButton, 2, 0, 2, 1, Qt::AlignHCenter);

	// misc configuration switches
	LedCheckBox *waterfallButton = new LedCheckBox(tr("Waterfall"), this);
	waterfallButton->setToolTip(tr("Display real-time spectrogram"));
	waterfallButton->setCheckable(true);
	waterfallButton->setMinimumSize(70, 12);
	waterfallButton->setModel(&controls->m_waterfallModel);
	config_layout->addWidget(waterfallButton, 0, 1);

	LedCheckBox *smoothButton = new LedCheckBox(tr("Averaging"), this);
	smoothButton->setToolTip(tr("Enable exponential moving average"));
	smoothButton->setCheckable(true);
	smoothButton->setMinimumSize(70, 12);
	smoothButton->setModel(&controls->m_smoothModel);
	config_layout->addWidget(smoothButton, 1, 1);

	LedCheckBox *stereoButton = new LedCheckBox(tr("Stereo"), this);
	stereoButton->setToolTip(tr("Display stereo channels separately"));
	stereoButton->setCheckable(true);
	stereoButton->setMinimumSize(70, 12);
	stereoButton->setModel(&controls->m_stereoModel);
	config_layout->addWidget(stereoButton, 2, 1);

	LedCheckBox *peakHoldButton = new LedCheckBox(tr("Peak hold"), this);
	peakHoldButton->setToolTip(tr("Display envelope of peak values"));
	peakHoldButton->setCheckable(true);
	peakHoldButton->setMinimumSize(70, 12);
	peakHoldButton->setModel(&controls->m_peakHoldModel);
	config_layout->addWidget(peakHoldButton, 3, 1);

	// frequency: linear / log. switch and range selector
	PixmapButton *logXButton = new PixmapButton(this, tr("Logarithmic frequency"));
	logXButton->setToolTip(tr("Switch between logarithmic and linear frequency scale"));
	QPixmap *logXOnPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("x_log").scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	QPixmap *logXOffPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("x_linear").scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	logXOnPixmap->setDevicePixelRatio(devicePixelRatio());
	logXOffPixmap->setDevicePixelRatio(devicePixelRatio());
	logXButton->setActiveGraphic(*logXOnPixmap);
	logXButton->setInactiveGraphic(*logXOffPixmap);
	logXButton->setCheckable(true);
	logXButton->setModel(&controls->m_logXModel);
	config_layout->addWidget(logXButton, 0, 2, 2, 1, Qt::AlignRight);

	ComboBox *freqRangeCombo = new ComboBox(this, tr("Frequency range"));
	freqRangeCombo->setToolTip(tr("Frequency range"));
	freqRangeCombo->setMinimumSize(100, 22);
	freqRangeCombo->setMaximumSize(200, 22);
	freqRangeCombo->setModel(&controls->m_freqRangeModel);
	config_layout->addWidget(freqRangeCombo, 0, 3, 2, 1);

	// amplitude: linear / log switch and range selector
	PixmapButton *logYButton = new PixmapButton(this, tr("Logarithmic amplitude"));
	logYButton->setToolTip(tr("Switch between logarithmic and linear amplitude scale"));
	QPixmap *logYOnPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("y_log").scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	QPixmap *logYOffPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("y_linear").scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	logYOnPixmap->setDevicePixelRatio(devicePixelRatio());
	logYOffPixmap->setDevicePixelRatio(devicePixelRatio());
	logYButton->setActiveGraphic(*logYOnPixmap);
	logYButton->setInactiveGraphic(*logYOffPixmap);
	logYButton->setCheckable(true);
	logYButton->setModel(&controls->m_logYModel);
	config_layout->addWidget(logYButton, 2, 2, 2, 1, Qt::AlignRight);

	ComboBox *ampRangeCombo = new ComboBox(this, tr("Amplitude range"));
	ampRangeCombo->setToolTip(tr("Amplitude range"));
	ampRangeCombo->setMinimumSize(100, 22);
	ampRangeCombo->setMaximumSize(200, 22);
	ampRangeCombo->setModel(&controls->m_ampRangeModel);
	config_layout->addWidget(ampRangeCombo, 2, 3, 2, 1);

	// FFT: block size: icon and selector
	QLabel *blockSizeLabel = new QLabel("", this);
	QPixmap *blockSizeIcon = new QPixmap(PLUGIN_NAME::getIconPixmap("block_size"));
	blockSizeIcon->setDevicePixelRatio(devicePixelRatio());
	blockSizeLabel->setPixmap(blockSizeIcon->scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	config_layout->addWidget(blockSizeLabel, 0, 4, 2, 1, Qt::AlignRight);

	ComboBox *blockSizeCombo = new ComboBox(this, tr("FFT block size"));
	blockSizeCombo->setToolTip(tr("FFT block size"));
	blockSizeCombo->setMinimumSize(100, 22);
	blockSizeCombo->setMaximumSize(200, 22);
	blockSizeCombo->setModel(&controls->m_blockSizeModel);
	config_layout->addWidget(blockSizeCombo, 0, 5, 2, 1);
	processor->reallocateBuffers();
	connect(&controls->m_blockSizeModel, &ComboBoxModel::dataChanged, [=] {processor->reallocateBuffers();});

	// FFT: window type: icon and selector
	QLabel *windowLabel = new QLabel("", this);
	QPixmap *windowIcon = new QPixmap(PLUGIN_NAME::getIconPixmap("window"));
	windowIcon->setDevicePixelRatio(devicePixelRatio());
	windowLabel->setPixmap(windowIcon->scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	config_layout->addWidget(windowLabel, 2, 4, 2, 1, Qt::AlignRight);

	ComboBox *windowCombo = new ComboBox(this, tr("FFT window type"));
	windowCombo->setToolTip(tr("FFT window type"));
	windowCombo->setMinimumSize(100, 22);
	windowCombo->setMaximumSize(200, 22);
	windowCombo->setModel(&controls->m_windowModel);
	config_layout->addWidget(windowCombo, 2, 5, 2, 1);
	processor->rebuildWindow();
	connect(&controls->m_windowModel, &ComboBoxModel::dataChanged, [=] {processor->rebuildWindow();});

	// set stretch factors so that combo boxes expand first
	config_layout->setColumnStretch(3, 2);
	config_layout->setColumnStretch(5, 3);


	// Advanced configuration
	QWidget *advanced_widget = new QWidget;
	QGridLayout *advanced_layout = new QGridLayout;
	advanced_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	advanced_widget->setMaximumHeight(m_configHeight);
	advanced_widget->setLayout(advanced_layout);
	advanced_widget->hide();
	controls_layout->addWidget(advanced_widget);
	controls_layout->setStretchFactor(advanced_widget, 10);

	// Peak envelope resolution
	Knob *envelopeResolutionKnob = new Knob(knobSmall_17, this);
	envelopeResolutionKnob->setModel(&controls->m_envelopeResolutionModel);
	envelopeResolutionKnob->setLabel(tr("Envelope res."));
	envelopeResolutionKnob->setToolTip(tr("Increase envelope resolution for better details, decrease for better GUI performance."));
	envelopeResolutionKnob->setHintText(tr("Draw at most"), tr(" envelope points per pixel"));
	advanced_layout->addWidget(envelopeResolutionKnob, 0, 0, 1, 1, Qt::AlignCenter);

	// Spectrum graph resolution
	Knob *spectrumResolutionKnob = new Knob(knobSmall_17, this);
	spectrumResolutionKnob->setModel(&controls->m_spectrumResolutionModel);
	spectrumResolutionKnob->setLabel(tr("Spectrum res."));
	spectrumResolutionKnob->setToolTip(tr("Increase spectrum resolution for better details, decrease for better GUI performance."));
	spectrumResolutionKnob->setHintText(tr("Draw at most"), tr(" spectrum points per pixel"));
	advanced_layout->addWidget(spectrumResolutionKnob, 1, 0, 1, 1, Qt::AlignCenter);

	// Peak falloff speed
	Knob *peakDecayFactorKnob = new Knob(knobSmall_17, this);
	peakDecayFactorKnob->setModel(&controls->m_peakDecayFactorModel);
	peakDecayFactorKnob->setLabel(tr("Falloff factor"));
	peakDecayFactorKnob->setToolTip(tr("Decrease to make peaks fall faster."));
	peakDecayFactorKnob->setHintText(tr("Multiply buffered value by"), "");
	advanced_layout->addWidget(peakDecayFactorKnob, 0, 1, 1, 1, Qt::AlignCenter);

	// Averaging weight
	Knob *averagingWeightKnob = new Knob(knobSmall_17, this);
	averagingWeightKnob->setModel(&controls->m_averagingWeightModel);
	averagingWeightKnob->setLabel(tr("Averaging weight"));
	averagingWeightKnob->setToolTip(tr("Decrease to make averaging slower and smoother."));
	averagingWeightKnob->setHintText(tr("New sample contributes"), "");
	advanced_layout->addWidget(averagingWeightKnob, 1, 1, 1, 1, Qt::AlignCenter);

	// Waterfall history size
	Knob *waterfallHeightKnob = new Knob(knobSmall_17, this);
	waterfallHeightKnob->setModel(&controls->m_waterfallHeightModel);
	waterfallHeightKnob->setLabel(tr("Waterfall height"));
	waterfallHeightKnob->setToolTip(tr("Increase to get slower scrolling, decrease to see fast transitions better. Warning: medium CPU usage."));
	waterfallHeightKnob->setHintText(tr("Keep"), tr(" lines"));
	advanced_layout->addWidget(waterfallHeightKnob, 0, 2, 1, 1, Qt::AlignCenter);
	processor->reallocateBuffers();
	connect(&controls->m_waterfallHeightModel, &FloatModel::dataChanged, [=] {processor->reallocateBuffers();});

	// Waterfall gamma correction
	Knob *waterfallGammaKnob = new Knob(knobSmall_17, this);
	waterfallGammaKnob->setModel(&controls->m_waterfallGammaModel);
	waterfallGammaKnob->setLabel(tr("Waterfall gamma"));
	waterfallGammaKnob->setToolTip(tr("Decrease to see very weak signals, increase to get better contrast."));
	waterfallGammaKnob->setHintText(tr("Gamma value:"), "");
	advanced_layout->addWidget(waterfallGammaKnob, 1, 2, 1, 1, Qt::AlignCenter);

	// FFT window overlap
	Knob *windowOverlapKnob = new Knob(knobSmall_17, this);
	windowOverlapKnob->setModel(&controls->m_windowOverlapModel);
	windowOverlapKnob->setLabel(tr("Window overlap"));
	windowOverlapKnob->setToolTip(tr("Increase to prevent missing fast transitions arriving near FFT window edges. Warning: high CPU usage."));
	windowOverlapKnob->setHintText(tr("Each sample processed"), tr(" times"));
	advanced_layout->addWidget(windowOverlapKnob, 0, 3, 1, 1, Qt::AlignCenter);

	// FFT zero padding
	Knob *zeroPaddingKnob = new Knob(knobSmall_17, this);
	zeroPaddingKnob->setModel(&controls->m_zeroPaddingModel);
	zeroPaddingKnob->setLabel(tr("Zero padding"));
	zeroPaddingKnob->setToolTip(tr("Increase to get smoother-looking spectrum. Warning: high CPU usage."));
	zeroPaddingKnob->setHintText(tr("Processing buffer is"), tr(" steps larger than input block"));
	advanced_layout->addWidget(zeroPaddingKnob, 1, 3, 1, 1, Qt::AlignCenter);
	processor->reallocateBuffers();
	connect(&controls->m_zeroPaddingModel, &FloatModel::dataChanged, [=] {processor->reallocateBuffers();});


	// Advanced settings button
	PixmapButton *advancedButton = new PixmapButton(this, tr("Advanced settings"));
	advancedButton->setToolTip(tr("Access advanced settings"));
	QPixmap *advancedOnPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("advanced_on").scaled(advButtonSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	QPixmap *advancedOffPixmap = new QPixmap(PLUGIN_NAME::getIconPixmap("advanced_off").scaled(advButtonSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	advancedOnPixmap->setDevicePixelRatio(devicePixelRatio());
	advancedOffPixmap->setDevicePixelRatio(devicePixelRatio());
	advancedButton->setActiveGraphic(*advancedOnPixmap);
	advancedButton->setInactiveGraphic(*advancedOffPixmap);
	advancedButton->setCheckable(true);
	controls_layout->addStretch(0);
	controls_layout->addWidget(advancedButton);

	connect(advancedButton, &PixmapButton::toggled, [=](bool checked)
		{
			if (checked)
			{
				config_widget->hide();
				advanced_widget->show();
			}
			else
			{
				config_widget->show();
				advanced_widget->hide();
			}
		}
	);

	// QSplitter middle and bottom: spectrum display widgets
	m_spectrum = new SaSpectrumView(controls, processor, this);
	display_splitter->addWidget(m_spectrum);

	m_waterfall = new SaWaterfallView(controls, processor, this);
	display_splitter->addWidget(m_waterfall);
	m_waterfall->setVisible(m_controls->m_waterfallModel.value());
	connect(&controls->m_waterfallModel, &BoolModel::dataChanged, [=] {m_waterfall->updateVisibility();});
}


// Suggest the best current widget size.
QSize SaControlsDialog::sizeHint() const
{
	// Best width is determined by spectrum display sizeHint.
	// Best height depends on whether waterfall is visible and
	// consists of heights of the config section, spectrum, waterfall
	// and some reserve for margins.
	if (m_waterfall->isVisible())
	{
		return QSize(m_spectrum->sizeHint().width(),
					 m_configHeight + m_spectrum->sizeHint().height() + m_waterfall->sizeHint().height() + 50);
	}
	else
	{
		return QSize(m_spectrum->sizeHint().width(),
					 m_configHeight + m_spectrum->sizeHint().height() + 50);
	}
}

