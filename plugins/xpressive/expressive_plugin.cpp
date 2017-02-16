/*
 * expressive_plugin.cpp - instrument which uses a mathematical formula parser
 *
 * Copyright (c) 2016-2017 Orr Dvori
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

#include "expressive_plugin.h"

#include <QDomElement>

#include "Engine.h"
#include "Graph.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "LedCheckbox.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "NotePlayHandle.h"
#include "Oscillator.h"
#include "PixmapButton.h"
#include "Song.h"
#include "SubWindow.h"
#include "ToolTip.h"

#include "base64.h"
#include "lmms_constants.h"
#include "templates.h"

#include "embed.cpp"

#include "exprfront.h"

extern "C" {

Plugin::Descriptor PLUGIN_EXPORT xpressive_plugin_descriptor = { STRINGIFY(
		PLUGIN_NAME), "X-Pressive", QT_TRANSLATE_NOOP("pluginBrowser",
		"Mathematical expression parser"), "Orr Dvori", 0x0100,
		Plugin::Instrument, new PluginPixmapLoader("logo"), NULL, NULL };

}

ExprSynth::ExprSynth(const WaveSample *gW1, const WaveSample *gW2, const WaveSample *gW3,
	ExprFront *exprO1, ExprFront *exprO2,
	NotePlayHandle *nph, const sample_rate_t sample_rate,
	const FloatModel* pan1, const FloatModel* pan2, float rel_trans):
	m_exprO1(exprO1),
	m_exprO2(exprO2),
	m_W1(gW1),
	m_W2(gW2),
	m_W3(gW3),
	m_nph(nph),
	m_sample_rate(sample_rate),
	m_pan1(pan1),
	m_pan2(pan2),
	m_rel_transition(rel_trans)
{
	m_note_sample = 0;
	m_note_rel_sample = 0;
	m_note_rel_sec = 0;
	m_note_sample_sec = 0;
	m_released = 0;
	m_frequency = m_nph->frequency();
	m_rel_inc = 1000.0 / (m_sample_rate * m_rel_transition);//rel_transition in ms. compute how much increment in each frame

	auto init_expression_step2 = [this](ExprFront * e) {
		e->add_cyclic_vector("W1", m_W1->m_samples,m_W1->m_length, m_W1->m_interpolate);
		e->add_cyclic_vector("W2", m_W2->m_samples,m_W2->m_length, m_W2->m_interpolate);
		e->add_cyclic_vector("W3", m_W3->m_samples,m_W3->m_length, m_W3->m_interpolate);
		e->add_variable("t", m_note_sample_sec);
		e->add_variable("f", m_frequency);
		e->add_variable("rel",m_released);
		e->add_variable("trel",m_note_rel_sec);
		e->setIntegrate(&m_note_sample,m_sample_rate);
		e->compile();
	};
	init_expression_step2(m_exprO1);
	init_expression_step2(m_exprO2);

}

ExprSynth::~ExprSynth()
{
	if (m_exprO1)
	{
		delete m_exprO1;
	}
	if (m_exprO2)
	{
		delete m_exprO2;
	}
}

void ExprSynth::renderOutput(fpp_t frames, sampleFrame *buf)
{

	float o1,o2;
	const float pn1=m_pan1->value()*0.5;
	const float pn2=m_pan2->value()*0.5;
	const float new_freq=m_nph->frequency();
	const float freq_inc=(new_freq-m_frequency)/frames;
	const bool is_released = m_nph->isReleased();
	if (is_released && m_note_rel_sample == 0)
	{
		m_note_rel_sample = m_note_sample;
	}
	for (fpp_t frame = 0; frame < frames ; ++frame)
	{
		if (is_released && m_released < 1)
			m_released=fmin(m_released+m_rel_inc,1);
		o1=m_exprO1->evaluate();
		o2=m_exprO2->evaluate();
		buf[frame][0] = (-pn1 + 0.5)*o1+(-pn2 + 0.5)*o2;
		buf[frame][1] = ( pn1 + 0.5)*o1+( pn2 + 0.5)*o2;
		m_note_sample++;
		m_note_sample_sec = m_note_sample/(float)m_sample_rate;
		if (is_released)
			m_note_rel_sec = (m_note_sample-m_note_rel_sample)/(float)m_sample_rate;
		m_frequency += freq_inc;
	}
	m_frequency = new_freq;
}
/*
 * nice test:
O1 -> trianglew(2t*f)*(0.5+0.5sinew(12*A1*t+0.5))+sinew(t*f)*(0.5+0.5sinew(12*A1*t))
O2 -> trianglew(2t*f)*(0.5+0.5sinew(12*A1*t))+sinew(t*f)*(0.5+0.5sinew(12*A1*t+0.5))
*/


/***********************************************************************
 *
 *	class Expressive
 *
 *	lmms - plugin
 *
 ***********************************************************************/
#define GRAPH_LENGTH 4096

Expressive::Expressive(InstrumentTrack* instrument_track) :
	Instrument(instrument_track, &xpressive_plugin_descriptor),
	m_graphO1(-1.0f, 1.0f, 360, this),
	m_graphO2(-1.0f, 1.0f, 360, this),
	m_graphW1(-1.0f, 1.0f, GRAPH_LENGTH, this),
	m_graphW2(-1.0f, 1.0f, GRAPH_LENGTH, this),
	m_graphW3(-1.0f, 1.0f, GRAPH_LENGTH, this),
	m_rawgraphW1(-1.0f, 1.0f, GRAPH_LENGTH, this),
	m_rawgraphW2(-1.0f, 1.0f, GRAPH_LENGTH, this),
	m_rawgraphW3(-1.0f, 1.0f, GRAPH_LENGTH, this),
	m_selectedGraph(0, 0, 6, this, tr("Selected graph")),
	m_parameterA1(1, -1.0f, 1.0f, 0.01f, this, tr("A1")),
	m_parameterA2(1, -1.0f, 1.0f, 0.01f, this, tr("A2")),
	m_parameterA3(1, -1.0f, 1.0f, 0.01f, this, tr("A3")),
	m_smoothW1(0, 0.0f, 70.0f, 1.0f, this, tr("W1 smoothing")),
	m_smoothW2(0, 0.0f, 70.0f, 1.0f, this, tr("W2 smoothing")),
	m_smoothW3(0, 0.0f, 70.0f, 1.0f, this, tr("W3 smoothing")),
	m_interpolateW1(false, this),
	m_interpolateW2(false, this),
	m_interpolateW3(false, this),
	m_panning1( 1, -1.0f, 1.0f, 0.01f, this, tr("PAN1")),
	m_panning2(-1, -1.0f, 1.0f, 0.01f, this, tr("PAN2")),
	m_relTransition(50.0f, 0.0f, 500.0f, 1.0f, this, tr("REL TRANS")),
	m_W1(GRAPH_LENGTH),
	m_W2(GRAPH_LENGTH),
	m_W3(GRAPH_LENGTH),
	m_exprValid(false, this)
{
	m_outputExpression[0]="sinew(integrate(f*(1+0.05sinew(12t))))*(2^(-(1.1+A2)*t)*(0.4+0.1(1+A3)+0.4sinew((2.5+2A1)t))^2)";
	m_outputExpression[1]="expw(integrate(f*atan(500t)*2/pi))*0.5+0.12";
}

Expressive::~Expressive() {
}

void Expressive::saveSettings(QDomDocument & _doc, QDomElement & _this) {

	// Save plugin version
	_this.setAttribute("version", "0.1");
	_this.setAttribute("O1", QString(m_outputExpression[0]));
	_this.setAttribute("O2", QString(m_outputExpression[1]));
	_this.setAttribute("W1", QString(m_wavesExpression[0]));
	// Save sample shape base64-encoded
	QString sampleString;
	base64::encode( (const char*)m_rawgraphW1.samples(),
		 m_rawgraphW1.length() * sizeof(float), sampleString );
	_this.setAttribute( "W1sample", sampleString );

	_this.setAttribute("W2", QString(m_wavesExpression[1]));
	base64::encode( (const char*)m_rawgraphW2.samples(),
		 m_rawgraphW2.length() * sizeof(float), sampleString );
	_this.setAttribute( "W2sample", sampleString );
	_this.setAttribute("W3", QString(m_wavesExpression[2]));
	base64::encode( (const char*)m_rawgraphW3.samples(),
		 m_rawgraphW3.length() * sizeof(float), sampleString );
	_this.setAttribute( "W3sample", sampleString );
	m_smoothW1.saveSettings(_doc,_this,"smoothW1");
	m_smoothW2.saveSettings(_doc,_this,"smoothW2");
	m_smoothW3.saveSettings(_doc,_this,"smoothW3");
	m_interpolateW1.saveSettings(_doc,_this,"interpolateW1");
	m_interpolateW2.saveSettings(_doc,_this,"interpolateW2");
	m_interpolateW3.saveSettings(_doc,_this,"interpolateW3");
	m_parameterA1.saveSettings(_doc,_this,"A1");
	m_parameterA2.saveSettings(_doc,_this,"A2");
	m_parameterA3.saveSettings(_doc,_this,"A3");
	m_panning1.saveSettings(_doc,_this,"PAN1");
	m_panning2.saveSettings(_doc,_this,"PAN2");
	m_relTransition.saveSettings(_doc,_this,"RELTRANS");

}

void Expressive::loadSettings(const QDomElement & _this) {

	m_outputExpression[0]=_this.attribute( "O1").toLatin1();
	m_outputExpression[1]=_this.attribute( "O2").toLatin1();
	m_wavesExpression[0]=_this.attribute( "W1").toLatin1();
	m_wavesExpression[1]=_this.attribute( "W2").toLatin1();
	m_wavesExpression[2]=_this.attribute( "W3").toLatin1();

	m_smoothW1.loadSettings(_this,"smoothW1");
	m_smoothW2.loadSettings(_this,"smoothW2");
	m_smoothW3.loadSettings(_this,"smoothW3");
	m_interpolateW1.loadSettings(_this,"interpolateW1");
	m_interpolateW2.loadSettings(_this,"interpolateW2");
	m_interpolateW3.loadSettings(_this,"interpolateW3");
	m_parameterA1.loadSettings(_this,"A1");
	m_parameterA2.loadSettings(_this,"A2");
	m_parameterA3.loadSettings(_this,"A3");
	m_panning1.loadSettings(_this,"PAN1");
	m_panning2.loadSettings(_this,"PAN2");
	m_relTransition.loadSettings(_this,"RELTRANS");

	int size = 0;
	char * dst = 0;
	base64::decode( _this.attribute( "W1sample"), &dst, &size );

	m_rawgraphW1.setSamples( (float*) dst );
	delete[] dst;
	base64::decode( _this.attribute( "W2sample"), &dst, &size );

	m_rawgraphW2.setSamples( (float*) dst );
	delete[] dst;
	base64::decode( _this.attribute( "W3sample"), &dst, &size );

	m_rawgraphW3.setSamples( (float*) dst );
	delete[] dst;

	smooth(m_smoothW1.value(),&m_rawgraphW1,&m_graphW1);
	smooth(m_smoothW2.value(),&m_rawgraphW2,&m_graphW2);
	smooth(m_smoothW3.value(),&m_rawgraphW3,&m_graphW3);
	m_W1.copyFrom(&m_graphW1);
	m_W2.copyFrom(&m_graphW2);
	m_W3.copyFrom(&m_graphW3);
}


QString Expressive::nodeName() const {
	return (xpressive_plugin_descriptor.name);
}

void Expressive::playNote(NotePlayHandle* nph, sampleFrame* working_buffer) {
	m_A1=m_parameterA1.value();
	m_A2=m_parameterA2.value();
	m_A3=m_parameterA3.value();

	if (nph->totalFramesPlayed() == 0 || nph->m_pluginData == NULL) {

		ExprFront *exprO1 = new ExprFront(m_outputExpression[0].constData());
		ExprFront *exprO2 = new ExprFront(m_outputExpression[1].constData());

		auto init_expression_step1 = [this, nph](ExprFront* e) {
			e->add_constant("key", nph->key());
			e->add_constant("bnote", nph->instrumentTrack()->baseNote());
			e->add_constant("srate", Engine::mixer()->processingSampleRate());
			e->add_constant("v", nph->getVolume() / 255.0);
			e->add_variable("A1", m_A1);
			e->add_variable("A2", m_A2);
			e->add_variable("A3", m_A3);
		};
		init_expression_step1(exprO1);
		init_expression_step1(exprO2);

		m_W1.setInterpolate(m_interpolateW1.value());
		m_W2.setInterpolate(m_interpolateW2.value());
		m_W3.setInterpolate(m_interpolateW3.value());
		nph->m_pluginData = new ExprSynth(&m_W1, &m_W2, &m_W3, exprO1, exprO2, nph,
				Engine::mixer()->processingSampleRate(), &m_panning1, &m_panning2, m_relTransition.value());
	}




	ExprSynth *ps = static_cast<ExprSynth*>(nph->m_pluginData);
	const fpp_t frames = nph->framesLeftForCurrentPeriod();
	const f_cnt_t offset = nph->noteOffset();

	ps->renderOutput(frames, working_buffer + offset);

	instrumentTrack()->processAudioBuffer(working_buffer, frames + offset, nph);
}

void Expressive::deleteNotePluginData(NotePlayHandle* nph) {
	delete static_cast<ExprSynth *>(nph->m_pluginData);
}

PluginView * Expressive::instantiateView(QWidget* parent) {
	return (new expressiveView(this, parent));
}

class expressiveKnob: public Knob {
public:
	void setStyle()
	{
		setFixedSize(29, 29);
		setCenterPointX(14.5);
		setCenterPointY(14.5);
		setInnerRadius(4);
		setOuterRadius(9);
		setOuterColor(QColor(0x519fff));
		setTotalAngle(300.0);
		setLineWidth(3);
	}
	expressiveKnob(QWidget * _parent, const QString & _name) :
			Knob(knobStyled, _parent,_name) {
		setStyle();
	}
	expressiveKnob(QWidget * _parent) :
			Knob(knobStyled, _parent) {
		setStyle();
	}

};


expressiveView::expressiveView(Instrument * _instrument, QWidget * _parent) :
	InstrumentView(_instrument, _parent)

{
	const int COL_KNOBS = 194;
	const int ROW_KNOBSA1 = 26;
	const int ROW_KNOBSA2 = 26 + 32;
	const int ROW_KNOBSA3 = 26 + 64;
	const int ROW_KNOBSP1 = 126;
	const int ROW_KNOBSP2 = 126 + 32;
	const int ROW_KNOBREL = 126 + 64;
	const int ROW_WAVEBTN = 234;

	setAutoFillBackground(true);
	QPalette pal;

	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);

	m_graph = new Graph(this, Graph::LinearStyle, 180, 81);
	m_graph->move(9, 27);
	m_graph->setAutoFillBackground(true);
	m_graph->setGraphColor(QColor(255, 255, 255));
	m_graph->setEnabled(false);

	ToolTip::add(m_graph, tr("Draw your own waveform here "
			"by dragging your mouse on this graph."));

	pal = QPalette();
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("wavegraph"));
	m_graph->setPalette(pal);

	PixmapButton * m_w1Btn;
	PixmapButton * m_w2Btn;
	PixmapButton * m_w3Btn;
	PixmapButton * m_o1Btn;
	PixmapButton * m_o2Btn;
	PixmapButton * m_helpBtn;

	m_w1Btn = new PixmapButton(this, NULL);
	m_w1Btn->move(9, 111);
	m_w1Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("w1_active"));
	m_w1Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("w1_inactive"));
	ToolTip::add(m_w1Btn, tr("Select oscillator W1"));

	m_w2Btn = new PixmapButton(this, NULL);
	m_w2Btn->move(32, 111);
	m_w2Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("w2_active"));
	m_w2Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("w2_inactive"));
	ToolTip::add(m_w2Btn, tr("Select oscillator W2"));

	m_w3Btn = new PixmapButton(this, NULL);
	m_w3Btn->move(55, 111);
	m_w3Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("w3_active"));
	m_w3Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("w3_inactive"));
	ToolTip::add(m_w3Btn, tr("Select oscillator W3"));

	m_o1Btn = new PixmapButton(this, NULL);
	m_o1Btn->move(85, 111);
	m_o1Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("o1_active"));
	m_o1Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("o1_inactive"));
	ToolTip::add(m_o1Btn, tr("Select OUTPUT 1"));

	m_o2Btn = new PixmapButton(this, NULL);
	m_o2Btn->move(107, 111);
	m_o2Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("o2_active"));
	m_o2Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("o2_inactive"));
	ToolTip::add(m_o2Btn, tr("Select OUTPUT 2"));

	m_helpBtn = new PixmapButton(this, NULL);
	m_helpBtn->move(139, 111);
	m_helpBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("help_active"));
	m_helpBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("help_inactive"));
	ToolTip::add(m_helpBtn, tr("Open help window"));

	m_selectedGraphGroup = new automatableButtonGroup(this);
	m_selectedGraphGroup->addButton(m_w1Btn);
	m_selectedGraphGroup->addButton(m_w2Btn);
	m_selectedGraphGroup->addButton(m_w3Btn);
	m_selectedGraphGroup->addButton(m_o1Btn);
	m_selectedGraphGroup->addButton(m_o2Btn);

	Expressive *e = castModel<Expressive>();
	m_selectedGraphGroup->setModel(&e->m_selectedGraph);

	m_sinWaveBtn = new PixmapButton(this, tr("Sine wave"));
	m_sinWaveBtn->move(10, ROW_WAVEBTN);
	m_sinWaveBtn->setActiveGraphic(embed::getIconPixmap("sin_wave_active"));
	m_sinWaveBtn->setInactiveGraphic(embed::getIconPixmap("sin_wave_inactive"));
	ToolTip::add(m_sinWaveBtn, tr("Click for a sine-wave."));

	m_moogWaveBtn = new PixmapButton(this, tr("Moog-Saw wave"));
	m_moogWaveBtn->move(10, ROW_WAVEBTN-14);
	m_moogWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "moog_saw_wave_active" ) );
	m_moogWaveBtn->setInactiveGraphic(embed::getIconPixmap("moog_saw_wave_inactive"));
	ToolTip::add(m_moogWaveBtn, tr("Click for a Moog-Saw-wave."));

	m_expWaveBtn = new PixmapButton(this, tr("Exponential wave"));
	m_expWaveBtn->move(10 +14, ROW_WAVEBTN-14);
	m_expWaveBtn->setActiveGraphic(embed::getIconPixmap( "exp_wave_active" ) );
	m_expWaveBtn->setInactiveGraphic(embed::getIconPixmap( "exp_wave_inactive" ) );
	ToolTip::add(m_expWaveBtn, tr("Click for an exponential wave."));

	m_sawWaveBtn = new PixmapButton(this, tr("Saw wave"));
	m_sawWaveBtn->move(10 + 14 * 2, ROW_WAVEBTN-14);
	m_sawWaveBtn->setActiveGraphic(embed::getIconPixmap("saw_wave_active"));
	m_sawWaveBtn->setInactiveGraphic(embed::getIconPixmap("saw_wave_inactive"));
	ToolTip::add(m_sawWaveBtn, tr("Click here for a saw-wave."));

	m_usrWaveBtn = new PixmapButton(this, tr("User defined wave"));
	m_usrWaveBtn->move(10 + 14 * 3, ROW_WAVEBTN-14);
	m_usrWaveBtn->setActiveGraphic(embed::getIconPixmap("usr_wave_active"));
	m_usrWaveBtn->setInactiveGraphic(embed::getIconPixmap("usr_wave_inactive"));
	ToolTip::add(m_usrWaveBtn, tr("Click here for a user-defined shape."));

	m_triangleWaveBtn = new PixmapButton(this, tr("Triangle wave"));
	m_triangleWaveBtn->move(10 + 14, ROW_WAVEBTN);
	m_triangleWaveBtn->setActiveGraphic(
			embed::getIconPixmap("triangle_wave_active"));
	m_triangleWaveBtn->setInactiveGraphic(
			embed::getIconPixmap("triangle_wave_inactive"));
	ToolTip::add(m_triangleWaveBtn, tr("Click here for a triangle-wave."));

	m_sqrWaveBtn = new PixmapButton(this, tr("Square wave"));
	m_sqrWaveBtn->move(10 + 14 * 2, ROW_WAVEBTN);
	m_sqrWaveBtn->setActiveGraphic(embed::getIconPixmap("square_wave_active"));
	m_sqrWaveBtn->setInactiveGraphic(
			embed::getIconPixmap("square_wave_inactive"));
	ToolTip::add(m_sqrWaveBtn, tr("Click here for a square-wave."));

	m_whiteNoiseWaveBtn = new PixmapButton(this, tr("White noise wave"));
	m_whiteNoiseWaveBtn->move(10 + 14 * 3, ROW_WAVEBTN);
	m_whiteNoiseWaveBtn->setActiveGraphic(
			embed::getIconPixmap("white_noise_wave_active"));
	m_whiteNoiseWaveBtn->setInactiveGraphic(
			embed::getIconPixmap("white_noise_wave_inactive"));
	ToolTip::add(m_whiteNoiseWaveBtn, tr("Click here for white-noise."));


	m_waveInterpolate  = new LedCheckBox("Interpolate", this, tr("WaveInterpolate"),
										 LedCheckBox::Green);
	m_waveInterpolate->move(120, 230);

	m_expressionValidToggle = new LedCheckBox("", this, tr("ExpressionValid"),
			LedCheckBox::Red);
	m_expressionValidToggle->move(174, 216);
	m_expressionValidToggle->setEnabled( false );

	m_expressionEditor = new QPlainTextEdit(this);
	m_expressionEditor->move(9, 128);
	m_expressionEditor->resize(180, 90);

	m_generalPurposeKnob[0] = new expressiveKnob(this,"A1");
	m_generalPurposeKnob[0]->setHintText(tr("General purpose 1:"), "");
	m_generalPurposeKnob[0]->move(COL_KNOBS, ROW_KNOBSA1);

	m_generalPurposeKnob[1] = new expressiveKnob(this,"A2");
	m_generalPurposeKnob[1]->setHintText(tr("General purpose 2:"), "");
	m_generalPurposeKnob[1]->move(COL_KNOBS, ROW_KNOBSA2);

	m_generalPurposeKnob[2] = new expressiveKnob(this,"A3");
	m_generalPurposeKnob[2]->setHintText(tr("General purpose 3:"), "");
	m_generalPurposeKnob[2]->move(COL_KNOBS, ROW_KNOBSA3);

	m_panningKnob[0] = new expressiveKnob(this,"O1 panning");
	m_panningKnob[0]->setHintText(tr("O1 panning:"), "");
	m_panningKnob[0]->move(COL_KNOBS, ROW_KNOBSP1);

	m_panningKnob[1] = new expressiveKnob(this,"O2 panning");
	m_panningKnob[1]->setHintText(tr("O2 panning:"), "");
	m_panningKnob[1]->move(COL_KNOBS, ROW_KNOBSP2);

	m_relKnob = new expressiveKnob(this,"Release transition");
	m_relKnob->setHintText(tr("Release transition:"), "ms");
	m_relKnob->move(COL_KNOBS, ROW_KNOBREL);



	m_smoothKnob=new Knob(this,"Smoothness");
	m_smoothKnob->setHintText(tr("Smoothness"), "");
	m_smoothKnob->move(80, 220);

	connect(m_generalPurposeKnob[0], SIGNAL(sliderMoved(float)), this,
			SLOT(expressionChanged()));
	connect(m_generalPurposeKnob[1], SIGNAL(sliderMoved(float)), this,
			SLOT(expressionChanged()));
	connect(m_generalPurposeKnob[2], SIGNAL(sliderMoved(float)), this,
			SLOT(expressionChanged()));

	connect(m_expressionEditor, SIGNAL(textChanged()), this,
			SLOT(expressionChanged()));
	connect(m_smoothKnob, SIGNAL(sliderMoved(float)), this,
			SLOT(smoothChanged()));

	connect(m_sinWaveBtn, SIGNAL(clicked()), this, SLOT(sinWaveClicked()));
	connect(m_triangleWaveBtn, SIGNAL(clicked()), this,
			SLOT(triangleWaveClicked()));
	connect(m_expWaveBtn, SIGNAL(clicked()), this, SLOT(expWaveClicked()));
	connect(m_moogWaveBtn, SIGNAL(clicked()), this,
			SLOT(moogSawWaveClicked()));
	connect(m_sawWaveBtn, SIGNAL(clicked()), this, SLOT(sawWaveClicked()));
	connect(m_sqrWaveBtn, SIGNAL(clicked()), this, SLOT(sqrWaveClicked()));
	connect(m_whiteNoiseWaveBtn, SIGNAL(clicked()), this,
			SLOT(noiseWaveClicked()));
	connect(m_usrWaveBtn, SIGNAL(clicked()), this, SLOT(usrWaveClicked()));
	connect(m_helpBtn, SIGNAL(clicked()), this, SLOT(helpClicked()));

	connect(m_w1Btn, SIGNAL(clicked()), this, SLOT(updateLayout()));
	connect(m_w2Btn, SIGNAL(clicked()), this, SLOT(updateLayout()));
	connect(m_w3Btn, SIGNAL(clicked()), this, SLOT(updateLayout()));
	connect(m_o1Btn, SIGNAL(clicked()), this, SLOT(updateLayout()));
	connect(m_o2Btn, SIGNAL(clicked()), this, SLOT(updateLayout()));

	updateLayout();
}

expressiveView::~expressiveView()
{
}


void expressiveView::expressionChanged() {
	Expressive * e = castModel<Expressive>();
	QByteArray text = m_expressionEditor->toPlainText().toLatin1();



	switch (m_selectedGraphGroup->model()->value()) {
	case W1_EXPR:
		e->m_wavesExpression[0] = text;
		break;
	case W2_EXPR:
		e->m_wavesExpression[1] = text;
		break;
	case W3_EXPR:
		e->m_wavesExpression[2] = text;
		break;
	case O1_EXPR:
		e->m_outputExpression[0] = text;
		break;
	case O2_EXPR:
		e->m_outputExpression[1] = text;
		break;
	}

	if (text.size()>0)
	{
		ExprFront expr(text.constData());
		float t=0;
		const float f=10,key=5,v=0.5;
		unsigned int i;
		const unsigned int sample_rate=m_raw_graph->length();
		expr.add_variable("t", t);

		if (m_output_expr)
		{
			expr.add_constant("f", f);
			expr.add_constant("key", key);
			expr.add_constant("rel", 0);
			expr.add_constant("trel", 0);
			expr.add_constant("bnote",e->instrumentTrack()->baseNote());
			expr.add_constant("v", v);
			expr.add_constant("A1", e->m_parameterA1.value());
			expr.add_constant("A2", e->m_parameterA2.value());
			expr.add_constant("A3", e->m_parameterA3.value());
			expr.add_cyclic_vector("W1",e->m_graphW1.samples(),e->m_graphW1.length());
			expr.add_cyclic_vector("W2",e->m_graphW2.samples(),e->m_graphW2.length());
			expr.add_cyclic_vector("W3",e->m_graphW3.samples(),e->m_graphW3.length());
		}
		expr.setIntegrate(&i,sample_rate);
		expr.add_constant("srate",sample_rate);

		const bool parse_ok=expr.compile();

		if (parse_ok) {
			e->m_exprValid.setValue(0);
			const int length = m_raw_graph->length();
			float * const samples = new float[length];
			for (i = 0; i < length; i++) {
				t = i / (float) length;
				samples[i] = expr.evaluate();
				if (isinff(samples[i]) != 0 || isnan(samples[i]) != 0)
					samples[i] = 0;
			}
			m_raw_graph->setSamples(samples);
			delete[] samples;
			if (m_wave_expr)
			{
				smoothChanged();
			}
			else
			{
				Engine::getSong()->setModified();
			}
		}
		else
		{
			e->m_exprValid.setValue(1);
			if (m_output_expr)
				m_raw_graph->clear();
		}
	}
	else
	{
		e->m_exprValid.setValue(0);
		if (m_output_expr)
			m_raw_graph->clear();
	}
}

void Expressive::smooth(float smoothness,const graphModel * in,graphModel * out)
{
	out->setSamples(in->samples());
	if (smoothness>0)
	{
		const int guass_size = (int)(smoothness * 5) | 1;
		const int guass_center = guass_size/2;
		const float delta = smoothness;
		const float a= 1.0f / (sqrtf(2.0f * F_PI) * delta);
		float * const guassian = new float [guass_size];
		float sum = 0.0f;
		float temp = 0.0f;
		int i;
		for (i = 0; i < guass_size; i++ )
		{
			temp = (i - guass_center) / delta;
			sum += guassian[i] = a * powf(F_E, -0.5f * temp * temp);
		}
		for (i = 0; i < guass_size; i++ )
		{
			guassian[i] = guassian[i] / sum;
		}
		out->convolve(guassian, guass_size, guass_center);
		delete [] guassian;
	}
}

void expressiveView::smoothChanged()
{

	Expressive * e = castModel<Expressive>();
	float smoothness=0;
	switch (m_selectedGraphGroup->model()->value()) {
	case W1_EXPR:
		smoothness=e->m_smoothW1.value();
		break;
	case W2_EXPR:
		smoothness=e->m_smoothW2.value();
		break;
	case W3_EXPR:
		smoothness=e->m_smoothW3.value();
		break;
	}
	Expressive::smooth(smoothness,m_raw_graph,m_graph->model());
	switch (m_selectedGraphGroup->model()->value()) {
	case W1_EXPR:
		e->m_W1.copyFrom(m_graph->model());
		break;
	case W2_EXPR:
		e->m_W2.copyFrom(m_graph->model());
		break;
	case W3_EXPR:
		e->m_W3.copyFrom(m_graph->model());
		break;
	}
	Engine::getSong()->setModified();
}

void expressiveView::modelChanged() {
	Expressive * b = castModel<Expressive>();

	m_expressionValidToggle->setModel(&b->m_exprValid);
	m_generalPurposeKnob[0]->setModel(&b->m_parameterA1);
	m_generalPurposeKnob[1]->setModel(&b->m_parameterA2);
	m_generalPurposeKnob[2]->setModel(&b->m_parameterA3);

	m_panningKnob[0]->setModel( &b->m_panning1 );
	m_panningKnob[1]->setModel( &b->m_panning2 );
	m_relKnob->setModel( &b->m_relTransition );
	m_selectedGraphGroup->setModel(&b->m_selectedGraph);

	updateLayout();
}

void expressiveView::updateLayout() {
	Expressive * e = castModel<Expressive>();
	m_output_expr=false;
	m_wave_expr=false;
	switch (m_selectedGraphGroup->model()->value()) {
	case W1_EXPR:
		m_wave_expr=true;
		m_graph->setModel(&e->m_graphW1, true);
		m_raw_graph=&(e->m_rawgraphW1);
		m_expressionEditor->setPlainText(e->m_wavesExpression[0]);
		m_smoothKnob->setModel(&e->m_smoothW1);
		m_waveInterpolate->setModel(&e->m_interpolateW1);
		m_smoothKnob->show();
		m_usrWaveBtn->show();
		m_waveInterpolate->show();
		break;
	case W2_EXPR:
		m_wave_expr=true;
		m_graph->setModel(&e->m_graphW2, true);
		m_raw_graph=&(e->m_rawgraphW2);
		m_expressionEditor->setPlainText(e->m_wavesExpression[1]);
		m_smoothKnob->setModel(&e->m_smoothW2);
		m_waveInterpolate->setModel(&e->m_interpolateW2);
		m_smoothKnob->show();
		m_usrWaveBtn->show();
		m_waveInterpolate->show();
		break;
	case W3_EXPR:
		m_wave_expr=true;
		m_graph->setModel(&e->m_graphW3, true);
		m_raw_graph=&(e->m_rawgraphW3);
		m_expressionEditor->setPlainText(e->m_wavesExpression[2]);
		m_smoothKnob->setModel(&e->m_smoothW3);
		m_waveInterpolate->setModel(&e->m_interpolateW3);
		m_smoothKnob->show();
		m_usrWaveBtn->show();
		m_waveInterpolate->show();
		break;
	case O1_EXPR:
		m_output_expr=true;
		m_graph->setModel(&e->m_graphO1, true);
		m_raw_graph=&(e->m_graphO1);
		m_expressionEditor->setPlainText(e->m_outputExpression[0]);
		m_smoothKnob->hide();
		m_usrWaveBtn->hide();
		m_waveInterpolate->hide();
		break;
	case O2_EXPR:
		m_output_expr=true;
		m_graph->setModel(&e->m_graphO2, true);
		m_raw_graph=&(e->m_graphO2);
		m_expressionEditor->setPlainText(e->m_outputExpression[1]);
		m_smoothKnob->hide();
		m_usrWaveBtn->hide();
		m_waveInterpolate->hide();
		break;
	}
}

void expressiveView::sinWaveClicked() {
	if (m_output_expr)
		m_expressionEditor->appendPlainText("sinew(t*f)");
	else
		m_expressionEditor->appendPlainText("sinew(t)");
	Engine::getSong()->setModified();
}

void expressiveView::triangleWaveClicked() {
	if (m_output_expr)
		m_expressionEditor->appendPlainText("trianglew(t*f)");
	else
		m_expressionEditor->appendPlainText("trianglew(t)");
	Engine::getSong()->setModified();
}

void expressiveView::sawWaveClicked() {
	if (m_output_expr)
		m_expressionEditor->appendPlainText("saww(t*f)");
	else
		m_expressionEditor->appendPlainText("saww(t)");
	Engine::getSong()->setModified();
}

void expressiveView::sqrWaveClicked() {
	if (m_output_expr)
		m_expressionEditor->appendPlainText("squarew(t*f)");
	else
		m_expressionEditor->appendPlainText("squarew(t)");
	Engine::getSong()->setModified();
}

void expressiveView::noiseWaveClicked() {
	m_expressionEditor->appendPlainText("rand");
	Engine::getSong()->setModified();
}

void expressiveView::moogSawWaveClicked()
{
	if (m_output_expr)
		m_expressionEditor->appendPlainText("moogsaww(t*f)");
	else
		m_expressionEditor->appendPlainText("moogsaww(t)");
	Engine::getSong()->setModified();
}
void expressiveView::expWaveClicked()
{
	if (m_output_expr)
		m_expressionEditor->appendPlainText("expw(t*f)");
	else
		m_expressionEditor->appendPlainText("expw(t)");
	Engine::getSong()->setModified();
}

void expressiveView::usrWaveClicked() {
	m_expressionEditor->setPlainText("");
	QString fileName = m_raw_graph->setWaveToUser();
	smoothChanged();
	Engine::getSong()->setModified();
}

expressiveHelpView* expressiveHelpView::s_instance=0;

QString expressiveHelpView::s_helpText=
"<b>O1, O2</b> - Two output waves. panning is controled by PN1 and PN2.<br>"
"<b>W1, W2, W3</b> - Wave samples evaluated by expression. In these samples, t variable ranges [0,1).<br>"
"These waves can be used as functions inside the output waves (O1, O2). The wave period is 1.<br>"
"<h4>Available variables:</h4><br>"
"<b>t</b> - time in seconds.<br>"
"<b>f</b> - note's pitched frequency. available only in the output expressions.<br>"
"<b>key</b> - note's keyboard key. 0 denotes C0, 48 denotes C4, 96 denotes C8. available only in the output expressions.<br>"
"<b>bnote</b> - Base note. By default it is 57 which means A5, unless you change it.<br>"
"<b>srate</b> - Sample rate. In wave expression it returns the wave's number of samples.<br>"
"<b>v</b> - note's volume. note that the output is already multiplied by the volume. available only in the output expressions.<br>"
"<b>rel</b> - gives 0.0 while the key is holded, and 1.0 after the key release. available only in the output expressions.<br>"
"<b>trel</b> - time after release. While the note is holded, it gives 0.0. then it start counting seconds.<br>"
"The time it takes to shift from 0.0 to 1.0 after key release is determined by the REL knob<br>"
"<b>A1, A2, A3</b> - general purpose knobs. you can reference them only in O1 and O2. In range [-1,1].<br>"
"<h4>Available functions:</h4><br>"
"<b>W1, W2, W3</b> - as mentioned before. you can reference them only in O1 and O2.<br>"
"<b>cent(x)</b> - gives pow(2,x/1200), so you can multiply it with the f variable to pitch the frequency.<br>"
"100 cents equals one semitone<br>"
"<b>semitone(x)</b> - gives pow(2,x/12), so you can multiply it with the f variable to pitch the frequency.<br>"
"<b>integrate(x)</b> - integrates x by delta t (it sums values and divides them by sample rate).<br>"
"If you use notes with automated frequency, you should use:<br>"
"sinew(integrate(f)) instead of sinew(t*f)<br>"
"<b>randv(x)</b> - A random vector. each cell is reference by an integer index in the range [0,2^31]<br>"
"Each evaluation of an expression results in different random vector.<br>"
"Although, it remains consistent in the lifetime of a single wave.<br>"
"If you want a single random values you can use randv(0),randv(1)... <br>"
"and every reference to randv(a) will give you the same value."
"If you want a random wave you can use randv(t*srate).<br>"
"Each random value is in the range [-1,1).<br>"
"<b>sinew(x)</b> - a sine wave with period of 1 (in contrast to real sine wave which have a period of 2*pi).<br>"
"<b>trianglew(x)</b> - a triangle wave with period of 1.<br>"
"<b>squarew(x)</b> - a square wave with period of 1.<br>"
"<b>saww(x)</b> - a saw wave with period of 1.<br>"
"<b>clamp(min_val,x,max_val)</b> - if x is in range of (min_val,max_val) returns x. otherwise if it's greater than max_val returns max_val, else returns min_val.<br>"
"<b>abs, sin, cos, tan, cot, asin, acos, atan, atan2, sinh, cosh, tanh, asinh, acosh, atanh, sinc, "
"hypot, exp, log, log2, log10, logn, pow, sqrt, min, max, floor, ceil, round, trunc, frac, "
"avg, sgn, mod, etc. are also available.</b><br>"
"<b>Operands + - * / % ^ &gt; &lt; &gt;= &lt;= == != &amp; | are also available.</b><br>"
"<b>Amplitude Modulation</b> - W1(t*f)*(1+W2(t*f))<br>"
"<b>Ring Modulation</b> - W1(t * f)*W2(t * f)<br>"
"<b>Mix Modulation</b> - 0.5*( W1(t * f) + W2(t * f) )<br>"
"<b>Frequency Modulation</b> - [vol1]*W1( integrate( f + srate*[vol2]*W2( integrate(f) ) ) )<br>"
"<b>Phase Modulation</b> - [vol1]*W1( integrate(f) + [vol2]*W2( integrate(f) ) )<br>"
		;

expressiveHelpView::expressiveHelpView():QTextEdit(s_helpText)
{
	setWindowTitle ( "X-Pressive Help" );
	setTextInteractionFlags ( Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse );
	gui->mainWindow()->addWindowedWidget( this );
	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->setWindowIcon( PLUGIN_NAME::getIconPixmap( "logo" ) );
	parentWidget()->setFixedSize( 300, 500);
}

void expressiveView::helpClicked() {
	expressiveHelpView::getInstance()->show();

}

__attribute__((destructor)) static void module_destroy()
{
	expressiveHelpView::finalize();
}

extern "C" {

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main(Model *, void * _data) {
	return (new Expressive(static_cast<InstrumentTrack *>(_data)));
}

}




