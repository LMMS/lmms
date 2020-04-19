/*
 * CompressorControlDialog.cpp
 *
 * Copyright (c) 2020 Lost Robot <r94231@gmail.com>
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

#include <QLayout>

#include "Compressor.h"
#include "CompressorControlDialog.h"
#include "CompressorControls.h"
#include "embed.h"
#include <QLabel>
#include "GuiApplication.h"
#include "MainWindow.h"
#include "interpolation.h"
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include "ToolTip.h"
#include "gui_templates.h"

CompressorControlDialog::CompressorControlDialog(CompressorControls* controls) :
	EffectControlDialog(controls),
	m_controls(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setMinimumSize(MIN_COMP_SCREEN_X, MIN_COMP_SCREEN_Y);
	resize(COMP_SCREEN_X, COMP_SCREEN_Y);

	m_graphPixmap.fill(QColor("transparent"));

	m_visPixmap.fill(QColor("transparent"));

	m_kneePixmap.fill(QColor("transparent"));

	m_kneePixmap2.fill(QColor("transparent"));

	m_miscPixmap.fill(QColor("transparent"));

	m_controlsBoxLabel = new QLabel(this);
	m_controlsBoxLabel->setPixmap(PLUGIN_NAME::getIconPixmap("controlsBox"));
	m_controlsBoxLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

	m_rmsEnabledLabel = new QLabel(this);
	m_rmsEnabledLabel->setPixmap(PLUGIN_NAME::getIconPixmap("knob_enabled"));
	m_rmsEnabledLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

	m_blendEnabledLabel = new QLabel(this);
	m_blendEnabledLabel->setPixmap(PLUGIN_NAME::getIconPixmap("knob_enabled"));
	m_blendEnabledLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

	m_lookaheadEnabledLabel = new QLabel(this);
	m_lookaheadEnabledLabel->setPixmap(PLUGIN_NAME::getIconPixmap("knob_enabled"));
	m_lookaheadEnabledLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

	m_ratioEnabledLabel = new QLabel(this);
	m_ratioEnabledLabel->setPixmap(PLUGIN_NAME::getIconPixmap("knob_enabled_large"));
	m_ratioEnabledLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

	m_thresholdKnob = new Knob(knobStyled, this);
	makeLargeKnob(m_thresholdKnob, tr("Threshold:") , " dbFs");
	m_thresholdKnob->setModel(&controls->m_thresholdModel);
	ToolTip::add(m_thresholdKnob, tr("Volume at which the compression begins to take place"));

	m_ratioKnob = new Knob(knobStyled, this);
	makeLargeKnob(m_ratioKnob, tr("Ratio:") , ":1");
	m_ratioKnob->setModel(&controls->m_ratioModel);
	ToolTip::add(m_ratioKnob, tr("How far the compressor must turn the volume down after crossing the threshold"));

	m_attackKnob = new Knob(knobStyled, this);
	makeLargeKnob(m_attackKnob, tr("Attack:") , " ms");
	m_attackKnob->setModel(&controls->m_attackModel);
	ToolTip::add(m_attackKnob, tr("Speed at which the compressor starts to compress the audio"));

	m_releaseKnob = new Knob(knobStyled, this);
	makeLargeKnob(m_releaseKnob, tr("Release:") , " ms");
	m_releaseKnob->setModel(&controls->m_releaseModel);
	ToolTip::add(m_releaseKnob, tr("Speed at which the compressor ceases to compress the audio"));

	m_kneeKnob = new Knob(knobStyled, this);
	makeSmallKnob(m_kneeKnob, tr("Knee:") , " dbFs");
	m_kneeKnob->setModel(&controls->m_kneeModel);
	ToolTip::add(m_kneeKnob, tr("Smooth out the gain reduction curve around the threshold"));

	m_rangeKnob = new Knob(knobStyled, this);
	makeSmallKnob(m_rangeKnob, tr("Range:") , " dbFs");
	m_rangeKnob->setModel(&controls->m_rangeModel);
	ToolTip::add(m_rangeKnob, tr("Maximum gain reduction"));

	m_lookaheadLengthKnob = new Knob(knobStyled, this);
	makeSmallKnob(m_lookaheadLengthKnob, tr("Lookahead Length:") , " ms");
	m_lookaheadLengthKnob->setModel(&controls->m_lookaheadLengthModel);
	ToolTip::add(m_lookaheadLengthKnob, tr("How long the compressor has to react to the sidechain signal ahead of time"));

	m_holdKnob = new Knob(knobStyled, this);
	makeSmallKnob(m_holdKnob, tr("Hold:") , " ms");
	m_holdKnob->setModel(&controls->m_holdModel);
	ToolTip::add(m_holdKnob, tr("Delay between attack and release stages"));

	m_rmsKnob = new Knob(knobStyled, this);
	makeSmallKnob(m_rmsKnob, tr("RMS Size:") , "");
	m_rmsKnob->setModel(&controls->m_rmsModel);
	ToolTip::add(m_rmsKnob, tr("Size of the RMS buffer"));

	m_inBalanceKnob = new Knob(knobStyled, this);
	makeSmallKnob(m_inBalanceKnob, tr("Input Balance:") , "");
	m_inBalanceKnob->setModel(&controls->m_inBalanceModel);
	ToolTip::add(m_inBalanceKnob, tr("Bias the input audio to the left/right or mid/side"));

	m_outBalanceKnob = new Knob(knobStyled, this);
	makeSmallKnob(m_outBalanceKnob, tr("Output Balance:") , "");
	m_outBalanceKnob->setModel(&controls->m_outBalanceModel);
	ToolTip::add(m_outBalanceKnob, tr("Bias the output audio to the left/right or mid/side"));

	m_stereoBalanceKnob = new Knob(knobStyled, this);
	makeSmallKnob(m_stereoBalanceKnob, tr("Stereo Balance:") , "");
	m_stereoBalanceKnob->setModel(&controls->m_stereoBalanceModel);
	ToolTip::add(m_stereoBalanceKnob, tr("Bias the sidechain signal to the left/right or mid/side"));

	m_blendKnob = new Knob(knobStyled, this);
	makeSmallKnob(m_blendKnob, tr("Stereo Link Blend:") , "");
	m_blendKnob->setModel(&controls->m_blendModel);
	ToolTip::add(m_blendKnob, tr("Blend between unlinked/maximum/average/minimum stereo linking modes"));

	m_tiltKnob = new Knob(knobStyled, this);
	makeSmallKnob(m_tiltKnob, tr("Tilt Gain:") , " db");
	m_tiltKnob->setModel(&controls->m_tiltModel);
	ToolTip::add(m_tiltKnob, tr("Bias the sidechain signal to the low or high frequencies.  -6 db is lowpass, 6 db is highpass."));

	m_tiltFreqKnob = new Knob(knobStyled, this);
	makeSmallKnob(m_tiltFreqKnob, tr("Tilt Frequency:") , " Hz");
	m_tiltFreqKnob->setModel(&controls->m_tiltFreqModel);
	ToolTip::add(m_tiltFreqKnob, tr("Center frequency of sidechain tilt filter"));

	m_mixKnob = new Knob(knobStyled, this);
	makeSmallKnob(m_mixKnob, tr("Mix:") , "%");
	m_mixKnob->setModel(&controls->m_mixModel);
	ToolTip::add(m_mixKnob, tr("Balance between wet and dry signals"));

	m_autoAttackKnob = new Knob(knobStyled, this);
	makeSmallKnob(m_autoAttackKnob, tr("Auto Attack:") , "%");
	m_autoAttackKnob->setModel(&controls->m_autoAttackModel);
	ToolTip::add(m_autoAttackKnob, tr("Automatically control attack value depending on crest factor"));

	m_autoReleaseKnob = new Knob(knobStyled, this);
	makeSmallKnob(m_autoReleaseKnob, tr("Auto Release:") , "%");
	m_autoReleaseKnob->setModel(&controls->m_autoReleaseModel);
	ToolTip::add(m_autoReleaseKnob, tr("Automatically control release value depending on crest factor"));




	m_outFader = new EqFader(&controls->m_outGainModel,tr("Output gain"),
		this, &controls->m_outPeakL, &controls->m_outPeakR);
	m_outFader->setDisplayConversion(false);
	m_outFader->setHintText(tr("Gain"), "dBFS");
	ToolTip::add(m_outFader, tr("Output volume"));

	m_inFader = new EqFader(&controls->m_inGainModel,tr("Input gain"),
		this, &controls->m_inPeakL, &controls->m_inPeakR);
	m_inFader->setDisplayConversion(false);
	m_inFader->setHintText(tr("Gain"), "dBFS");
	ToolTip::add(m_inFader, tr("Input volume"));

	rmsButton = new PixmapButton(this, tr("Root Mean Square"));
	rmsButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("rms_sel"));
	rmsButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("rms_unsel"));
	ToolTip::add(rmsButton, tr("Use RMS of the input"));

	peakButton = new PixmapButton(this, tr("Peak"));
	peakButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("peak_sel"));
	peakButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("peak_unsel"));
	ToolTip::add(peakButton, tr("Use absolute value of the input"));
	
	rmsPeakGroup = new automatableButtonGroup(this);
	rmsPeakGroup->addButton(rmsButton);
	rmsPeakGroup->addButton(peakButton);
	rmsPeakGroup->setModel(&controls->m_peakmodeModel);

	leftRightButton = new PixmapButton(this, tr("Left/Right"));
	leftRightButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("leftright_sel"));
	leftRightButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("leftright_unsel"));
	ToolTip::add(leftRightButton, tr("Compress left and right audio"));

	midSideButton = new PixmapButton(this, tr("Mid/Side"));
	midSideButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("midside_sel"));
	midSideButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("midside_unsel"));
	ToolTip::add(midSideButton, tr("Compress mid and side audio"));
	
	leftRightMidSideGroup = new automatableButtonGroup(this);
	leftRightMidSideGroup->addButton(leftRightButton);
	leftRightMidSideGroup->addButton(midSideButton);
	leftRightMidSideGroup->setModel(&controls->m_midsideModel);

	compressButton = new PixmapButton(this, tr("Compressor"));
	compressButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("compressor_sel"));
	compressButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("compressor_unsel"));
	ToolTip::add(compressButton, tr("Compress the audio"));

	limitButton = new PixmapButton(this, tr("Limiter"));
	limitButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("limiter_sel"));
	limitButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("limiter_unsel"));
	ToolTip::add(limitButton, tr("Set Ratio to infinity"));

	compressLimitGroup = new automatableButtonGroup(this);
	compressLimitGroup->addButton(compressButton);
	compressLimitGroup->addButton(limitButton);
	compressLimitGroup->setModel(&controls->m_limiterModel);

	unlinkedButton = new PixmapButton(this, tr("Unlinked"));
	unlinkedButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("unlinked_sel"));
	unlinkedButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("unlinked_unsel"));
	ToolTip::add(unlinkedButton, tr("Compress each channel separately"));

	maximumButton = new PixmapButton(this, tr("Maximum"));
	maximumButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("maximum_sel"));
	maximumButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("maximum_unsel"));
	ToolTip::add(maximumButton, tr("Compress based on the loudest channel"));

	averageButton = new PixmapButton(this, tr("Average"));
	averageButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("average_sel"));
	averageButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("average_unsel"));
	ToolTip::add(averageButton, tr("Compress based on the averaged channel volume"));

	minimumButton = new PixmapButton(this, tr("Minimum"));
	minimumButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("minimum_sel"));
	minimumButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("minimum_unsel"));
	ToolTip::add(minimumButton, tr("Compress based on the quietest channel"));

	blendButton = new PixmapButton(this, tr("Blend"));
	blendButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("blend_sel"));
	blendButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("blend_unsel"));
	ToolTip::add(blendButton, tr("Blend between stereo linking modes"));

	stereoLinkGroup = new automatableButtonGroup(this);
	stereoLinkGroup->addButton(unlinkedButton);
	stereoLinkGroup->addButton(maximumButton);
	stereoLinkGroup->addButton(averageButton);
	stereoLinkGroup->addButton(minimumButton);
	stereoLinkGroup->addButton(blendButton);
	stereoLinkGroup->setModel(&controls->m_stereoLinkModel);

	autoMakeupButton = new PixmapButton(this, tr("Auto Makeup Gain"));
	autoMakeupButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("autogain_sel"));
	autoMakeupButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("autogain_unsel"));
	ToolTip::add(autoMakeupButton, tr("Automatically change makeup gain depending on threshold, knee, and ratio settings"));
	autoMakeupButton->setCheckable(true);
	autoMakeupButton->setModel(&controls->m_autoMakeupModel);

	auditionButton = new PixmapButton(this, tr("Soft Clip"));
	auditionButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("audition_sel"));
	auditionButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("audition_unsel"));
	ToolTip::add(auditionButton, tr("Play the delta signal"));
	auditionButton->setCheckable(true);
	auditionButton->setModel(&controls->m_auditionModel);

	feedbackButton = new PixmapButton(this, tr("Soft Clip"));
	feedbackButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("feedback_sel"));
	feedbackButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("feedback_unsel"));
	ToolTip::add(feedbackButton, tr("Use the compressor's output as the sidechain input"));
	feedbackButton->setCheckable(true);
	feedbackButton->setModel(&controls->m_feedbackModel);

	lookaheadButton = new PixmapButton(this, tr("Lookahead Enabled"));
	lookaheadButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("lookahead_sel"));
	lookaheadButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("lookahead_unsel"));
	ToolTip::add(lookaheadButton, tr("Enable Lookahead, which introduces 20 milliseconds of latency"));
	lookaheadButton->setCheckable(true);
	lookaheadButton->setModel(&controls->m_lookaheadModel);

	connect(gui->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(updateDisplay()));

	connect(&m_controls->m_peakmodeModel, SIGNAL(dataChanged()), this, SLOT(peakmodeChanged()));
	connect(&m_controls->m_stereoLinkModel, SIGNAL(dataChanged()), this, SLOT(stereoLinkChanged()));
	connect(&m_controls->m_lookaheadModel, SIGNAL(dataChanged()), this, SLOT(lookaheadChanged()));
	connect(&m_controls->m_limiterModel, SIGNAL(dataChanged()), this, SLOT(limiterChanged()));

	m_timeElapsed.start();

	emit peakmodeChanged();
	emit stereoLinkChanged();
	emit lookaheadChanged();
	emit limiterChanged();
}


void CompressorControlDialog::makeLargeKnob(Knob * knob, QString hint, QString unit)
{
	knob->setHintText(hint, unit);
	knob->setFixedSize(56, 56);
	knob->setOuterRadius(23);
	knob->setInnerRadius(15);
	knob->setCenterPointX(28);
	knob->setCenterPointY(28);
}

void CompressorControlDialog::makeSmallKnob(Knob * knob, QString hint, QString unit)
{
	knob->setHintText(hint, unit);
	knob->setFixedSize(30, 30);
	knob->setOuterRadius(10);
	knob->setInnerRadius(4);
	knob->setCenterPointX(15);
	knob->setCenterPointY(15);
}


void CompressorControlDialog::peakmodeChanged()
{
	m_rmsKnob->setVisible(!m_controls->m_peakmodeModel.value());
	m_rmsEnabledLabel->setVisible(!m_controls->m_peakmodeModel.value());
}


void CompressorControlDialog::stereoLinkChanged()
{
	m_blendKnob->setVisible(m_controls->m_stereoLinkModel.value() == 4);
	m_blendEnabledLabel->setVisible(m_controls->m_stereoLinkModel.value() == 4);
}


void CompressorControlDialog::lookaheadChanged()
{
	m_lookaheadLengthKnob->setVisible(m_controls->m_lookaheadModel.value());
	m_lookaheadEnabledLabel->setVisible(m_controls->m_lookaheadModel.value());
}


void CompressorControlDialog::limiterChanged()
{
	m_ratioKnob->setVisible(!m_controls->m_limiterModel.value());
	m_ratioEnabledLabel->setVisible(!m_controls->m_limiterModel.value());
}


void CompressorControlDialog::updateDisplay()
{
	if (!isVisible())
	{
		return;
	}

	int elapsedMil = m_timeElapsed.elapsed();
	m_timeElapsed.restart();
	m_timeSinceLastUpdate += elapsedMil;
	int compPixelMovement = int(m_timeSinceLastUpdate / COMP_MILLI_PER_PIXEL);
	m_timeSinceLastUpdate %= COMP_MILLI_PER_PIXEL;

	// Time Change / Daylight Savings Time protection
	if (!compPixelMovement || compPixelMovement <= 0)
	{
		return;
	}

	if (!m_controls->m_effect->isEnabled() || !m_controls->m_effect->isRunning())
	{
		m_controls->m_effect->m_displayPeak[0] = COMP_NOISE_FLOOR;
		m_controls->m_effect->m_displayPeak[1] = COMP_NOISE_FLOOR;
		m_controls->m_effect->m_displayGain[0] = 1;
		m_controls->m_effect->m_displayGain[1] = 1;
		m_lastPoint = dbfsToYPoint(-9999);
		m_lastGainPoint = dbfsToYPoint(0);
	}

	const float peakAvg = (m_controls->m_effect->m_displayPeak[0] + m_controls->m_effect->m_displayPeak[1]) * 0.5f;
	const float gainAvg = (m_controls->m_effect->m_displayGain[0] + m_controls->m_effect->m_displayGain[1]) * 0.5f;

	QPainter p;
	float yPoint = dbfsToYPoint(ampToDbfs(peakAvg));
	float yGainPoint = dbfsToYPoint(ampToDbfs(gainAvg));

	int threshYPoint = dbfsToYPoint(m_controls->m_effect->m_thresholdVal);
	int threshXPoint = m_kneeWindowSizeY - threshYPoint;

	p.begin(&m_visPixmap);

	// Move entire display to the left
	p.setCompositionMode(QPainter::CompositionMode_Source);
	p.drawPixmap(-compPixelMovement, 0, m_visPixmap);
	p.fillRect(m_windowSizeX-compPixelMovement, 0, m_windowSizeX, m_windowSizeY, QColor("transparent"));
	p.setCompositionMode(QPainter::CompositionMode_SourceOver);

	p.setRenderHint(QPainter::Antialiasing, true);

	// Draw translucent portion of input volume line
	p.setPen(QPen(m_inVolAreaColor, 1));
	for (int i = 0; i < compPixelMovement; ++i)
	{
		const int temp = linearInterpolate(m_lastPoint, yPoint, float(i) / float(compPixelMovement));
		p.drawLine(m_windowSizeX-compPixelMovement+i, temp, m_windowSizeX-compPixelMovement+i, m_windowSizeY);
	}

	// Draw input volume line
	p.setPen(QPen(m_inVolColor, 1));
	p.drawLine(m_windowSizeX-compPixelMovement-1, m_lastPoint, m_windowSizeX, yPoint);

	// Draw translucent portion of output volume line
	p.setPen(QPen(m_outVolAreaColor, 1));
	for (int i = 0; i < compPixelMovement; ++i)
	{
		const int temp = linearInterpolate(m_lastPoint+m_lastGainPoint, yPoint+yGainPoint, float(i) / float(compPixelMovement));
		p.drawLine(m_windowSizeX-compPixelMovement+i, temp, m_windowSizeX-compPixelMovement+i, m_windowSizeY);
	}

	// Draw output volume line
	p.setPen(QPen(m_outVolColor, 1));
	p.drawLine(m_windowSizeX-compPixelMovement-1, m_lastPoint+m_lastGainPoint, m_windowSizeX, yPoint+yGainPoint);

	// Draw gain reduction line
	p.setPen(QPen(m_gainReductionColor, 2));
	p.drawLine(m_windowSizeX-compPixelMovement-1, m_lastGainPoint, m_windowSizeX, yGainPoint);

	p.end();

	if (m_controls->m_effect->m_redrawKnee)
	{
		m_controls->m_effect->m_redrawKnee = false;

		// Start drawing knee visualizer
		p.begin(&m_kneePixmap);

		p.setRenderHint(QPainter::Antialiasing, false);

		// Clear display
		p.setCompositionMode(QPainter::CompositionMode_Source);
		p.fillRect(0, 0, m_windowSizeX, m_kneeWindowSizeY, QColor("transparent"));
		p.setCompositionMode(QPainter::CompositionMode_SourceOver);

		p.setRenderHint(QPainter::Antialiasing, true);

		p.setPen(QPen(m_kneeColor, 3));

		// Limiter = infinite ratio
		float actualRatio = m_controls->m_limiterModel.value() ? 0 : m_controls->m_effect->m_ratioVal;

		// Calculate endpoints for the two straight lines
		float kneePoint1 = m_controls->m_effect->m_thresholdVal - m_controls->m_effect->m_kneeVal;
		float kneePoint2X = m_controls->m_effect->m_thresholdVal + m_controls->m_effect->m_kneeVal;
		float kneePoint2Y = (m_controls->m_effect->m_thresholdVal + (-m_controls->m_effect->m_thresholdVal * (actualRatio * (m_controls->m_effect->m_kneeVal / -m_controls->m_effect->m_thresholdVal))));
		float ratioPoint = m_controls->m_effect->m_thresholdVal + (-m_controls->m_effect->m_thresholdVal * actualRatio);

		// Draw two straight lines
		p.drawLine(0, m_kneeWindowSizeY, dbfsToXPoint(kneePoint1), dbfsToYPoint(kneePoint1));
		if (dbfsToXPoint(kneePoint2X) < m_kneeWindowSizeY)
		{
			p.drawLine(dbfsToXPoint(kneePoint2X), dbfsToYPoint(kneePoint2Y), m_kneeWindowSizeY, dbfsToYPoint(ratioPoint));
		}

		// Draw knee section
		if (m_controls->m_effect->m_kneeVal)
		{
			p.setPen(QPen(m_kneeColor2, 3));

			float prevPoint[2] = {kneePoint1, kneePoint1};
			float newPoint[2] = {0, 0};

			// Draw knee curve using many straight lines.
			for (int i = 0; i < COMP_KNEE_LINES; ++i)
			{
				newPoint[0] = linearInterpolate(kneePoint1, kneePoint2X, (i + 1) / (float)COMP_KNEE_LINES);

				const float temp = newPoint[0] - m_controls->m_effect->m_thresholdVal + m_controls->m_effect->m_kneeVal;
				newPoint[1] = (newPoint[0] + (actualRatio - 1) * temp * temp / (4 * m_controls->m_effect->m_kneeVal));

				p.drawLine(dbfsToXPoint(prevPoint[0]), dbfsToYPoint(prevPoint[1]), dbfsToXPoint(newPoint[0]), dbfsToYPoint(newPoint[1]));

				prevPoint[0] = newPoint[0];
				prevPoint[1] = newPoint[1];
			}
		}

		p.setRenderHint(QPainter::Antialiasing, false);

		// Erase right portion
		p.setCompositionMode(QPainter::CompositionMode_Source);
		p.fillRect(m_kneeWindowSizeX + 1, 0, m_windowSizeX, m_kneeWindowSizeY, QColor("transparent"));
		p.setCompositionMode(QPainter::CompositionMode_SourceOver);

		p.end();

		p.begin(&m_kneePixmap2);
		
		p.setCompositionMode(QPainter::CompositionMode_Source);
		p.fillRect(0, 0, m_windowSizeX, m_kneeWindowSizeY, QColor("transparent"));
		p.setCompositionMode(QPainter::CompositionMode_SourceOver);

		p.end();

		m_lastKneePoint = 0;
	}

	// Start drawing second knee layer
	p.begin(&m_kneePixmap2);

	p.setRenderHint(QPainter::Antialiasing, false);

	int kneePoint = dbfsToXPoint(ampToDbfs(peakAvg));
	if (kneePoint > m_lastKneePoint)
	{
		QRectF knee2Rect = QRect(m_lastKneePoint, 0, kneePoint - m_lastKneePoint, m_kneeWindowSizeY);
		p.drawPixmap(knee2Rect, m_kneePixmap, knee2Rect);
	}
	else
	{
		p.setCompositionMode(QPainter::CompositionMode_Source);
		p.fillRect(kneePoint, 0, m_lastKneePoint, m_kneeWindowSizeY, QColor("transparent"));
		p.setCompositionMode(QPainter::CompositionMode_SourceOver);
	}
	m_lastKneePoint = kneePoint;

	p.end();

	if (m_controls->m_effect->m_redrawThreshold)
	{
		p.begin(&m_miscPixmap);

		p.setCompositionMode(QPainter::CompositionMode_Source);
		p.fillRect(0, 0, m_windowSizeX, m_windowSizeY, QColor("transparent"));
		p.setCompositionMode(QPainter::CompositionMode_SourceOver);

		p.setRenderHint(QPainter::Antialiasing, true);

		// Draw threshold lines
		p.setPen(QPen(m_threshColor, 2, Qt::DotLine));
		p.drawLine(0, threshYPoint, m_windowSizeX, threshYPoint);
		p.drawLine(threshXPoint, 0, threshXPoint, m_kneeWindowSizeY);

		p.end();

		m_controls->m_effect->m_redrawThreshold = false;
	}

	m_lastPoint = yPoint;
	m_lastGainPoint = yGainPoint;

	update();
}



void CompressorControlDialog::paintEvent(QPaintEvent *event)
{
	if (isVisible())
	{
		QPainter p(this);

		p.setCompositionMode(QPainter::CompositionMode_Source);
		p.fillRect(0, 0, m_windowSizeX, m_windowSizeY, QColor("transparent"));
		p.setCompositionMode(QPainter::CompositionMode_SourceOver);

		p.drawPixmap(0, 0, m_graphPixmap);
		p.drawPixmap(0, 0, m_visPixmap);
		p.setOpacity(0.25);
		p.drawPixmap(0, 0, m_kneePixmap);
		p.setOpacity(1);
		p.drawPixmap(0, 0, m_kneePixmap2);
		p.setOpacity(1);
		p.drawPixmap(0, 0, m_miscPixmap);

		p.end();
	}
}


inline int CompressorControlDialog::dbfsToYPoint(float inDbfs)
{
	return (-((inDbfs + m_dbRange) / m_dbRange) + 1) * m_windowSizeY;
}

inline int CompressorControlDialog::dbfsToXPoint(float inDbfs)
{
	return m_kneeWindowSizeY - dbfsToYPoint(inDbfs);
}


void CompressorControlDialog::resizeEvent(QResizeEvent *event)
{
	resetCompressorView();
}


void CompressorControlDialog::wheelEvent(QWheelEvent * event)
{
	const float temp = m_dbRange;
	m_dbRange = round(qBound(3.f, m_dbRange - event->delta() / 20.f, 96.f) / 3.f) * 3.f;

	// Only reset view if the scolling had an effect
	if (m_dbRange != temp)
	{
		resetGraph();
		m_controls->m_effect->m_redrawKnee = true;
		m_controls->m_effect->m_redrawThreshold = true;
	}
}


void CompressorControlDialog::resetGraph()
{
	QPainter p;

	p.begin(&m_graphPixmap);

	p.setRenderHint(QPainter::Antialiasing, false);

	p.setCompositionMode(QPainter::CompositionMode_Source);
	p.fillRect(0, 0, m_windowSizeX, m_windowSizeY, QColor("transparent"));
	p.setCompositionMode(QPainter::CompositionMode_SourceOver);

	p.setPen(QPen(m_textColor, 1));
	p.setFont(QFont("Arial", qMax(int(m_windowSizeY / 1080.f * 24), 12)));

	// Redraw graph
	p.setPen(QPen(m_graphColor, 1));
	for (int i = 1; i < m_dbRange / 3.f + 1; ++i)
	{
		p.drawLine(0, dbfsToYPoint(-3 * i), m_windowSizeX, dbfsToYPoint(-3 * i));
		p.drawLine(dbfsToXPoint(-3 * i), 0, dbfsToXPoint(-3 * i), m_kneeWindowSizeY);
		p.drawText(QRectF(m_windowSizeX - 50, dbfsToYPoint(-3 * i), 50, 50), Qt::AlignRight | Qt::AlignTop, QString::number(i * -3));
	}

	p.end();
}


void CompressorControlDialog::resetCompressorView()
{
	m_windowSizeX = size().width();
	m_windowSizeY = size().height();
	m_kneeWindowSizeX = m_windowSizeY;
	m_kneeWindowSizeY = m_windowSizeY;
	m_controlsBoxX = (m_windowSizeX - COMP_BOX_X) * 0.5;
	m_controlsBoxY = m_windowSizeY - 40 - COMP_BOX_Y;

	m_controls->m_effect->m_redrawKnee = true;
	m_controls->m_effect->m_redrawThreshold = true;
	m_lastKneePoint = 0;

	QPainter p;

	resetGraph();

	p.begin(&m_visPixmap);

	p.setCompositionMode(QPainter::CompositionMode_Source);
	p.fillRect(0, 0, m_windowSizeX, m_windowSizeY, QColor("transparent"));
	p.setCompositionMode(QPainter::CompositionMode_SourceOver);

	// Draw line at right side, so the sudden
	// content that the visualizer will display
	// later on won't look too ugly
	p.setPen(QPen(m_resetColor, 3));
	p.drawLine(m_windowSizeX, 0, m_windowSizeX, m_windowSizeY);

	p.end();

	m_controlsBoxLabel->move(m_controlsBoxX, m_controlsBoxY);
	m_rmsEnabledLabel->move(m_controlsBoxX + 429, m_controlsBoxY + 209);
	m_blendEnabledLabel->move(m_controlsBoxX + 587, m_controlsBoxY + 197);
	m_lookaheadEnabledLabel->move(m_controlsBoxX + 221, m_controlsBoxY + 135);
	m_ratioEnabledLabel->move(m_controlsBoxX + 267, m_controlsBoxY + 21);

	m_thresholdKnob->move(m_controlsBoxX + 137, m_controlsBoxY + 21);
	m_ratioKnob->move(m_controlsBoxX + 267, m_controlsBoxY + 21);
	m_attackKnob->move(m_controlsBoxX + 397, m_controlsBoxY + 21);
	m_releaseKnob->move(m_controlsBoxX + 527, m_controlsBoxY + 21);
	m_kneeKnob->move(m_controlsBoxX + 97, m_controlsBoxY + 135);
	m_rangeKnob->move(m_controlsBoxX + 159, m_controlsBoxY + 135);
	m_lookaheadLengthKnob->move(m_controlsBoxX + 221, m_controlsBoxY + 135);
	m_holdKnob->move(m_controlsBoxX + 283, m_controlsBoxY + 135);

	m_rmsKnob->move(m_controlsBoxX + 429, m_controlsBoxY + 209);
	m_inBalanceKnob->move(m_controlsBoxX + 27, m_controlsBoxY + 219);
	m_outBalanceKnob->move(m_controlsBoxX + 662, m_controlsBoxY + 219);
	m_stereoBalanceKnob->move(m_controlsBoxX + 522, m_controlsBoxY + 137);
	m_blendKnob->move(m_controlsBoxX + 587, m_controlsBoxY + 197);
	m_tiltKnob->move(m_controlsBoxX + 364, m_controlsBoxY + 138);
	m_tiltFreqKnob->move(m_controlsBoxX + 415, m_controlsBoxY + 138);
	m_mixKnob->move(m_controlsBoxX + 27, m_controlsBoxY + 13);

	m_outFader->move(m_controlsBoxX + 666, m_controlsBoxY + 91);
	m_inFader->move(m_controlsBoxX + 31, m_controlsBoxY + 91);

	rmsButton->move(m_controlsBoxX + 337, m_controlsBoxY + 231);
	peakButton->move(m_controlsBoxX + 337, m_controlsBoxY + 248);

	leftRightButton->move(m_controlsBoxX + 220, m_controlsBoxY + 231);
	midSideButton->move(m_controlsBoxX + 220, m_controlsBoxY + 248);

	compressButton->move(m_controlsBoxX + 98, m_controlsBoxY + 231);
	limitButton->move(m_controlsBoxX + 98, m_controlsBoxY + 248);

	unlinkedButton->move(m_controlsBoxX + 495, m_controlsBoxY + 180);
	maximumButton->move(m_controlsBoxX + 495, m_controlsBoxY + 197);
	averageButton->move(m_controlsBoxX + 495, m_controlsBoxY + 214);
	minimumButton->move(m_controlsBoxX + 495, m_controlsBoxY + 231);
	blendButton->move(m_controlsBoxX + 495, m_controlsBoxY + 248);

	autoMakeupButton->move(m_controlsBoxX + 220, m_controlsBoxY + 206);
	auditionButton->move(m_controlsBoxX + 658, m_controlsBoxY + 14);
	feedbackButton->move(m_controlsBoxX + 98, m_controlsBoxY + 206);
	m_autoAttackKnob->move(m_controlsBoxX + 460, m_controlsBoxY + 38);
	m_autoReleaseKnob->move(m_controlsBoxX + 590, m_controlsBoxY + 38);
	lookaheadButton->move(m_controlsBoxX + 202, m_controlsBoxY + 171);
}

// For theming purposes
QColor const & CompressorControlDialog::inVolAreaColor() const {return m_inVolAreaColor;}
QColor const & CompressorControlDialog::inVolColor() const {return m_inVolColor;}
QColor const & CompressorControlDialog::outVolAreaColor() const {return m_outVolAreaColor;}
QColor const & CompressorControlDialog::outVolColor() const {return m_outVolColor;}
QColor const & CompressorControlDialog::gainReductionColor() const {return m_gainReductionColor;}
QColor const & CompressorControlDialog::kneeColor() const {return m_kneeColor;}
QColor const & CompressorControlDialog::kneeColor2() const {return m_kneeColor2;}
QColor const & CompressorControlDialog::threshColor() const {return m_threshColor;}
QColor const & CompressorControlDialog::textColor() const {return m_textColor;}
QColor const & CompressorControlDialog::graphColor() const {return m_graphColor;}
QColor const & CompressorControlDialog::resetColor() const {return m_resetColor;}

void CompressorControlDialog::setInVolAreaColor(const QColor & c){m_inVolAreaColor = c;}
void CompressorControlDialog::setInVolColor(const QColor & c){m_inVolColor = c;}
void CompressorControlDialog::setOutVolAreaColor(const QColor & c){m_outVolAreaColor = c;}
void CompressorControlDialog::setOutVolColor(const QColor & c){m_outVolColor = c;}
void CompressorControlDialog::setGainReductionColor(const QColor & c){m_gainReductionColor = c;}
void CompressorControlDialog::setKneeColor(const QColor & c){m_kneeColor = c;}
void CompressorControlDialog::setKneeColor2(const QColor & c){m_kneeColor2 = c;}
void CompressorControlDialog::setThreshColor(const QColor & c){m_threshColor = c;}
void CompressorControlDialog::setTextColor(const QColor & c){m_textColor = c;}
void CompressorControlDialog::setGraphColor(const QColor & c){m_graphColor = c;}
void CompressorControlDialog::setResetColor(const QColor & c){m_resetColor = c;}
