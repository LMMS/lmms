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
#include <QPainterPath>
#include <QSizePolicy>
#include <QSplitter>
#include <QWidget>

#include "ComboBox.h"
#include "ComboBoxModel.h"
#include "FontHelper.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "PixmapButton.h"
#include "SaControls.h"
#include "SaProcessor.h"
#include "SaSpectrumView.h"
#include "SaWaterfallView.h"


namespace lmms::gui
{


// The entire GUI layout is built here.
SaControlsDialog::SaControlsDialog(SaControls *controls, SaProcessor *processor) :
	EffectControlDialog(controls),
	m_controls(controls),
	m_processor(processor)
{
	// Top level placement of sections is handled by QSplitter widget.
	auto master_layout = new QHBoxLayout;
	auto display_splitter = new QSplitter(Qt::Vertical);
	master_layout->addWidget(display_splitter);
	master_layout->setContentsMargins(2, 6, 2, 8);
	setLayout(master_layout);

	// Display splitter top: controls section
	auto controls_widget = new QWidget;
	auto controls_layout = new QHBoxLayout;
	controls_layout->setContentsMargins(0, 0, 0, 0);
	controls_widget->setLayout(controls_layout);
	controls_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
	controls_widget->setMaximumHeight(m_configHeight);
	display_splitter->addWidget(controls_widget);


	// Basic configuration
	auto config_widget = new QWidget;
	auto config_layout = new QGridLayout;
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
	auto pauseButton = new PixmapButton(this, tr("Pause"));
	pauseButton->setToolTip(tr("Pause data acquisition"));
	static auto s_pauseOnPixmap
		= PLUGIN_NAME::getIconPixmap("play").scaled(buttonSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	static auto s_pauseOffPixmap
		= PLUGIN_NAME::getIconPixmap("pause").scaled(buttonSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	s_pauseOnPixmap.setDevicePixelRatio(devicePixelRatio());
	s_pauseOffPixmap.setDevicePixelRatio(devicePixelRatio());
	pauseButton->setActiveGraphic(s_pauseOnPixmap);
	pauseButton->setInactiveGraphic(s_pauseOffPixmap);
	pauseButton->setCheckable(true);
	pauseButton->setModel(&controls->m_pauseModel);
	config_layout->addWidget(pauseButton, 0, 0, 2, 1, Qt::AlignHCenter);

	auto refFreezeButton = new PixmapButton(this, tr("Reference freeze"));
	refFreezeButton->setToolTip(tr("Freeze current input as a reference / disable falloff in peak-hold mode."));
	static auto s_freezeOnPixmap
		= PLUGIN_NAME::getIconPixmap("freeze").scaled(buttonSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	static auto s_freezeOffPixmap
		= PLUGIN_NAME::getIconPixmap("freeze_off").scaled(buttonSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	s_freezeOnPixmap.setDevicePixelRatio(devicePixelRatio());
	s_freezeOffPixmap.setDevicePixelRatio(devicePixelRatio());
	refFreezeButton->setActiveGraphic(s_freezeOnPixmap);
	refFreezeButton->setInactiveGraphic(s_freezeOffPixmap);
	refFreezeButton->setCheckable(true);
	refFreezeButton->setModel(&controls->m_refFreezeModel);
	config_layout->addWidget(refFreezeButton, 2, 0, 2, 1, Qt::AlignHCenter);

	// misc configuration switches
	auto waterfallButton = new LedCheckBox(tr("Waterfall"), this);
	waterfallButton->setToolTip(tr("Display real-time spectrogram"));
	waterfallButton->setCheckable(true);
	waterfallButton->setMinimumSize(100, 12);
	waterfallButton->setModel(&controls->m_waterfallModel);
	config_layout->addWidget(waterfallButton, 0, 1);

	auto smoothButton = new LedCheckBox(tr("Averaging"), this);
	smoothButton->setToolTip(tr("Enable exponential moving average"));
	smoothButton->setCheckable(true);
	smoothButton->setMinimumSize(100, 12);
	smoothButton->setModel(&controls->m_smoothModel);
	config_layout->addWidget(smoothButton, 1, 1);

	auto stereoButton = new LedCheckBox(tr("Stereo"), this);
	stereoButton->setToolTip(tr("Display stereo channels separately"));
	stereoButton->setCheckable(true);
	stereoButton->setMinimumSize(100, 12);
	stereoButton->setModel(&controls->m_stereoModel);
	config_layout->addWidget(stereoButton, 2, 1);

	auto peakHoldButton = new LedCheckBox(tr("Peak hold"), this);
	peakHoldButton->setToolTip(tr("Display envelope of peak values"));
	peakHoldButton->setCheckable(true);
	peakHoldButton->setMinimumSize(100, 12);
	peakHoldButton->setModel(&controls->m_peakHoldModel);
	config_layout->addWidget(peakHoldButton, 3, 1);

	// frequency: linear / log. switch and range selector
	auto logXButton = new PixmapButton(this, tr("Logarithmic frequency"));
	logXButton->setToolTip(tr("Switch between logarithmic and linear frequency scale"));
	static auto s_logXOnPixmap
		= PLUGIN_NAME::getIconPixmap("x_log").scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	static auto s_logXOffPixmap
		= PLUGIN_NAME::getIconPixmap("x_linear").scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	s_logXOnPixmap.setDevicePixelRatio(devicePixelRatio());
	s_logXOffPixmap.setDevicePixelRatio(devicePixelRatio());
	logXButton->setActiveGraphic(s_logXOnPixmap);
	logXButton->setInactiveGraphic(s_logXOffPixmap);
	logXButton->setCheckable(true);
	logXButton->setModel(&controls->m_logXModel);
	config_layout->addWidget(logXButton, 0, 2, 2, 1, Qt::AlignRight);

	auto freqRangeCombo = new ComboBox(this, tr("Frequency range"));
	freqRangeCombo->setToolTip(tr("Frequency range"));
	freqRangeCombo->setMinimumSize(100, ComboBox::DEFAULT_HEIGHT);
	freqRangeCombo->setMaximumSize(200, ComboBox::DEFAULT_HEIGHT);
	freqRangeCombo->setModel(&controls->m_freqRangeModel);
	config_layout->addWidget(freqRangeCombo, 0, 3, 2, 1);

	// amplitude: linear / log switch and range selector
	auto logYButton = new PixmapButton(this, tr("Logarithmic amplitude"));
	logYButton->setToolTip(tr("Switch between logarithmic and linear amplitude scale"));
	static auto s_logYOnPixmap
		= PLUGIN_NAME::getIconPixmap("y_log").scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	static auto s_logYOffPixmap
		= PLUGIN_NAME::getIconPixmap("y_linear").scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	s_logYOnPixmap.setDevicePixelRatio(devicePixelRatio());
	s_logYOffPixmap.setDevicePixelRatio(devicePixelRatio());
	logYButton->setActiveGraphic(s_logYOnPixmap);
	logYButton->setInactiveGraphic(s_logYOffPixmap);
	logYButton->setCheckable(true);
	logYButton->setModel(&controls->m_logYModel);
	config_layout->addWidget(logYButton, 2, 2, 2, 1, Qt::AlignRight);

	auto ampRangeCombo = new ComboBox(this, tr("Amplitude range"));
	ampRangeCombo->setToolTip(tr("Amplitude range"));
	ampRangeCombo->setMinimumSize(100, ComboBox::DEFAULT_HEIGHT);
	ampRangeCombo->setMaximumSize(200, ComboBox::DEFAULT_HEIGHT);
	ampRangeCombo->setModel(&controls->m_ampRangeModel);
	config_layout->addWidget(ampRangeCombo, 2, 3, 2, 1);

	// FFT: block size: icon and selector
	auto blockSizeLabel = new QLabel("", this);
	static auto s_blockSizeIcon = PLUGIN_NAME::getIconPixmap("block_size");
	s_blockSizeIcon.setDevicePixelRatio(devicePixelRatio());
	blockSizeLabel->setPixmap(s_blockSizeIcon.scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	config_layout->addWidget(blockSizeLabel, 0, 4, 2, 1, Qt::AlignRight);

	auto blockSizeCombo = new ComboBox(this, tr("FFT block size"));
	blockSizeCombo->setToolTip(tr("FFT block size"));
	blockSizeCombo->setMinimumSize(100, 22);
	blockSizeCombo->setMaximumSize(200, 22);
	blockSizeCombo->setModel(&controls->m_blockSizeModel);
	config_layout->addWidget(blockSizeCombo, 0, 5, 2, 1);
	processor->reallocateBuffers();
	connect(&controls->m_blockSizeModel, &ComboBoxModel::dataChanged, [=] {processor->reallocateBuffers();});

	// FFT: window type: icon and selector
	auto windowLabel = new QLabel("", this);
	static auto s_windowIcon = PLUGIN_NAME::getIconPixmap("window");
	s_windowIcon.setDevicePixelRatio(devicePixelRatio());
	windowLabel->setPixmap(s_windowIcon.scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	config_layout->addWidget(windowLabel, 2, 4, 2, 1, Qt::AlignRight);

	auto windowCombo = new ComboBox(this, tr("FFT window type"));
	windowCombo->setToolTip(tr("FFT window type"));
	windowCombo->setMinimumSize(100, ComboBox::DEFAULT_HEIGHT);
	windowCombo->setMaximumSize(200, ComboBox::DEFAULT_HEIGHT);
	windowCombo->setModel(&controls->m_windowModel);
	config_layout->addWidget(windowCombo, 2, 5, 2, 1);
	processor->rebuildWindow();
	connect(&controls->m_windowModel, &ComboBoxModel::dataChanged, [=] {processor->rebuildWindow();});

	// set stretch factors so that combo boxes expand first
	config_layout->setColumnStretch(3, 2);
	config_layout->setColumnStretch(5, 3);


	// Advanced configuration
	auto advanced_widget = new QWidget;
	auto advanced_layout = new QGridLayout;
	advanced_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	advanced_widget->setMaximumHeight(m_configHeight);
	advanced_widget->setLayout(advanced_layout);
	advanced_widget->hide();
	controls_layout->addWidget(advanced_widget);
	controls_layout->setStretchFactor(advanced_widget, 10);

	// Peak envelope resolution
	auto envelopeResolutionKnob = new Knob(KnobType::Small17, tr("Envelope res."), SMALL_FONT_SIZE, this);
	envelopeResolutionKnob->setModel(&controls->m_envelopeResolutionModel);
	envelopeResolutionKnob->setToolTip(tr("Increase envelope resolution for better details, decrease for better GUI performance."));
	envelopeResolutionKnob->setHintText(tr("Maximum number of envelope points drawn per pixel:"), "");
	advanced_layout->addWidget(envelopeResolutionKnob, 0, 0, 1, 1, Qt::AlignCenter);

	// Spectrum graph resolution
	auto spectrumResolutionKnob = new Knob(KnobType::Small17, tr("Spectrum res."), SMALL_FONT_SIZE, this);
	spectrumResolutionKnob->setModel(&controls->m_spectrumResolutionModel);
	spectrumResolutionKnob->setToolTip(tr("Increase spectrum resolution for better details, decrease for better GUI performance."));
	spectrumResolutionKnob->setHintText(tr("Maximum number of spectrum points drawn per pixel:"), "");
	advanced_layout->addWidget(spectrumResolutionKnob, 1, 0, 1, 1, Qt::AlignCenter);

	// Peak falloff speed
	auto peakDecayFactorKnob = new Knob(KnobType::Small17, tr("Falloff factor"), SMALL_FONT_SIZE, this);
	peakDecayFactorKnob->setModel(&controls->m_peakDecayFactorModel);
	peakDecayFactorKnob->setToolTip(tr("Decrease to make peaks fall faster."));
	peakDecayFactorKnob->setHintText(tr("Multiply buffered value by"), "");
	advanced_layout->addWidget(peakDecayFactorKnob, 0, 1, 1, 1, Qt::AlignCenter);

	// Averaging weight
	auto averagingWeightKnob = new Knob(KnobType::Small17, tr("Averaging weight"), SMALL_FONT_SIZE, this);
	averagingWeightKnob->setModel(&controls->m_averagingWeightModel);
	averagingWeightKnob->setToolTip(tr("Decrease to make averaging slower and smoother."));
	averagingWeightKnob->setHintText(tr("New sample contributes"), "");
	advanced_layout->addWidget(averagingWeightKnob, 1, 1, 1, 1, Qt::AlignCenter);

	// Waterfall history size
	auto waterfallHeightKnob = new Knob(KnobType::Small17, tr("Waterfall height"), SMALL_FONT_SIZE, this);
	waterfallHeightKnob->setModel(&controls->m_waterfallHeightModel);
	waterfallHeightKnob->setToolTip(tr("Increase to get slower scrolling, decrease to see fast transitions better. Warning: medium CPU usage."));
	waterfallHeightKnob->setHintText(tr("Number of lines to keep:"), "");
	advanced_layout->addWidget(waterfallHeightKnob, 0, 2, 1, 1, Qt::AlignCenter);
	processor->reallocateBuffers();
	connect(&controls->m_waterfallHeightModel, &FloatModel::dataChanged, [=] {processor->reallocateBuffers();});

	// Waterfall gamma correction
	auto waterfallGammaKnob = new Knob(KnobType::Small17, tr("Waterfall gamma"), SMALL_FONT_SIZE, this);
	waterfallGammaKnob->setModel(&controls->m_waterfallGammaModel);
	waterfallGammaKnob->setToolTip(tr("Decrease to see very weak signals, increase to get better contrast."));
	waterfallGammaKnob->setHintText(tr("Gamma value:"), "");
	advanced_layout->addWidget(waterfallGammaKnob, 1, 2, 1, 1, Qt::AlignCenter);

	// FFT window overlap
	auto windowOverlapKnob = new Knob(KnobType::Small17, tr("Window overlap"), SMALL_FONT_SIZE, this);
	windowOverlapKnob->setModel(&controls->m_windowOverlapModel);
	windowOverlapKnob->setToolTip(tr("Increase to prevent missing fast transitions arriving near FFT window edges. Warning: high CPU usage."));
	windowOverlapKnob->setHintText(tr("Number of times each sample is processed:"), "");
	advanced_layout->addWidget(windowOverlapKnob, 0, 3, 1, 1, Qt::AlignCenter);

	// FFT zero padding
	auto zeroPaddingKnob = new Knob(KnobType::Small17, tr("Zero padding"), SMALL_FONT_SIZE, this);
	zeroPaddingKnob->setModel(&controls->m_zeroPaddingModel);
	zeroPaddingKnob->setToolTip(tr("Increase to get smoother-looking spectrum. Warning: high CPU usage."));
	zeroPaddingKnob->setHintText(tr("Processing buffer is"), tr(" steps larger than input block"));
	advanced_layout->addWidget(zeroPaddingKnob, 1, 3, 1, 1, Qt::AlignCenter);
	processor->reallocateBuffers();
	connect(&controls->m_zeroPaddingModel, &FloatModel::dataChanged, [=] {processor->reallocateBuffers();});


	// Advanced settings button
	auto advancedButton = new PixmapButton(this, tr("Advanced settings"));
	advancedButton->setToolTip(tr("Access advanced settings"));
	static auto s_advancedOnPixmap = PLUGIN_NAME::getIconPixmap("advanced_on")
											.scaled(advButtonSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	static auto s_advancedOffPixmap = PLUGIN_NAME::getIconPixmap("advanced_off")
											 .scaled(advButtonSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	s_advancedOnPixmap.setDevicePixelRatio(devicePixelRatio());
	s_advancedOffPixmap.setDevicePixelRatio(devicePixelRatio());
	advancedButton->setActiveGraphic(s_advancedOnPixmap);
	advancedButton->setInactiveGraphic(s_advancedOffPixmap);
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


} // namespace lmms::gui
