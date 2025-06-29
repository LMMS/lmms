/*
 * LOMMControlDialog.cpp
 *
 * Copyright (c) 2023 Lost Robot <r94231/at/gmail/dot/com>
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


#include "LOMM.h"
#include "LOMMControlDialog.h"
#include "LOMMControls.h"


namespace lmms::gui
{

LOMMControlDialog::LOMMControlDialog(LOMMControls* controls) :
	EffectControlDialog(controls),
	m_controls(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(400, 256);

	createKnob(KnobType::Bright26, this, 10, 4, &controls->m_depthModel, tr("Depth:"), "", tr("Compression amount for all bands"));
	createKnob(KnobType::Bright26, this, 10, 41, &controls->m_timeModel, tr("Time:"), "", tr("Attack/release scaling for all bands"));
	createKnob(KnobType::Bright26, this, 10, 220, &controls->m_inVolModel, tr("Input Volume:"), " dB", tr("Input volume"));
	createKnob(KnobType::Bright26, this, 363, 220, &controls->m_outVolModel, tr("Output Volume:"), " dB", tr("Output volume"));
	createKnob(KnobType::Bright26, this, 10, 179, &controls->m_upwardModel, tr("Upward Depth:"), "", tr("Upward compression amount for all bands"));
	createKnob(KnobType::Bright26, this, 363, 179, &controls->m_downwardModel, tr("Downward Depth:"), "", tr("Downward compression amount for all bands"));

	createLcdFloatSpinBox(5, 2, "11green", tr("High/Mid Crossover"), this, 352, 76, &controls->m_split1Model, tr("High/Mid Crossover"));
	createLcdFloatSpinBox(5, 2, "11green", tr("Mid/Low Crossover"), this, 352, 156, &controls->m_split2Model, tr("Mid/Low Crossover"));

	createPixmapButton(tr("High/mid band split"), this, 369, 104, &controls->m_split1EnabledModel, "crossover_led_green", "crossover_led_off", tr("High/mid band split"));
	createPixmapButton(tr("Mid/low band split"), this, 369, 126, &controls->m_split2EnabledModel, "crossover_led_green", "crossover_led_off", tr("Mid/low band split"));

	createPixmapButton(tr("Enable High Band"), this, 143, 66, &controls->m_band1EnabledModel, "high_band_active", "high_band_inactive", tr("Enable High Band"));
	createPixmapButton(tr("Enable Mid Band"), this, 143, 146, &controls->m_band2EnabledModel, "mid_band_active", "mid_band_inactive", tr("Enable Mid Band"));
	createPixmapButton(tr("Enable Low Band"), this, 143, 226, &controls->m_band3EnabledModel, "low_band_active", "low_band_inactive", tr("Enable Low Band"));

	createKnob(KnobType::Bright26, this, 53, 43, &controls->m_inHighModel, tr("High Input Volume:"), " dB", tr("Input volume for high band"));
	createKnob(KnobType::Bright26, this, 53, 123, &controls->m_inMidModel, tr("Mid Input Volume:"), " dB", tr("Input volume for mid band"));
	createKnob(KnobType::Bright26, this, 53, 203, &controls->m_inLowModel, tr("Low Input Volume:"), " dB", tr("Input volume for low band"));
	createKnob(KnobType::Bright26, this, 320, 43, &controls->m_outHighModel, tr("High Output Volume:"), " dB", tr("Output volume for high band"));
	createKnob(KnobType::Bright26, this, 320, 123, &controls->m_outMidModel, tr("Mid Output Volume:"), " dB", tr("Output volume for mid band"));
	createKnob(KnobType::Bright26, this, 320, 203, &controls->m_outLowModel, tr("Low Output Volume:"), " dB", tr("Output volume for low band"));

	createLcdFloatSpinBox(3, 3, "11green", tr("Above Threshold High"), this, 300, 13, &controls->m_aThreshHModel, tr("Downward compression threshold for high band"));
	createLcdFloatSpinBox(3, 3, "11green", tr("Above Threshold Mid"), this, 300, 93, &controls->m_aThreshMModel, tr("Downward compression threshold for mid band"));
	createLcdFloatSpinBox(3, 3, "11green", tr("Above Threshold Low"), this, 300, 173, &controls->m_aThreshLModel, tr("Downward compression threshold for low band"));
	createLcdFloatSpinBox(2, 2, "11green", tr("Above Ratio High"), this, 284, 44, &controls->m_aRatioHModel, tr("Downward compression ratio for high band"));
	createLcdFloatSpinBox(2, 2, "11green", tr("Above Ratio Mid"), this, 284, 124, &controls->m_aRatioMModel, tr("Downward compression ratio for mid band"));
	createLcdFloatSpinBox(2, 2, "11green", tr("Above Ratio Low"), this, 284, 204, &controls->m_aRatioLModel, tr("Downward compression ratio for low band"));

	createLcdFloatSpinBox(3, 3, "11green", tr("Below Threshold High"), this, 59, 13, &controls->m_bThreshHModel, tr("Upward compression threshold for high band"));
	createLcdFloatSpinBox(3, 3, "11green", tr("Below Threshold Mid"), this, 59, 93, &controls->m_bThreshMModel, tr("Upward compression threshold for mid band"));
	createLcdFloatSpinBox(3, 3, "11green", tr("Below Threshold Low"), this, 59, 173, &controls->m_bThreshLModel, tr("Upward compression threshold for low band"));
	createLcdFloatSpinBox(2, 2, "11green", tr("Below Ratio High"), this, 87, 44, &controls->m_bRatioHModel, tr("Upward compression ratio for high band"));
	createLcdFloatSpinBox(2, 2, "11green", tr("Below Ratio Mid"), this, 87, 124, &controls->m_bRatioMModel, tr("Upward compression ratio for mid band"));
	createLcdFloatSpinBox(2, 2, "11green", tr("Below Ratio Low"), this, 87, 204, &controls->m_bRatioLModel, tr("Upward compression ratio for low band"));

	createKnob(KnobType::Small17, this, 120, 61, &controls->m_atkHModel, tr("Attack High:"), " ms", tr("Attack time for high band"));
	createKnob(KnobType::Small17, this, 120, 141, &controls->m_atkMModel, tr("Attack Mid:"), " ms", tr("Attack time for mid band"));
	createKnob(KnobType::Small17, this, 120, 221, &controls->m_atkLModel, tr("Attack Low:"), " ms", tr("Attack time for low band"));
	createKnob(KnobType::Small17, this, 261, 61, &controls->m_relHModel, tr("Release High:"), " ms", tr("Release time for high band"));
	createKnob(KnobType::Small17, this, 261, 141, &controls->m_relMModel, tr("Release Mid:"), " ms", tr("Release time for mid band"));
	createKnob(KnobType::Small17, this, 261, 221, &controls->m_relLModel, tr("Release Low:"), " ms", tr("Release time for low band"));

	createKnob(KnobType::Small17, this, 380, 42, &controls->m_rmsTimeModel, tr("RMS Time:"), " ms", tr("RMS size for sidechain signal (set to 0 for Peak mode)"));
	createKnob(KnobType::Small17, this, 356, 42, &controls->m_kneeModel, tr("Knee:"), " dB", tr("Knee size for all compressors"));
	createKnob(KnobType::Small17, this, 24, 146, &controls->m_rangeModel, tr("Range:"), " dB", tr("Maximum gain increase for all bands"));
	createKnob(KnobType::Small17, this, 13, 114, &controls->m_balanceModel, tr("Balance:"), " dB", tr("Bias input volume towards one channel"));

	createPixmapButton(tr("Scale output volume with Depth"), this, 51, 0, &controls->m_depthScalingModel, "depthScaling_active", "depthScaling_inactive",
						tr("Scale output volume with Depth parameter"));
	createPixmapButton(tr("Stereo Link"), this, 52, 237, &controls->m_stereoLinkModel, "stereoLink_active", "stereoLink_inactive",
						tr("Apply same gain change to both channels"));

	createKnob(KnobType::Small17, this, 24, 80, &controls->m_autoTimeModel, tr("Auto Time:"), "", tr("Speed up attack and release times when transients occur"));
	createKnob(KnobType::Bright26, this, 363, 4, &controls->m_mixModel, tr("Mix:"), "", tr("Wet/Dry of all bands"));

	m_feedbackButton = createPixmapButton(tr("Feedback"), this, 317, 238, &controls->m_feedbackModel, "feedback_active", "feedback_inactive",
										tr("Use output as sidechain signal instead of input"));
	createPixmapButton(tr("Mid/Side"), this, 285, 238, &controls->m_midsideModel, "midside_active", "midside_inactive", tr("Compress mid/side channels instead of left/right"));
	m_lowSideUpwardSuppressButton = createPixmapButton(tr("Suppress upward compression for side band"), this, 106, 180, &controls->m_lowSideUpwardSuppressModel,
														"lowSideUpwardSuppress_active", "lowSideUpwardSuppress_inactive", tr("Suppress upward compression for side band"));
	createPixmapButton(tr("Lookahead"), this, 147, 0, &controls->m_lookaheadEnableModel, "lookahead_active", "lookahead_inactive",
						tr(("Enable lookahead with fixed " + std::to_string(int(LOMM_MAX_LOOKAHEAD)) + " ms latency").c_str()));
	createLcdFloatSpinBox(2, 2, "11green", tr("Lookahead"), this, 214, 2, &controls->m_lookaheadModel, tr("Lookahead length"));

	PixmapButton* initButton = createPixmapButton(tr("Clear all parameters"), this, 84, 237, nullptr, "init_active", "init_inactive", tr("Clear all parameters"));
	initButton->setCheckable(false);

	connect(initButton, SIGNAL(clicked()), m_controls, SLOT(resetAllParameters()));
	connect(&controls->m_lookaheadEnableModel, SIGNAL(dataChanged()), this, SLOT(updateFeedbackVisibility()));
	connect(&controls->m_midsideModel, SIGNAL(dataChanged()), this, SLOT(updateLowSideUpwardSuppressVisibility()));
	connect(getGUI()->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(updateDisplay()));

	emit updateFeedbackVisibility();
	emit updateLowSideUpwardSuppressVisibility();
}

void LOMMControlDialog::updateFeedbackVisibility()
{
	m_feedbackButton->setVisible(!m_controls->m_lookaheadEnableModel.value());
}

void LOMMControlDialog::updateLowSideUpwardSuppressVisibility()
{
	m_lowSideUpwardSuppressButton->setVisible(m_controls->m_midsideModel.value());
}

void LOMMControlDialog::updateDisplay()
{
	update();
}

void LOMMControlDialog::paintEvent(QPaintEvent *event)
{
	if (!isVisible()) { return; }

	QPainter p;
	p.begin(this);

	// Draw threshold lines
	QColor aColor(255, 255, 0, 31);
	QColor bColor(255, 0, 0, 31);
	QPen aPen(QColor(255, 255, 0, 255), 1);
	QPen bPen(QColor(255, 0, 0, 255), 1);
	int thresholdsX[] = {dbfsToX(m_controls->m_aThreshHModel.value()),
						dbfsToX(m_controls->m_aThreshMModel.value()),
						dbfsToX(m_controls->m_aThreshLModel.value()),
						dbfsToX(m_controls->m_bThreshHModel.value()),
						dbfsToX(m_controls->m_bThreshMModel.value()),
						dbfsToX(m_controls->m_bThreshLModel.value())};
	for (int i = 0; i < 3; ++i) {
		p.setPen(aPen);
		p.fillRect(thresholdsX[i], LOMM_DISPLAY_Y[2 * i], LOMM_DISPLAY_X + LOMM_DISPLAY_WIDTH - thresholdsX[i], LOMM_DISPLAY_Y[2 * i + 1] + LOMM_DISPLAY_HEIGHT - LOMM_DISPLAY_Y[2 * i], aColor);
		p.drawLine(thresholdsX[i], LOMM_DISPLAY_Y[2 * i], thresholdsX[i], LOMM_DISPLAY_Y[2 * i + 1] + LOMM_DISPLAY_HEIGHT);

		p.setPen(bPen);
		p.fillRect(LOMM_DISPLAY_X, LOMM_DISPLAY_Y[2 * i], thresholdsX[i + 3] - LOMM_DISPLAY_X, LOMM_DISPLAY_Y[2 * i + 1] + LOMM_DISPLAY_HEIGHT - LOMM_DISPLAY_Y[2 * i], bColor);
		p.drawLine(thresholdsX[i + 3], LOMM_DISPLAY_Y[2 * i], thresholdsX[i + 3], LOMM_DISPLAY_Y[2 * i + 1] + LOMM_DISPLAY_HEIGHT);
	}

	QPen inputPen(QColor(200, 200, 200, 80), 1);
	QPen outputPen(QColor(255, 255, 255, 255), 1);
	for (int i = 0; i < 3; ++i) {
		// Draw input lines
		p.setPen(inputPen);
		int inL = dbfsToX(m_controls->m_effect->m_displayIn[i][0]);
		p.drawLine(inL, LOMM_DISPLAY_Y[2 * i] + 4, inL, LOMM_DISPLAY_Y[2 * i] + LOMM_DISPLAY_HEIGHT);
		int inR = dbfsToX(m_controls->m_effect->m_displayIn[i][1]);
		p.drawLine(inR, LOMM_DISPLAY_Y[2 * i + 1], inR, LOMM_DISPLAY_Y[2 * i + 1] + LOMM_DISPLAY_HEIGHT - 4);

		// Draw output lines
		p.setPen(outputPen);
		int outL = dbfsToX(m_controls->m_effect->m_displayOut[i][0]);
		p.drawLine(outL, LOMM_DISPLAY_Y[2 * i], outL, LOMM_DISPLAY_Y[2 * i] + LOMM_DISPLAY_HEIGHT);
		int outR = dbfsToX(m_controls->m_effect->m_displayOut[i][1]);
		p.drawLine(outR, LOMM_DISPLAY_Y[2 * i + 1], outR, LOMM_DISPLAY_Y[2 * i + 1] + LOMM_DISPLAY_HEIGHT);
	}

	p.end();
}

int LOMMControlDialog::dbfsToX(float dbfs)
{
	float returnX = (dbfs - LOMM_DISPLAY_MIN) / (LOMM_DISPLAY_MAX - LOMM_DISPLAY_MIN);
	returnX = qBound(LOMM_DISPLAY_X, LOMM_DISPLAY_X + returnX * LOMM_DISPLAY_WIDTH, LOMM_DISPLAY_X + LOMM_DISPLAY_WIDTH);
	return returnX;
}

float LOMMControlDialog::xToDbfs(int x)
{
	float xNorm = static_cast<float>(x - LOMM_DISPLAY_X) / LOMM_DISPLAY_WIDTH;
	float dbfs = xNorm * (LOMM_DISPLAY_MAX - LOMM_DISPLAY_MIN) + LOMM_DISPLAY_MIN;
	return dbfs;
}

void LOMMControlDialog::mousePressEvent(QMouseEvent* event)
{
	if ((event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) && !(event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)))
	{
		const QPoint& p = event->pos();

		if (LOMM_DISPLAY_X - 10 <= p.x() && p.x() <= LOMM_DISPLAY_X + LOMM_DISPLAY_WIDTH + 10)
		{
			FloatModel* aThresh[] = {&m_controls->m_aThreshHModel, &m_controls->m_aThreshMModel, &m_controls->m_aThreshLModel};
			FloatModel* bThresh[] = {&m_controls->m_bThreshHModel, &m_controls->m_bThreshMModel, &m_controls->m_bThreshLModel};

			for (int i = 0; i < 3; ++i)
			{
				if (LOMM_DISPLAY_Y[i * 2] <= p.y() && p.y() <= LOMM_DISPLAY_Y[i * 2 + 1] + LOMM_DISPLAY_HEIGHT)
				{
					int behavior = (p.x() < dbfsToX(bThresh[i]->value())) ? 0 : (p.x() > dbfsToX(aThresh[i]->value())) ? 1 : 2;
					if (event->button() == Qt::MiddleButton)
					{
						if (behavior == 0 || behavior == 2) {bThresh[i]->reset();}
						if (behavior == 1 || behavior == 2) {aThresh[i]->reset();}
						return;
					}

					m_bandDrag = i;
					m_lastMousePos = p;
					m_buttonPressed = true;

					m_dragType = behavior;
					return;
				}
			}
		}
	}
}

void LOMMControlDialog::mouseMoveEvent(QMouseEvent * event)
{
	if (m_buttonPressed && event->pos() != m_lastMousePos)
	{
		const float distance = event->pos().x() - m_lastMousePos.x();
		float dbDistance = distance * LOMM_DISPLAY_DB_PER_PIXEL;
		m_lastMousePos = event->pos();

		FloatModel* aModel[] = {&m_controls->m_aThreshHModel, &m_controls->m_aThreshMModel, &m_controls->m_aThreshLModel};
		FloatModel* bModel[] = {&m_controls->m_bThreshHModel, &m_controls->m_bThreshMModel, &m_controls->m_bThreshLModel};

		float bVal = bModel[m_bandDrag]->value();
		float aVal = aModel[m_bandDrag]->value();
		if (m_dragType == 0)
		{
			bModel[m_bandDrag]->setValue(bVal + dbDistance);
		}
		else if (m_dragType == 1)
		{
			aModel[m_bandDrag]->setValue(aVal + dbDistance);
		}
		else
		{
			dbDistance = qBound(bModel[m_bandDrag]->minValue(), bVal + dbDistance, bModel[m_bandDrag]->maxValue()) - bVal;
			dbDistance = qBound(aModel[m_bandDrag]->minValue(), aVal + dbDistance, aModel[m_bandDrag]->maxValue()) - aVal;
			bModel[m_bandDrag]->setValue(bVal + dbDistance);
			aModel[m_bandDrag]->setValue(aVal + dbDistance);
		}
	}
}

void LOMMControlDialog::mouseReleaseEvent(QMouseEvent* event)
{
	if (event && event->button() == Qt::LeftButton)
	{
		m_buttonPressed = false;
	}
}


} // namespace lmms::gui
