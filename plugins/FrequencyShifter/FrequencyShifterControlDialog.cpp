/*
 * FrequencyShifterControlDialog.cpp
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "FrequencyShifterControlDialog.h"
#include "FrequencyShifterControls.h"

#include <QTextEdit>

#include "AutomatableButton.h"
#include "embed.h"
#include "GuiApplication.h"
#include "Knob.h"
#include "LcdFloatSpinBox.h"
#include "MainWindow.h"
#include "PixmapButton.h"

namespace lmms::gui
{

static inline void setupKnobGeometry(Knob* k, int w, int h)
{
	k->setFixedSize(w, h);

	const int cx = w / 2;
	const int cy = h / 2;
	k->setCenterPointX(cx);
	k->setCenterPointY(cy);

	int outer = std::max(1, cx - 3);
	int inner = std::max(1, outer - ((w >= 40) ? 16 : (w >= 24) ? 10 : 6));

	if (w <= 16)
	{
		outer = cx - 2;
		inner = 2;
	}

	k->setOuterRadius(outer);
	k->setInnerRadius(inner);
}

FrequencyShifterControlDialog::FrequencyShifterControlDialog(FrequencyShifterControls* c) :
	EffectControlDialog(c)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(288, 360);

	auto mk = [this](int x, int y,
		const QString& lbl, FloatModel* m, const QString& unit,
		const char* objName,
		QSize sz)
	{
		Knob* k = new Knob(KnobType::Styled, this);
		k->setObjectName(objName);
		k->move(x, y);
		k->setModel(m);
		k->setHintText(lbl, unit);
		setupKnobGeometry(k, sz.width(), sz.height());
		return k;
	};

	const QSize K60(60, 60);
	const QSize K36(36, 36);
	const QSize K24(24, 24);
	const QSize K19(19, 19);

	LcdFloatSpinBox* shiftSpin = new LcdFloatSpinBox(6, 3, "19red", tr("Frequency Shift"), this);
	shiftSpin->move(100, 43);
	shiftSpin->setModel(&c->m_freqShift);
	shiftSpin->setSeamless(true, true);

	mk(18, 30, "Mix", &c->m_mix, "", "fs_mix", K60);
	mk(235, 24, "Spread", &c->m_spreadShift,"Hz", "fs_spread", K24);
	mk(235, 72, "Phase",&c->m_phase, "", "fs_phase", K24);
	mk(24, 115, "Ring", &c->m_ring, "", "fs_ring", K36);
	mk(72, 115, "Harmonics", &c->m_harmonics, "", "fs_harm", K36);
	mk(120, 115, "Tone",&c->m_tone, "Hz", "fs_tone", K36);
	mk(200, 147, "Glide", &c->m_glide, "", "fs_glide", K19);

	mk(18, 200, "LFO", &c->m_lfoAmount, "Hz", "fs_lfo", K36);
	mk(66, 200, "LFO Rate", &c->m_lfoRate, "Hz", "fs_lforate", K36);
	mk(114, 200, "LFO Stereo Phase", &c->m_lfoStereoPhase, "", "fs_lfost", K36);

	mk(18, 282, "Delay Length", &c->m_delayLengthLong, "ms", "fs_delay", K36);
	mk(114, 282, "Feedback", &c->m_feedback, "", "fs_feedback", K36);
	mk(24, 324, "Delay Length (fine)", &c->m_delayLengthShort, "ms", "fs_finedelay", K24);
	mk(120, 324, "Delay Damping", &c->m_delayDamp, "Hz", "fs_damp", K24);
	mk(245, 315, "Delay Glide", &c->m_delayGlide, "", "fs_dglide", K19);

	PixmapButton* antireflectButton = new PixmapButton(this, "Antireflect");
	antireflectButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("antireflect_on"));
	antireflectButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("antireflect_off"));
	antireflectButton->setToolTip("Anti-reflect");
	antireflectButton->move(188, 122);
	antireflectButton->setCheckable(true);
	antireflectButton->setModel(&c->m_antireflect);

	PixmapButton* routeSend = new PixmapButton(this, tr("Send"));
	routeSend->setActiveGraphic(PLUGIN_NAME::getIconPixmap("send_on"));
	routeSend->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("send_off"));
	routeSend->setToolTip(tr("Route: Send"));
	routeSend->setCheckable(true);
	routeSend->move(188, 199);

	PixmapButton* routePass = new PixmapButton(this, tr("Pass"));
	routePass->setActiveGraphic(PLUGIN_NAME::getIconPixmap("pass_on"));
	routePass->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("pass_off"));
	routePass->setToolTip(tr("Route: Pass"));
	routePass->setCheckable(true);
	routePass->move(188, 217);

	PixmapButton* routeMute = new PixmapButton(this, tr("Mute"));
	routeMute->setActiveGraphic(PLUGIN_NAME::getIconPixmap("mute_on"));
	routeMute->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("mute_off"));
	routeMute->setToolTip(tr("Route: Mute"));
	routeMute->setCheckable(true);
	routeMute->move(188, 235);

	AutomatableButtonGroup* routeGroup = new AutomatableButtonGroup(this);
	routeGroup->addButton(routeSend);
	routeGroup->addButton(routePass);
	routeGroup->addButton(routeMute);
	routeGroup->setModel(&c->m_routeMode);

	PixmapButton* resetShifterBtn = new PixmapButton(this, tr("Reset Shifter"));
	resetShifterBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("reset_shifter_on"));
	resetShifterBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("reset_shifter_off"));
	resetShifterBtn->setToolTip(tr("Reset the shifter's oscillator phases to 0 (automatable)"));
	resetShifterBtn->setCheckable(false);
	resetShifterBtn->move(77, 5);
	resetShifterBtn->setModel(&c->m_resetShifter);

	PixmapButton* resetLfoBtn = new PixmapButton(this, tr("Reset LFO"));
	resetLfoBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("reset_lfo_on"));
	resetLfoBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("reset_lfo_off"));
	resetLfoBtn->setToolTip(tr("Reset the LFO phase to 0 (automatable)"));
	resetLfoBtn->setCheckable(false);
	resetLfoBtn->move(60, 179);
	resetLfoBtn->setModel(&c->m_resetLfo);

	PixmapButton* helpBtn = new PixmapButton(this, nullptr);
	helpBtn->move(256, 278);
	helpBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("help_on"));
	helpBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("help_off"));
	helpBtn->setToolTip(tr("Open help window"));
	connect(helpBtn, &PixmapButton::clicked, this, &FrequencyShifterControlDialog::showHelpWindow);
}

void FrequencyShifterControlDialog::showHelpWindow()
{
	FrequencyShifterHelpView::getInstance()->close();
	FrequencyShifterHelpView::getInstance()->show();
}

QString FrequencyShifterHelpView::s_helpText = tr(
"<div style='text-align: center;'>"
"<b>Frequency Shifter</b><br><br>"
"Plugin by Lost Robot<br>"
"GUI by Haeleon<br>"
"</div>"
"<h3>Overview:</h3>"
"Frequency Shifter is <b>not</b> a pitch shifter.<br><br>"
"While &quot;frequency&quot; refers to Hz, &quot;pitch&quot; refers to octaves, semitones, cents, etc. <br>"
"So, pitch shifting impacts all partials in the audio multiplicatively, while frequency shifting impacts it additively.<br>"
"For example: If you have frequencies 100, 200, and 300 Hz, a pitch shift upward by 1.2x would result in 120, 240, and 360 Hz. "
"Meanwhile, a frequency shift upward by 20 Hz would result in 120, 220, and 320 Hz.<br>"
"Notice that a pitch shifter preserves the harmonic relationships between these frequencies, while frequency shifting destroys them entirely, "
"resulting in an inharmonic timbre.<br><br>"
"A frequency shifter can also be used as a &quot;barberpole phaser&quot;. This is similar to other phasers, but unlike those, "
"it can audibly move upward or downward infinitely, similar to a Shepard tone.<br>"
"To achieve this, simply set the frequency shift amount to your desired phaser rate, and set the Mix to 50%. "
"The resulting phase cancellation will filter the audio.<br>"
"You may also achieve this by simply increasing the delay feedback, and keeping the delay length very low.<br><br>"
"This frequency shifter sports a unique &quot;anti-reflect&quot; algorithm which eliminates all frequencies aliasing through Nyquist and 0 Hz.<br><br>"
"This plugin may also be used as a ring modulator via the RING parameter. "
"Ring modulation is the result of frequency shifting the audio upward and downward by the same amount in parallel.<br>"
"<br><h3>Shifter:</h3>"
"<b>Mix</b> - Blends between the wet and dry signals.<br>"
"<b>Frequency Shift</b> - The amount of frequency shifting, in Hz.<br>"
"<b>Spread</b> - Offsets the frequency shift amount in opposite directions for the left and right channels.<br>"
"Even very small amounts will add a lot of stereo width to the signal.<br>"
"<b>Phase</b> - Gives you manual control over the phase of the frequency shifter's internal oscillators.<br>"
"When using the frequency shifter as a barberpole phaser, it is recommended to set the frequency shift amount to 0 and "
"automate this Phase parameter.<br>"
"<b>Ring</b> - Blends in ring modulation, instead of just frequency shifting.<br>"
"<b>Harm</b> - Distorts the frequency shifter's internal sine oscillators. This brings them much closer to a smoothed square shape.<br>"
"<b>Tone</b> - A basic 1-pole lowpass on the frequency shifter's output, helpful for taming harsh high frequencies.<br>"
"<b>Glide</b> - Lowpass filters any frequency shift and phase parameter movements, so they move slowly over time rather than snapping "
"to their target value instantly.<br>"
"<b>Reset</b> - Instantly resets the phases of the frequency shifter's internal oscillators. This is automatable.<br>"
"<b>Anti-reflect</b> - Magic.<br>"
"It removes all aliased frequencies through Nyquist and through 0 Hz. "
"This is done via clean and CPU-efficient math tricks, not oversampling.<br>"
"<br><h3>LFO:</h3>"
"This modulates the frequency shift amount. Audio-rate modulation is fully supported.<br><br>"
"<b>Amount</b> - The amplitude of the LFO.<br>"
"<b>Rate</b> - LFO rate, in Hz.<br>"
"<b>Stereo Phase</b> - Offsets the phase of the LFO's right channel, making things stereo.<br>"
"<b>Reset</b> - Instantly resets the phases of the LFO's oscillators. This is automatable.<br>"
"<br><h3>Routing:</h3>"
"<b>Send</b> - Sends the frequency shifter output into the delay.<br>"
"<b>Pass</b> - The audio input bypasses the frequency shifter, and is sent to both the delay and the output. "
"The frequency shifter is now located inside of the delay line. Use this if you want the frequency shifter to only impact the echoes.<br>"
"<b>Mute</b> - Like &quot;Pass&quot; routing, except the input signal isn't sent to the output, "
"so all you hear is the output from the delay line.<br>"
"<br><h3>Delay:</h3>"
"<b>Length</b> - Delay time in milliseconds.<br>"
"<b>Fine</b> - Identical to delay Length, but with a smaller knob range. "
"This is helpful when using the feedback to cause comb filtering, giving you access to a unique phaser/flanger hybrid.<br>"
"<b>Feedback</b> - Feeds the output of the delay back into the input of the frequency shifter.<br>"
"The delay's feedback path has very gentle saturation at high amplitudes, so the plugin can't break from high feedback values.<br>"
"<b>Damping</b> - A 1-pole lowpass filter in the feedback loop, so high frequencies fade out sooner than low frequencies.<br>"
"<b>Glide</b> - Lowpass filters any delay length changes, so they move slowly over time rather than snapping to their target value instantly.<br>"
"<b>Help</b> - Instantly spawns a kiwano in a randomized location on the planet. 30 second cooldown.<br>"
);

FrequencyShifterHelpView::FrequencyShifterHelpView() :
	QTextEdit(s_helpText)
{
#if (QT_VERSION < QT_VERSION_CHECK(5,12,0))
	// Bug workaround: https://codereview.qt-project.org/c/qt/qtbase/+/225348
	using ::operator|;
#endif
	setWindowTitle(tr("Frequency Shifter Help"));
	setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
	getGUI()->mainWindow()->addWindowedWidget(this);
	parentWidget()->setAttribute(Qt::WA_DeleteOnClose, false);

	// No maximize button
	Qt::WindowFlags flags = parentWidget()->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	parentWidget()->setWindowFlags(flags);
}

} // namespace lmms::gui

