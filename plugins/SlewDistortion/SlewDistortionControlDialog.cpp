/*
 * SlewDistortionControlDialog.cpp
 *
 * Copyright (c) 2025 Lost Robot <r94231/at/gmail/dot/com>
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

#include "SlewDistortionControlDialog.h"
#include "SlewDistortionControls.h"
#include "SlewDistortion.h"

#include "embed.h"
#include "Knob.h"
#include "MainWindow.h"
#include <QPainter>
#include "GuiApplication.h"
#include "PixmapButton.h"
#include "Draggable.h"
#include "lmms_math.h"

#include <QPainterPath>

namespace lmms::gui
{

SlewDistortionControlDialog::SlewDistortionControlDialog(SlewDistortionControls* controls) :
	EffectControlDialog(controls),
	m_controls(controls)
{
	using DirectionOfManipulation = FloatModelEditorBase::DirectionOfManipulation;

	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(638, 271);
	
	auto makeKnob = [this](int x, int y, const QString& hintText, const QString& unit, FloatModel* model, bool smol = false)
	{
		Knob* newKnob = new Knob(smol ? KnobType::Small17 : KnobType::Bright26, this);
		newKnob->move(x, y);
		newKnob->setModel(model);
		newKnob->setHintText(hintText, unit);
		return newKnob;
	};
	
	auto makeToggleButton = [this](int x, int y, const QString& tooltip, const std::string& activeIcon, const std::string& inactiveIcon, BoolModel* model)
	{
		PixmapButton* button = new PixmapButton(this, tooltip);
		button->setActiveGraphic(PLUGIN_NAME::getIconPixmap(activeIcon));
		button->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(inactiveIcon));
		button->setToolTip(tooltip);
		button->move(x, y);
		button->setCheckable(true);
		button->setModel(model);
		return button;
	};
	
	auto makeGroupButton = [this](int x, int y, const QString& tooltip, const std::string& activeIcon, const std::string& inactiveIcon)
	{
		PixmapButton* button = new PixmapButton(this, tooltip);
		button->setActiveGraphic(PLUGIN_NAME::getIconPixmap(activeIcon));
		button->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(inactiveIcon));
		button->setToolTip(tooltip);
		button->move(x, y);
		return button;
	};

	ComboBox* distType1Box = new ComboBox(this);
	distType1Box->setGeometry(85, 26, 115, 22);
	//distType1Box->setFont(pointSize<8>(distType1Box->font()));
	distType1Box->setModel(&controls->m_distType1Model);
	
	ComboBox* distType2Box = new ComboBox(this);
	distType2Box->setGeometry(85, 147, 115, 22);
	//distType2Box->setFont(pointSize<8>(distType2Box->font()));
	distType2Box->setModel(&controls->m_distType2Model);
	
	Draggable* drive1Draggable = new Draggable(DirectionOfManipulation::Vertical,
		&controls->m_drive1Model, PLUGIN_NAME::getIconPixmap("handle"), 108, 34, this);
	drive1Draggable->move(16, drive1Draggable->y());
	drive1Draggable->setDefaultValPixmap(PLUGIN_NAME::getIconPixmap("handle_zero"));
	Draggable* drive2Draggable = new Draggable(DirectionOfManipulation::Vertical,
		&controls->m_drive2Model, PLUGIN_NAME::getIconPixmap("handle"), 229, 155, this);
	drive2Draggable->move(16, drive2Draggable->y());
	drive2Draggable->setDefaultValPixmap(PLUGIN_NAME::getIconPixmap("handle_zero"));
	
	Draggable* bias1Draggable = new Draggable(DirectionOfManipulation::Vertical,
		&controls->m_bias1Model, PLUGIN_NAME::getIconPixmap("handle"), 112, 34, this);
	bias1Draggable->move(416, bias1Draggable->y());
	bias1Draggable->setDefaultValPixmap(PLUGIN_NAME::getIconPixmap("handle_zero"));
	Draggable* bias2Draggable = new Draggable(DirectionOfManipulation::Vertical,
		&controls->m_bias2Model, PLUGIN_NAME::getIconPixmap("handle"), 233, 155, this);
	bias2Draggable->move(416, bias2Draggable->y());
	bias2Draggable->setDefaultValPixmap(PLUGIN_NAME::getIconPixmap("handle_zero"));
	
	m_slewUp1Knob = makeKnob(96, 65, tr("Slew Up 1:"), "", &controls->m_slewUp1Model);
	m_slewUp2Knob = makeKnob(96, 186, tr("Slew Up 2:"), "", &controls->m_slewUp2Model);
	m_slewDown1Knob = makeKnob(163, 65, tr("Slew Down 1:"), "",
		controls->m_slewLink1Model.value() ? &controls->m_slewUp1Model : &controls->m_slewDown1Model);
	m_slewDown2Knob = makeKnob(163, 186, tr("Slew Down 2:"), "",
		controls->m_slewLink2Model.value() ? &controls->m_slewUp2Model : &controls->m_slewDown2Model);
	makeKnob(329, 26, tr("Warp 1:"), "", &controls->m_warp1Model);
	makeKnob(329, 147, tr("Warp 2:"), "", &controls->m_warp2Model);
	makeKnob(371, 26, tr("Crush 1:"), "", &controls->m_crush1Model);
	makeKnob(371, 147, tr("Crush 2:"), "", &controls->m_crush2Model);
	makeKnob(225, 65, tr("Attack 1:"), "", &controls->m_attack1Model);
	makeKnob(225, 186, tr("Attack 2:"), "", &controls->m_attack2Model);
	makeKnob(267, 65, tr("Release 1:"), "", &controls->m_release1Model);
	makeKnob(267, 186, tr("Release 2:"), "", &controls->m_release2Model);
	makeKnob(225, 26, tr("Dynamics 1:"), "", &controls->m_dynamics1Model);
	makeKnob(225, 147, tr("Dynamics 2:"), "", &controls->m_dynamics2Model);
	makeKnob(267, 26, tr("Dynamic Slew 1:"), "", &controls->m_dynamicSlew1Model);
	makeKnob(267, 147, tr("Dynamic Slew 2:"), "", &controls->m_dynamicSlew2Model);
	
	Draggable* outVol1Draggable = new Draggable(DirectionOfManipulation::Vertical,
		&controls->m_outVol1Model, PLUGIN_NAME::getIconPixmap("handle"), 108, 34, this);
	outVol1Draggable->move(594, outVol1Draggable->y());
	outVol1Draggable->setDefaultValPixmap(PLUGIN_NAME::getIconPixmap("handle_zero"));
	Draggable* outVol2Draggable = new Draggable(DirectionOfManipulation::Vertical,
		&controls->m_outVol2Model, PLUGIN_NAME::getIconPixmap("handle"), 229, 155, this);
	outVol2Draggable->move(594, outVol2Draggable->y());
	outVol2Draggable->setDefaultValPixmap(PLUGIN_NAME::getIconPixmap("handle_zero"));
	
	makeToggleButton(132, 70, tr("Slew Link 1"), "link_on", "link_off", &controls->m_slewLink1Model);
	connect(&controls->m_slewLink1Model, &BoolModel::dataChanged, this, [this, controls]{
		if (controls->m_slewLink1Model.value())
		{
			controls->m_slewDown1Model.setValue(controls->m_slewUp1Model.value());
			m_slewDown1Knob->setModel(&controls->m_slewUp1Model);
		}
		else
		{
			m_slewDown1Knob->setModel(&controls->m_slewDown1Model);
		}
	});
	makeToggleButton(132, 191, tr("Slew Link 2"), "link_on", "link_off", &controls->m_slewLink2Model);
	connect(&controls->m_slewLink2Model, &BoolModel::dataChanged, this, [this, controls]{
		if (controls->m_slewLink2Model.value())
		{
			controls->m_slewDown2Model.setValue(controls->m_slewUp2Model.value());
			m_slewDown2Knob->setModel(&controls->m_slewUp2Model);
		}
		else
		{
			m_slewDown2Knob->setModel(&controls->m_slewDown2Model);
		}
	});
	
	makeToggleButton(9, 248, tr("DC Offset Removal"), "dc_on", "dc_off", &controls->m_dcRemoveModel);
	makeToggleButton(99, 248, tr("Multiband"), "mb_on", "mb_off", &controls->m_multibandModel);

	makeKnob(190, 249, tr("Split:"), "", &controls->m_splitModel, true);
	makeKnob(338, 78, tr("Mix 1:"), "", &controls->m_mix1Model);
	makeKnob(338, 199, tr("Mix 2:"), "", &controls->m_mix2Model);
	
	PixmapButton* oversample1xButton = makeGroupButton(454, 248, tr("Disable Oversampling"), "oversample_1x_on", "oversample_1x_off");
	PixmapButton* oversample2xButton = makeGroupButton(479, 248, tr("2x Oversampling"), "oversample_2x_on", "oversample_2x_off");
	PixmapButton* oversample4xButton = makeGroupButton(504, 248, tr("4x Oversampling"), "oversample_4x_on", "oversample_4x_off");
	PixmapButton* oversample8xButton = makeGroupButton(529, 248, tr("8x Oversampling"), "oversample_8x_on", "oversample_8x_off");
	PixmapButton* oversample16xButton = makeGroupButton(554, 248, tr("16x Oversampling"), "oversample_16x_on", "oversample_16x_off");
	PixmapButton* oversample32xButton = makeGroupButton(579, 248, tr("32x Oversampling"), "oversample_32x_on", "oversample_32x_off");

	AutomatableButtonGroup* oversampleGroup = new AutomatableButtonGroup(this);
	oversampleGroup->addButton(oversample1xButton);
	oversampleGroup->addButton(oversample2xButton);
	oversampleGroup->addButton(oversample4xButton);
	oversampleGroup->addButton(oversample8xButton);
	oversampleGroup->addButton(oversample16xButton);
	oversampleGroup->addButton(oversample32xButton);
	oversampleGroup->setModel(&controls->m_oversamplingModel);
	
	PixmapButton* m_helpBtn = new PixmapButton(this, nullptr);
	m_helpBtn->move(614, 250);
	m_helpBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("help_on"));
	m_helpBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("help_off"));
	m_helpBtn->setToolTip(tr("Open help window"));
	connect(m_helpBtn, &PixmapButton::clicked, this, &SlewDistortionControlDialog::showHelpWindow);
	
	connect(getGUI()->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(update()));
}

void SlewDistortionControlDialog::paintEvent(QPaintEvent* event)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	
	QRect inMeters[] = { {22, 31, 8, 75}, {30, 31, 8, 75}, {22, 152, 8, 75}, {30, 152, 8, 75} };
	QRect outMeters[] = { {600, 31, 8, 75}, {608, 31, 8, 75}, {600, 152, 8, 75}, {608, 152, 8, 75} };

	float* inPeak = &m_controls->m_effect->m_inPeakDisplay[0];
	float* outPeak = &m_controls->m_effect->m_outPeakDisplay[0];

	for (int i = 0; i < 4; ++i)
	{
		m_lastInPeaks[i] = std::max((inPeak[i] != -1.0f) ? inPeak[i] : m_lastInPeaks[i], SLEW_DISTORTION_MIN_FLOOR);
		m_lastOutPeaks[i] = std::max((outPeak[i] != -1.0f) ? outPeak[i] : m_lastOutPeaks[i], SLEW_DISTORTION_MIN_FLOOR);
		inPeak[i] = outPeak[i] = -1.0f;
	}

	auto drawInverseMeters = [&p](const QRect meters[], const float values[], QColor coverColor)
	{
		const float dbfsMin = -24.0f;
		const float dbfsMax = 24.0f;

		for (int i = 0; i < 4; ++i)
		{
			float valueDbfs = ampToDbfs(values[i]);

			float normalizedValue = (valueDbfs - dbfsMin) / (dbfsMax - dbfsMin);
			normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);

			int coveredHeight = static_cast<int>(meters[i].height() * (1.0f - normalizedValue));
			QRect coveredRect(meters[i].left(), meters[i].top(), meters[i].width(), coveredHeight);

			p.fillRect(coveredRect, coverColor);
		}
	};

	drawInverseMeters(inMeters, &m_lastInPeaks[0], QColor(10, 10, 10));
	drawInverseMeters(outMeters, &m_lastOutPeaks[0], QColor(10, 10, 10));

	QRect curveRect1(452, 10, 100, 100);
	QRect curveRect2(452, 131, 100, 100);

	QPen gridPen(QColor(36, 40, 48));
	gridPen.setStyle(Qt::DotLine);
	p.setPen(gridPen);

	auto drawGrid = [&p](const QRect& rect)
	{
		for (int i = 1; i < 8; ++i)
		{
			int x = rect.left() + i * rect.width() / 8 + 1;
			p.drawLine(x, rect.top() + 1, x, rect.bottom());

			int y = rect.top() + i * rect.height() / 8 + 1;
			p.drawLine(rect.left() + 1, y, rect.right(), y);
		}
	};

	drawGrid(curveRect1);
	drawGrid(curveRect2);

	QPen axisPen(QColor(62, 66, 75));
	axisPen.setWidth(2);
	p.setPen(axisPen);

	auto drawAxes = [&p](const QRect& rect)
	{
		p.drawLine(rect.center().x() + 2, rect.top() + 1, rect.center().x() + 2, rect.bottom());
		p.drawLine(rect.left() + 1, rect.center().y() + 2, rect.right(), rect.center().y() + 2);
	};

	drawAxes(curveRect1);
	drawAxes(curveRect2);

	auto drawCurve = [&](const QRect& rect, int band)
	{
		QVector<QPointF> points;

		QPen curvePen(QColor(34, 226, 108));
		curvePen.setWidth(2);
		p.setPen(curvePen);

		const int distType = band == 0 ? m_controls->m_distType1Model.value() : m_controls->m_distType2Model.value();
		const float drive = dbfsToAmp(band == 0 ? m_controls->m_drive1Model.value() : m_controls->m_drive2Model.value());
		const float bias = band == 0 ? m_controls->m_bias1Model.value() : m_controls->m_bias2Model.value();
		const float warp = band == 0 ? m_controls->m_warp1Model.value() : m_controls->m_warp2Model.value();
		const float crush = dbfsToAmp(band == 0 ? m_controls->m_crush1Model.value() : m_controls->m_crush2Model.value());

		const float halfLineWidth = curvePen.widthF() / 2.0f;
		const float amplitudeScale = (rect.height() - curvePen.widthF()) / rect.height();

		const int numSteps = curveRect1.width() * 2;
		for (int i = 0; i <= numSteps; ++i)
		{
			float x = -1.0f + 2.0f * i / numSteps;

			float biasedIn = x * drive + bias;
			float distIn = (biasedIn - copysign(warp / crush, biasedIn)) / (1.0f - warp);
			float distOut;

			switch (static_cast<SlewDistortionType>(distType))
			{
				case SlewDistortionType::HardClip: {
					distOut = std::clamp(distIn, -1.f, 1.f);
					break;
				}
				case SlewDistortionType::Tanh: {
					const float temp = std::clamp(distIn, -40.f, 40.f);
					distOut = 2.f / (1.f + std::exp(-2.f * temp)) - 1;
					break;
				}
				case SlewDistortionType::FastSoftClip1: {
					const float temp = std::clamp(distIn, -2.f, 2.f);
					distOut = temp / (1 + 0.25f * temp * temp);
					break;
				}
				case SlewDistortionType::FastSoftClip2: {
					const float temp = std::clamp(distIn, -1.5f, 1.5f);
					distOut = temp - (4.f / 27.f) * temp * temp * temp;
					break;
				}
				case SlewDistortionType::Sinusoidal: {
					// using a polynomial approximation so it matches with the SSE2 code
					// x - x^3 / 6 + x^5 / 120
					float modInput = std::fmod(distIn - std::numbers::pi_v<float> * 0.5f, 2.f * std::numbers::pi_v<float>);
					if (modInput < 0) {modInput += 2.f * std::numbers::pi_v<float>;}
					const float x = std::abs(modInput - std::numbers::pi_v<float>) - std::numbers::pi_v<float> * 0.5f;
					const float x2 = x * x;
					const float x3 = x2 * x;
					const float x5 = x3 * x2;
					distOut = x - (x3 / 6.0f) + (x5 / 120.0f);
					break;
				}
				case SlewDistortionType::Foldback: {
					distOut = std::abs(std::abs(std::fmod(distIn - 1.f, 4.f)) - 2.f) - 1.f;
					break;
				}
				case SlewDistortionType::FullRectify: {
					distOut = std::abs(distIn);
					break;
				}
				case SlewDistortionType::SmoothRectify:
				{
					distOut = std::sqrt(distIn * distIn + 0.04f) - 0.2f;
					break;
				}
				case SlewDistortionType::HalfRectify:
				{
					distOut = std::max(0.0f, distIn);
					break;
				}
				case SlewDistortionType::Bitcrush:
				{
					const float scale = 16 / drive;
					distOut = std::round(distIn / drive * scale) / scale;
					break;
				}
				default:
				{
					distOut = distIn;
				}
			}

			distOut = distOut * (1.0f - warp) + copysign(warp, biasedIn);
			if (std::abs(biasedIn) < warp / crush)
			{
				distOut = biasedIn * crush;
			}

			distOut *= amplitudeScale;

			float px = rect.left() + (x + 1.f) * 0.5f * rect.width();
			float py = rect.bottom() - (distOut + 1.f) * 0.5f * rect.height();

			py += halfLineWidth;

			points.append(QPointF(px, py));
		}

		QPainterPath path;
		path.addPolygon(QPolygonF(points));
		p.save();
		p.setClipRect(rect);
		p.drawPath(path);
		p.restore();
	};

	drawCurve(curveRect1, 0);
	drawCurve(curveRect2, 1);
}

void SlewDistortionControlDialog::showHelpWindow()
{
	SlewDistortionHelpView::getInstance()->close();
	SlewDistortionHelpView::getInstance()->show();
}


QString SlewDistortionHelpView::s_helpText = tr(
"<div style='text-align: center;'>"
"<b>Slew Distortion</b><br><br>"
"Plugin by Lost Robot<br>"
"GUI by thismoon<br>"
"</div>"
"<h3>Overview:</h3>"
"Slew Distortion is a multiband slew rate limiter and distortion effect.<br><br>"
"Slew rate limiting is something I accidentally invented while trying to make a lowpass filter for the first time.<br>"
"In short, a slew rate limiter limits how quickly the waveform can move from one point to the next.<br>"
"You'll hear that it has a similar quality to a lowpass filter, in that it does quieten the high frequencies by quite a bit.<br>"
"However, the intensity of this effect depends heavily on the input signal, and with it comes a rather unique distortion of that signal.<br><br>"
"In this plugin, the slew rate limiting is followed by waveshaping distortion.<br>"
"Every distortion type is a pure waveshaping function with no filters or delays of any kind involved.<br>"
"These distortions will generate new harmonics at exact frequency multiples of the incoming audio.<br><br>"
"Because the plugin is multiband, you can apply these effects to different frequency ranges independently.<br>"
"<br><h3>Distortion Types:</h3>"
"<b>Hard Clip</b> - Aggressively clamps the audio signal to 0 dBFS.<br>"
"This leaves the signal entirely untouched until it passes the clamping threshold, beyond which all content is clipped out entirely.<br>"
"<b>Tanh</b> - A very gentle sigmoid distortion.<br>"
"This waveshape is mathematically smooth and continuous at all derivatives.<br>"
"It can be pushed significantly harder than most other distortion shapes before it starts generating harsh high frequencies.<br>"
"<b>Fast Soft Clip 1</b> - A CPU-efficient soft clipping function.<br>"
"<b>Fast Soft Clip 2</b> - A CPU-efficient cubic soft clipping function.<br>"
"<b>Sinusoidal</b> - Incredibly smooth wavewrapping distortion.<br>"
"Unlike all the previous distortion types, loud audio information is not entirely lost or clipped away, and is instead wrapped back down to lower values.<br>"
"<b>Foldover</b> - A non-smooth wavewrapping alternative.<br>"
"This leaves the audio values untouched relative to neighboring values,<br>"
"except at the borders where the waveshape sharply changes directions, generating harsh distortion.<br>"
"<b>Full-wave Rectify</b> - Flips the bottom half of the waveform to the top half.<br>"
"The timbre of this commonly sounds similar to shifting the audio upward by one octave.<br>"
"Unlike all the previous distortion types, this one is asymmetrical by default, meaning it will generate even-multiple harmonics.<br>"
"<b>Smooth Rectify</b> - An alternative to Full-wave Rectify which has a smooth corner.<br>"
"<b>Half-wave Rectify</b> - An alternative to Full-wave Rectify which clips all negative audio samples instead of reflecting them upward.<br>"
"<b>Bitcrush</b> - Bit depth reduction. This distortion type is special-cased to have the Drive change its shape instead of its input amplitude.<br>"
"<br><h3>Slew:</h3>"
"This section controls the slew rate limit, the speed at which the incoming waveform's values can change.<br>"
"<b>Up</b> and <b>Down</b> control the slew rate limit for upward and downward movement, respectively.<br>"
"The <b>Slew Link</b> button locks the Slew Up and Slew Down parameters to the same value, for convenience.<br>"
"<br><h3>Dynamics:</h3>"
"This section uses an envelope follower to track the volume of the incoming audio signal.<br>"
"<b>Amount</b> - Restores the dynamic range lost from the distortion and slew rate limiting by matching the output volume to the input volume.<br>"
"<b>Slew</b> - Dynamically changes the slew rate, depending on the input volume.<br>"
"<b>Attack</b> - How quickly the envelope follower responds to increases in volume (e.g. transients).<br>"
"<b>Release</b> - How quickly the envelope follower responds to decreases in volume.<br>"
"<br><h3>Shape:</h3>"
"This section allows further sculpting of the distortion shape beyond what the distortion types can achieve on their own.<br>"
"<b>Warp</b> - Causes input values smaller than this value to be unimpacted by the waveshaping.<br>"
"The distortion shape is properly scaled and shifted to ensure it remains perfectly clean and continuous.<br>"
"<b>Crush</b> - Increases the volume of audio below the Warp value.<br>"
"This adds a sharp corner to the waveshaping function, resulting in much more aggressive distortion.<br>"
"<br><h3>Miscellaneous:</h3>"
"<b>Mix</b> - Blends between the wet and dry signals for the current band.<br>"
"Since both the wet and dry signal are after the crossover filter and have oversampling applied,<br>"
"this parameter is entirely immune to phase issues caused by blending signals.<br>"
"<b>Bias</b> - Adds DC offset to the input signal before the distortion, causing the waveshaping to be asymmetrical.<br>"
"This allows every distortion type to generate even-multiple harmonics, including the symmetrical types which usually only generate odd-multiple harmonics.<br>"
"<b>DC Remover</b> - Removes DC offset (0 Hz audio) from the output signal. You'll almost always want to leave this enabled.<br>"
"<b>Multiband</b> - Splits the signal into two frequency bands. If disabled, the top band's parameters are applied to the entire audio signal.<br>"
"<b>Split</b> - The crossover frequency at which the Multiband mode splits the signal into two bands.<br>"
"<br><h3>Oversampling:</h3>"
"An audio signal is only capable of storing frequencies below Nyquist, which is half of the sample rate.<br>"
"If any form of distortion generates new frequencies that are above this Nyquist frequency, they will be reflected (aliased) back downward.<br>"
"For example, if the distortion generates a harmonic that is 5000 Hz above Nyquist, that frequency will be aliased down to 5000 Hz below Nyquist.<br>"
"This aliasing is inharmonic, oftentimes sounds unpleasant, and can even contribute to auditory masking within the song.<br><br>"
"Oversampling helps to resolve this issue by temporarily increasing the sample rate of the signal,<br>"
"so significantly higher frequencies can be supported before they start aliasing back into the audible range.<br>"
"Those higher frequencies are then filtered out before decreasing the sample rate back to its original value so they don't alias.<br><br>"
"This plugin supports up to five stages of oversampling.<br>"
"Each stage provides an extra 2 octaves of headroom before frequencies alias far enough to become audible.<br>"
"The number on the button is how much the sample rate is increased by. THE PLUGIN'S CPU USAGE WILL BE INCREASED BY APPROXIMATELY THE SAME AMOUNT.<br>"
"Even just 2x oversampling can make a massive difference and is oftentimes all you need, but up to 32x oversampling is supported.<br>"
);



SlewDistortionHelpView::SlewDistortionHelpView() : QTextEdit(s_helpText)
{
#if (QT_VERSION < QT_VERSION_CHECK(5,12,0))
	// Bug workaround: https://codereview.qt-project.org/c/qt/qtbase/+/225348
	using ::operator|;
#endif
	setWindowTitle("Slew Distortion Help");
	setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
	getGUI()->mainWindow()->addWindowedWidget(this);
	parentWidget()->setAttribute(Qt::WA_DeleteOnClose, false);
	parentWidget()->setWindowIcon(PLUGIN_NAME::getIconPixmap("logo"));
	
	// No maximize button
	Qt::WindowFlags flags = parentWidget()->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	parentWidget()->setWindowFlags(flags);
}


} // namespace lmms::gui
