/*
 * GranularPitchShifterControlDialog.cpp
 *
 * Copyright (c) 2024 Lost Robot <r94231/at/gmail/dot/com>
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

#include "embed.h"
#include "ComboBox.h"
#include "GranularPitchShifterControlDialog.h"
#include "GranularPitchShifterControls.h"
#include "Knob.h"
#include "LcdFloatSpinBox.h"
#include "MainWindow.h"
#include "GuiApplication.h"
#include "PixmapButton.h"


namespace lmms::gui
{

GranularPitchShifterControlDialog::GranularPitchShifterControlDialog(GranularPitchShifterControls* controls) :
	EffectControlDialog(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(305, 180);
	
	auto makeKnob = [this](KnobType style, int x, int y, const QString& hintText, const QString& unit, FloatModel* model)
	{
		Knob* newKnob = new Knob(style, this);
		newKnob->move(x, y);
		newKnob->setModel(model);
		newKnob->setHintText(hintText, unit);
		return newKnob;
	};

	makeKnob(KnobType::Bright26, 19, 78, tr("Grain Size:"), " Hz", &controls->m_sizeModel);
	makeKnob(KnobType::Bright26, 116, 10, tr("Spray:"), " seconds", &controls->m_sprayModel);
	makeKnob(KnobType::Bright26, 158, 10, tr("Jitter:"), " octaves", &controls->m_jitterModel);
	makeKnob(KnobType::Bright26, 200, 10, tr("Twitch:"), " octaves", &controls->m_twitchModel);
	makeKnob(KnobType::Bright26, 188, 60, tr("Spray Stereo Spread:"), "", &controls->m_spraySpreadModel);
	makeKnob(KnobType::Bright26, 135, 110, tr("Grain Shape:"), "", &controls->m_shapeModel);
	makeKnob(KnobType::Bright26, 188, 110, tr("Fade Length:"), "", &controls->m_fadeLengthModel);
	makeKnob(KnobType::Bright26, 258, 45, tr("Feedback:"), "", &controls->m_feedbackModel);
	makeKnob(KnobType::Bright26, 258, 92, tr("Minimum Allowed Latency:"), " seconds", &controls->m_minLatencyModel);
	makeKnob(KnobType::Small17, 66, 157, tr("Density:"), "x", &controls->m_densityModel);
	makeKnob(KnobType::Small17, 8, 157, tr("Glide:"), " seconds", &controls->m_glideModel);
	
	LcdFloatSpinBox* pitchBox = new LcdFloatSpinBox(3, 2, "11green", tr("Pitch"), this);
	pitchBox->move(15, 41);
	pitchBox->setModel(&controls->m_pitchModel);
	pitchBox->setToolTip(tr("Pitch"));
	pitchBox->setSeamless(true, true);
	
	LcdFloatSpinBox* pitchSpreadBox = new LcdFloatSpinBox(3, 2, "11green", tr("Pitch Stereo Spread"), this);
	pitchSpreadBox->move(133, 66);
	pitchSpreadBox->setModel(&controls->m_pitchSpreadModel);
	pitchSpreadBox->setToolTip(tr("Pitch Stereo Spread"));
	pitchSpreadBox->setSeamless(true, true);
	
	QPushButton button("Show Help", this);
	connect(&button, &QPushButton::clicked, this, &GranularPitchShifterControlDialog::showHelpWindow);
	
	PixmapButton* m_helpBtn = new PixmapButton(this, nullptr);
	m_helpBtn->move(278, 159);
	m_helpBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("help_active"));
	m_helpBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("help_inactive"));
	m_helpBtn->setToolTip(tr("Open help window"));
	connect(m_helpBtn, SIGNAL(clicked()), this, SLOT(showHelpWindow()));
	
	PixmapButton* prefilterButton = new PixmapButton(this, tr("Prefilter"));
	prefilterButton->move(8, 133);
	prefilterButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("prefilter_active"));
	prefilterButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("prefilter_inactive"));
	prefilterButton->setCheckable(true);
	prefilterButton->setModel(&controls->m_prefilterModel);
	prefilterButton->setToolTip(tr("Prefilter"));
	
	ComboBox* rangeBox = new ComboBox(this);
	rangeBox->setGeometry(189, 155, 80, 22);
	rangeBox->setModel(&controls->m_rangeModel);
	controls->updateRange();
}

void GranularPitchShifterControlDialog::showHelpWindow() {
	GranularPitchShifterHelpView::getInstance()->close();
	GranularPitchShifterHelpView::getInstance()->show();
}


QString GranularPitchShifterHelpView::s_helpText=
"<div style='text-align: center;'>"
"<b>Granular Pitch Shifter</b><br><br>"
"Plugin by Lost Robot<br>"
"GUI by thismoon<br>"
"</div>"
"<h3>Grain:</h3>"
"<b>Pitch</b> - The amount of pitch shifting to perform, in 12EDO semitones.<br>"
"<b>Size</b> - The length of each grain, in Hz.  By default, new grains will be created at double this rate.  <br>In most cases, you'll want this to be set to higher frequencies when shifting the pitch upward, and vice-versa.  <br>"
"<br><h3>Random:</h3>"
"<b>Spray</b> - The amount of randomization for the playback position of each grain, in seconds.  <br>This does not change when the grain plays, but rather what audio the grain is pulling from.  <br>For example, a value of 0.5 seconds will allow each grain to play back audio from up to half of a second ago.<br>It's oftentimes recommended to use at least a small amount of Spray, as this will break up the periodicity in the grains, which is usually the main artifact caused by a granular pitch shifter.  <br>This will also make the grains uncorrelated with each other, guaranteeing that a grain Shape value of 2 will always be optimal.<br>"
"<b>Jitter</b> - The amount of randomization for the pitch of each grain, in octaves.<br>  This does not impact how often grains are created.<br>"
"<b>Twitch</b> - The amount of randomization for how often new grains are created, in octaves.  <br>Jitter and Twitch both use the same random numbers, so if they're at the same value, then the grain creation timings will be changed exactly proportionally to their change in pitch.<br>"
"<br><h3>Stereo:</h3>"
"<b>Pitch</b> - The total distance in pitch between both stereo channels, in 12EDO semitones.<br>  Half of the amount of pitch shifting shown will be applied to the right channel, and the opposite to the left channel.<br>"
"<b>Spray</b> - The allowed distance between each channel's randomized position with the Spray feature in the Random category.  <br>A value of 1 makes the Spray values in each channel entirely unlinked."
"<br><h3>Shape:</h3>"
"<b>Shape</b> - The shape of each grain's fades.  In most cases, 2 is the optimal value, providing equal-power fades.  <br>However, when the plugin is performing minimal pitch shifting and has most of its parameters at default, a value of 1 may be more optimal, providing equal-gain fades.  <br>All fades are designed for 50% grain overlap.<br>"
"<b>Fade</b> - The length of the grain fades.  A value of 1 provides the cleanest fades, causing those fades to reach across the entire grain.  <br>Values below 1 make the fade artifacts more audible, but those fades will only apply to the outer edges of each grain.  <br>A value of 0 will result in clicking sounds due to the fades no longer being present.<br>"
"<br><h3>Delay:</h3>"
"<b>Feedback</b> - The amount of feedback for the pitch shifter.<br>  This feeds a portion of the pitch shifter output back into the input buffer.  Large values can be dangerous.<br>"
"<b>Latency</b> - The minimum amount of latency the pitch shifter will have.<br>  This granular pitch shifter dynamically changes its latency to be at the minimum possible amount depending on your settings.  <br>If you'd like for this latency to be more predictable, you may increase the value of this parameter until the latency no longer changes.  <br>This parameter may also be used to be set the minimum amount of delay for the feedback.<br>A larger latency amount can remove subtle fluttering artifacts that may result from automating the pitch shifting amount at high speeds."
"<br><h3>Miscellaneous:</h3>"
"<b>Prefilter</b> - Enables a 12 dB lowpass filter prior to the pitch shifting which automatically adjusts its cutoff to drastically reduce any resulting aliasing.<br>"
"<b>Density</b> - The multiplier for how often grains are spawned.  <br>This will increase the grain overlap above 50%.  <br>It will create painful piercing sounds if you don't make use of any of the knobs in the Random category.  <br>Otherwise, you can get some interesting effects similar to unison or a stationary Paulstretch.  <br>Note that this knob uses by far the most CPU out of any parameter in this plugin when increased.<br>"
"<b>Glide</b> - The length of interpolation for the amount of pitch shifting.<br>  A small amount of glide is very effective for cleaning up many of the artifacts that may result from changing the pitch shift amount over time.  <br>"
"<b>Range</b> - The length of the pitch shifter's internal ring buffer.<br>  Changing this will change the minimum and maximum values for some of the other parameters, which are listed in each of the options.<br>  Increase it if you need parameter values that aren't supported with the minimum buffer length.  Otherwise, it's best to leave it at its minimum value.<br>"
;

GranularPitchShifterHelpView::GranularPitchShifterHelpView():QTextEdit(s_helpText)
{
#if (QT_VERSION < QT_VERSION_CHECK(5,12,0))
	// Bug workaround: https://codereview.qt-project.org/c/qt/qtbase/+/225348
	using ::operator|;
#endif
	setWindowTitle("Granular Pitch Shifter Help");
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
