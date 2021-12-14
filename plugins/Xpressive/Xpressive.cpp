/*
 * Xpressive.cpp - Instrument which uses a mathematical formula parser
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

#include "Xpressive.h"
#include "XpressiveView.h"

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

#include "embed.h"

#include "ExprSynth.h"

#include "plugin_export.h"

extern "C" {

Plugin::Descriptor PLUGIN_EXPORT xpressive_plugin_descriptor = { STRINGIFY(
	PLUGIN_NAME), "Xpressive", QT_TRANSLATE_NOOP("PluginBrowser",
	"Mathematical expression parser"), "Orr Dvori", 0x0100,
	Plugin::Instrument, new PluginPixmapLoader("logo"), NULL, NULL };

}



/*
 * nice test:
O1 -> trianglew(2t*f)*(0.5+0.5sinew(12*A1*t+0.5))+sinew(t*f)*(0.5+0.5sinew(12*A1*t))
O2 -> trianglew(2t*f)*(0.5+0.5sinew(12*A1*t))+sinew(t*f)*(0.5+0.5sinew(12*A1*t+0.5))
*/


/***********************************************************************
 *
 *	class Xpressive
 *
 *	lmms - plugin
 *
 ***********************************************************************/
#define GRAPH_LENGTH 4096

Xpressive::Xpressive(InstrumentTrack* instrument_track) :
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
	m_panning1( 1, -1.0f, 1.0f, 0.01f, this, tr("Panning 1")),
	m_panning2(-1, -1.0f, 1.0f, 0.01f, this, tr("Panning 2")),
	m_relTransition(50.0f, 0.0f, 500.0f, 1.0f, this, tr("Rel trans")),
	m_W1(GRAPH_LENGTH),
	m_W2(GRAPH_LENGTH),
	m_W3(GRAPH_LENGTH),
	m_exprValid(false, this)
{
	m_outputExpression[0]="sinew(integrate(f*(1+0.05sinew(12t))))*(2^(-(1.1+A2)*t)*(0.4+0.1(1+A3)+0.4sinew((2.5+2A1)t))^2)";
	m_outputExpression[1]="expw(integrate(f*atan(500t)*2/pi))*0.5+0.12";
}

Xpressive::~Xpressive() {
}

void Xpressive::saveSettings(QDomDocument & _doc, QDomElement & _this) {

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

void Xpressive::loadSettings(const QDomElement & _this) {

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


QString Xpressive::nodeName() const {
	return (xpressive_plugin_descriptor.name);
}

void Xpressive::playNote(NotePlayHandle* nph, sampleFrame* working_buffer) {
	m_A1=m_parameterA1.value();
	m_A2=m_parameterA2.value();
	m_A3=m_parameterA3.value();

	if (nph->totalFramesPlayed() == 0 || nph->m_pluginData == NULL) {

		ExprFront *exprO1 = new ExprFront(m_outputExpression[0].constData(),Engine::mixer()->processingSampleRate());//give the "last" function a whole second
		ExprFront *exprO2 = new ExprFront(m_outputExpression[1].constData(),Engine::mixer()->processingSampleRate());

		auto init_expression_step1 = [this, nph](ExprFront* e) { //lambda function to init exprO1 and exprO2
			//add the constants and the variables to the expression.
			#if ENABLE_CUSTOM_KEY_MAPPING //removed because changes in the microtonal tuner.
			e->add_constant("key", nph->key());//the key that was pressed.
			e->add_constant("bnote", nph->instrumentTrack()->baseNote()); // the base note
			#endif
			e->add_constant("srate", Engine::mixer()->processingSampleRate());// sample rate of the mixer
			e->add_constant("v", nph->getVolume() / 255.0); //volume of the note.
			e->add_constant("tempo", Engine::getSong()->getTempo());//tempo of the song.
			e->add_variable("A1", m_A1);//A1,A2,A3: general purpose input controls.
			e->add_variable("A2", m_A2);
			e->add_variable("A3", m_A3);
		};
		init_expression_step1(exprO1);
		init_expression_step1(exprO2);

		m_W1.setInterpolate(m_interpolateW1.value());//set interpolation according to the user selection.
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

void Xpressive::deleteNotePluginData(NotePlayHandle* nph) {
	delete static_cast<ExprSynth *>(nph->m_pluginData);
}

PluginView * Xpressive::instantiateView(QWidget* parent) {
	return (new XpressiveView(this, parent));
}

class XpressiveKnob: public Knob {
public:
	void setStyle()
	{
		setFixedSize(29, 29);
		setCenterPointX(14.5);
		setCenterPointY(14.5);
		setInnerRadius(4);
		setOuterRadius(9);
		setTotalAngle(300.0);
		setLineWidth(3);
	}
	XpressiveKnob(QWidget * _parent, const QString & _name) :
			Knob(knobStyled, _parent,_name) {
		setStyle();
	}
	XpressiveKnob(QWidget * _parent) :
			Knob(knobStyled, _parent) {
		setStyle();
	}

};


XpressiveView::XpressiveView(Instrument * _instrument, QWidget * _parent) :
	InstrumentViewFixedSize(_instrument, _parent)

{
	const int COL_KNOBS = 191;
	const int BASE_START = 2;
	const int ROW_KNOBSA1 = BASE_START;
	const int ROW_KNOBSA2 = BASE_START + 32;
	const int ROW_KNOBSA3 = BASE_START + 64;
	const int ROW_KNOBSP1 = BASE_START + 100;
	const int ROW_KNOBSP2 = BASE_START + 100 + 32;
	const int ROW_KNOBREL = BASE_START + 100 + 64;
	const int ROW_BTN = BASE_START + 85;
	const int ROW_WAVEBTN = BASE_START + 233 - 26;
	const int EXPR_TEXT_Y = BASE_START + 102;
	const int EXPR_TEXT_H = 90;

	setAutoFillBackground(true);
	QPalette pal;

	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);

	m_graph = new Graph(this, Graph::LinearStyle, 180, 81);
	m_graph->move(3, BASE_START + 1);
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
	m_w1Btn->move(3, ROW_BTN);
	m_w1Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("w1_active"));
	m_w1Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("w1_inactive"));
	ToolTip::add(m_w1Btn, tr("Select oscillator W1"));

	m_w2Btn = new PixmapButton(this, NULL);
	m_w2Btn->move(26, ROW_BTN);
	m_w2Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("w2_active"));
	m_w2Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("w2_inactive"));
	ToolTip::add(m_w2Btn, tr("Select oscillator W2"));

	m_w3Btn = new PixmapButton(this, NULL);
	m_w3Btn->move(49, ROW_BTN);
	m_w3Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("w3_active"));
	m_w3Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("w3_inactive"));
	ToolTip::add(m_w3Btn, tr("Select oscillator W3"));

	m_o1Btn = new PixmapButton(this, NULL);
	m_o1Btn->move(79, ROW_BTN);
	m_o1Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("o1_active"));
	m_o1Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("o1_inactive"));
	ToolTip::add(m_o1Btn, tr("Select output O1"));

	m_o2Btn = new PixmapButton(this, NULL);
	m_o2Btn->move(101, ROW_BTN);
	m_o2Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("o2_active"));
	m_o2Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("o2_inactive"));
	ToolTip::add(m_o2Btn, tr("Select output O2"));

	m_helpBtn = new PixmapButton(this, NULL);
	m_helpBtn->move(133, ROW_BTN);
	m_helpBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("help_active"));
	m_helpBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("help_inactive"));
	ToolTip::add(m_helpBtn, tr("Open help window"));

	m_selectedGraphGroup = new automatableButtonGroup(this);
	m_selectedGraphGroup->addButton(m_w1Btn);
	m_selectedGraphGroup->addButton(m_w2Btn);
	m_selectedGraphGroup->addButton(m_w3Btn);
	m_selectedGraphGroup->addButton(m_o1Btn);
	m_selectedGraphGroup->addButton(m_o2Btn);

	Xpressive *e = castModel<Xpressive>();
	m_selectedGraphGroup->setModel(&e->selectedGraph());

	m_sinWaveBtn = new PixmapButton(this, tr("Sine wave"));
	m_sinWaveBtn->move(4, ROW_WAVEBTN);
	m_sinWaveBtn->setActiveGraphic(embed::getIconPixmap("sin_wave_active"));
	m_sinWaveBtn->setInactiveGraphic(embed::getIconPixmap("sin_wave_inactive"));
	ToolTip::add(m_sinWaveBtn, tr("Sine wave"));

	m_moogWaveBtn = new PixmapButton(this, tr("Moog-saw wave"));
	m_moogWaveBtn->move(4, ROW_WAVEBTN-14);
	m_moogWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "moog_saw_wave_active" ) );
	m_moogWaveBtn->setInactiveGraphic(embed::getIconPixmap("moog_saw_wave_inactive"));
	ToolTip::add(m_moogWaveBtn, tr("Moog-saw wave"));

	m_expWaveBtn = new PixmapButton(this, tr("Exponential wave"));
	m_expWaveBtn->move(4 +14, ROW_WAVEBTN-14);
	m_expWaveBtn->setActiveGraphic(embed::getIconPixmap( "exp_wave_active" ) );
	m_expWaveBtn->setInactiveGraphic(embed::getIconPixmap( "exp_wave_inactive" ) );
	ToolTip::add(m_expWaveBtn, tr("Exponential wave"));

	m_sawWaveBtn = new PixmapButton(this, tr("Saw wave"));
	m_sawWaveBtn->move(4 + 14 * 2, ROW_WAVEBTN-14);
	m_sawWaveBtn->setActiveGraphic(embed::getIconPixmap("saw_wave_active"));
	m_sawWaveBtn->setInactiveGraphic(embed::getIconPixmap("saw_wave_inactive"));
	ToolTip::add(m_sawWaveBtn, tr("Saw wave"));

	m_usrWaveBtn = new PixmapButton(this, tr("User-defined wave"));
	m_usrWaveBtn->move(4 + 14 * 3, ROW_WAVEBTN-14);
	m_usrWaveBtn->setActiveGraphic(embed::getIconPixmap("usr_wave_active"));
	m_usrWaveBtn->setInactiveGraphic(embed::getIconPixmap("usr_wave_inactive"));
	ToolTip::add(m_usrWaveBtn, tr("User-defined wave"));

	m_triangleWaveBtn = new PixmapButton(this, tr("Triangle wave"));
	m_triangleWaveBtn->move(4 + 14, ROW_WAVEBTN);
	m_triangleWaveBtn->setActiveGraphic(
			embed::getIconPixmap("triangle_wave_active"));
	m_triangleWaveBtn->setInactiveGraphic(
			embed::getIconPixmap("triangle_wave_inactive"));
	ToolTip::add(m_triangleWaveBtn, tr("Triangle wave"));

	m_sqrWaveBtn = new PixmapButton(this, tr("Square wave"));
	m_sqrWaveBtn->move(4 + 14 * 2, ROW_WAVEBTN);
	m_sqrWaveBtn->setActiveGraphic(embed::getIconPixmap("square_wave_active"));
	m_sqrWaveBtn->setInactiveGraphic(
			embed::getIconPixmap("square_wave_inactive"));
	ToolTip::add(m_sqrWaveBtn, tr("Square wave"));

	m_whiteNoiseWaveBtn = new PixmapButton(this, tr("White noise"));
	m_whiteNoiseWaveBtn->move(4 + 14 * 3, ROW_WAVEBTN);
	m_whiteNoiseWaveBtn->setActiveGraphic(
			embed::getIconPixmap("white_noise_wave_active"));
	m_whiteNoiseWaveBtn->setInactiveGraphic(
			embed::getIconPixmap("white_noise_wave_inactive"));
	ToolTip::add(m_whiteNoiseWaveBtn, tr("White noise"));


	m_waveInterpolate  = new LedCheckBox("Interpolate", this, tr("WaveInterpolate"),
										 LedCheckBox::Green);
	m_waveInterpolate->move(2, 230);

	m_expressionValidToggle = new LedCheckBox("", this, tr("ExpressionValid"),
			LedCheckBox::Red);
	m_expressionValidToggle->move(168, EXPR_TEXT_Y+EXPR_TEXT_H-2);
	m_expressionValidToggle->setEnabled( false );

    m_expressionEditor = new QTextEdit(this);
    m_expressionEditor->setAcceptRichText(false);
	m_expressionEditor->move(3, EXPR_TEXT_Y);
	m_expressionEditor->resize(180, EXPR_TEXT_H);
    m_expressionEditor->setReadOnly(false);

    m_highlighter = new Highlighter(m_expressionEditor->document());

	m_generalPurposeKnob[0] = new XpressiveKnob(this,"A1");
	m_generalPurposeKnob[0]->setHintText(tr("General purpose 1:"), "");
	m_generalPurposeKnob[0]->move(COL_KNOBS, ROW_KNOBSA1);

	m_generalPurposeKnob[1] = new XpressiveKnob(this,"A2");
	m_generalPurposeKnob[1]->setHintText(tr("General purpose 2:"), "");
	m_generalPurposeKnob[1]->move(COL_KNOBS, ROW_KNOBSA2);

	m_generalPurposeKnob[2] = new XpressiveKnob(this,"A3");
	m_generalPurposeKnob[2]->setHintText(tr("General purpose 3:"), "");
	m_generalPurposeKnob[2]->move(COL_KNOBS, ROW_KNOBSA3);

	m_panningKnob[0] = new XpressiveKnob(this,"O1 panning");
	m_panningKnob[0]->setHintText(tr("O1 panning:"), "");
	m_panningKnob[0]->move(COL_KNOBS, ROW_KNOBSP1);

	m_panningKnob[1] = new XpressiveKnob(this,"O2 panning");
	m_panningKnob[1]->setHintText(tr("O2 panning:"), "");
	m_panningKnob[1]->move(COL_KNOBS, ROW_KNOBSP2);

	m_relKnob = new XpressiveKnob(this,"Release transition");
	m_relKnob->setHintText(tr("Release transition:"), "ms");
	m_relKnob->move(COL_KNOBS, ROW_KNOBREL);



	m_smoothKnob=new Knob(knobStyled, this, "Smoothness");
	m_smoothKnob->setFixedSize(25, 25);
	m_smoothKnob->setCenterPointX(12.5);
	m_smoothKnob->setCenterPointY(12.5);
	m_smoothKnob->setInnerRadius(4);
	m_smoothKnob->setOuterRadius(9);
	m_smoothKnob->setTotalAngle(280.0);
	m_smoothKnob->setLineWidth(3);
	m_smoothKnob->setHintText(tr("Smoothness"), "");
	m_smoothKnob->move(66, EXPR_TEXT_Y + EXPR_TEXT_H + 4);

    m_smoothOverlay=new QWidget(this);
    m_smoothOverlay->setGeometry(66,198,67,25);
    m_smoothOverlay->setAutoFillBackground(true);
    QPalette overlayPalette= m_smoothOverlay->palette();
    overlayPalette.setColor(QPalette::Window, 0x1e2122);
    m_smoothOverlay->setPalette(overlayPalette);



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
	connect(m_graph, SIGNAL(drawn()), this,
			SLOT(graphDrawn()));

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

XpressiveView::~XpressiveView()
{
}


void XpressiveView::expressionChanged() {
	Xpressive * e = castModel<Xpressive>();
	QByteArray text = m_expressionEditor->toPlainText().toLatin1();

	switch (m_selectedGraphGroup->model()->value()) {
	case W1_EXPR:
		e->wavesExpression(0) = text;
		break;
	case W2_EXPR:
		e->wavesExpression(1) = text;
		break;
	case W3_EXPR:
		e->wavesExpression(2) = text;
		break;
	case O1_EXPR:
		e->outputExpression(0) = text;
		break;
	case O2_EXPR:
		e->outputExpression(1) = text;
		break;
	}
	if (m_wave_expr)
		m_graph->setEnabled(m_smoothKnob->model()->value() == 0 && text.size() == 0);

	if (text.size()>0)
	{
		const unsigned int sample_rate=m_raw_graph->length();
		ExprFront expr(text.constData(),sample_rate);
		float t=0;
		const float f=10,v=0.5;
		unsigned int i;
		expr.add_variable("t", t);

		if (m_output_expr)
		{
			expr.add_constant("f", f);
			#if ENABLE_CUSTOM_KEY_MAPPING //removed because changes in the microtonal tuner.
			expr.add_constant("key", key);
			expr.add_constant("bnote",e->instrumentTrack()->baseNote());
			#endif
			expr.add_constant("rel", 0);
			expr.add_constant("trel", 0);
			expr.add_constant("v", v);
			expr.add_constant("tempo", Engine::getSong()->getTempo());
			expr.add_constant("A1", e->parameterA1().value());
			expr.add_constant("A2", e->parameterA2().value());
			expr.add_constant("A3", e->parameterA3().value());
			expr.add_cyclic_vector("W1",e->graphW1().samples(),e->graphW1().length());
			expr.add_cyclic_vector("W2",e->graphW2().samples(),e->graphW2().length());
			expr.add_cyclic_vector("W3",e->graphW3().samples(),e->graphW3().length());
		}
		expr.setIntegrate(&i,sample_rate);
		expr.add_constant("srate",sample_rate);

		const bool parse_ok=expr.compile();

		if (parse_ok) {
			e->exprValid().setValue(0);
			const int length = m_raw_graph->length();
			float * const samples = new float[length];
			for (i = 0; i < length; i++) {
				t = i / (float) length;
				samples[i] = expr.evaluate();
				if (std::isinf(samples[i]) != 0 || std::isnan(samples[i]) != 0)
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
			e->exprValid().setValue(1);
			if (m_output_expr)
				m_raw_graph->clear();
		}
	}
	else
	{
		e->exprValid().setValue(0);
		if (m_output_expr)
			m_raw_graph->clear();
	}
}

void Xpressive::smooth(float smoothness,const graphModel * in,graphModel * out)
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



void XpressiveView::smoothChanged()
{
	
	Xpressive * e = castModel<Xpressive>();
	float smoothness=0;
	switch (m_selectedGraphGroup->model()->value()) {
	case W1_EXPR:
		smoothness=e->smoothW1().value();
		break;
	case W2_EXPR:
		smoothness=e->smoothW2().value();
		break;
	case W3_EXPR:
		smoothness=e->smoothW3().value();
		break;
	}
	Xpressive::smooth(smoothness,m_raw_graph,m_graph->model());
	switch (m_selectedGraphGroup->model()->value()) {
	case W1_EXPR:
		e->W1().copyFrom(m_graph->model());
		break;
	case W2_EXPR:
		e->W2().copyFrom(m_graph->model());
		break;
	case W3_EXPR:
		e->W3().copyFrom(m_graph->model());
		break;
	}
	Engine::getSong()->setModified();
	m_graph->setEnabled(m_smoothKnob->model()->value() == 0 && m_expressionEditor->toPlainText().size() == 0);
}

void XpressiveView::graphDrawn()
{
	m_raw_graph->setSamples(m_graph->model()->samples());
	Xpressive * e = castModel<Xpressive>();
	switch (m_selectedGraphGroup->model()->value()) {
	case W1_EXPR:
		e->W1().copyFrom(m_graph->model());
		break;
	case W2_EXPR:
		e->W2().copyFrom(m_graph->model());
		break;
	case W3_EXPR:
		e->W3().copyFrom(m_graph->model());
		break;
	}
	Engine::getSong()->setModified();
}

void XpressiveView::modelChanged() {
	Xpressive * b = castModel<Xpressive>();

	m_expressionValidToggle->setModel( &b->exprValid() );
	m_generalPurposeKnob[0]->setModel( &b->parameterA1() );
	m_generalPurposeKnob[1]->setModel( &b->parameterA2() );
	m_generalPurposeKnob[2]->setModel( &b->parameterA3() );

	m_panningKnob[0]->setModel( &b->panning1() );
	m_panningKnob[1]->setModel( &b->panning2() );
	m_relKnob->setModel( &b->relTransition() );
	m_selectedGraphGroup->setModel( &b->selectedGraph() );

	updateLayout();
}

void XpressiveView::updateLayout() {
	Xpressive * e = castModel<Xpressive>();
	m_output_expr=false;
	m_wave_expr=false;
	switch (m_selectedGraphGroup->model()->value()) {
	case W1_EXPR:
		m_wave_expr=true;
		m_graph->setModel(&e->graphW1(), true);
		m_raw_graph=&(e->rawgraphW1());
		m_expressionEditor->setPlainText(e->wavesExpression(0));
		m_smoothKnob->setModel(&e->smoothW1());
        m_smoothOverlay->hide();
		m_graph->setEnabled((e->smoothW1().value() == 0 && e->wavesExpression(0).size() == 0));
		m_waveInterpolate->setModel(&e->interpolateW1());
		m_smoothKnob->show();
		m_usrWaveBtn->show();
		m_waveInterpolate->show();

		break;
	case W2_EXPR:
		m_wave_expr=true;
		m_graph->setModel(&e->graphW2(), true);
		m_raw_graph=&(e->rawgraphW2());
		m_expressionEditor->setPlainText(e->wavesExpression(1));
		m_smoothKnob->setModel(&e->smoothW2());
        m_smoothOverlay->hide();
		m_graph->setEnabled((e->smoothW2().value() == 0 && e->wavesExpression(1).size() == 0));
		m_waveInterpolate->setModel(&e->interpolateW2());
		m_smoothKnob->show();
		m_usrWaveBtn->show();
		m_waveInterpolate->show();
		break;
	case W3_EXPR:
		m_wave_expr=true;
		m_graph->setModel(&e->graphW3(), true);
		m_raw_graph=&(e->rawgraphW3());
		m_expressionEditor->setPlainText(e->wavesExpression(2));
		m_smoothKnob->setModel(&e->smoothW3());
        m_smoothOverlay->hide();
		m_graph->setEnabled((e->smoothW3().value() == 0 && e->wavesExpression(2).size() == 0));
		m_waveInterpolate->setModel(&e->interpolateW3());
		m_smoothKnob->show();
		m_usrWaveBtn->show();
		m_waveInterpolate->show();
		break;
	case O1_EXPR:
		m_output_expr=true;
		m_graph->setModel(&e->graphO1(), true);
		m_raw_graph=&(e->graphO1());
		m_expressionEditor->setPlainText(e->outputExpression(0));
		m_smoothKnob->hide();
        m_smoothOverlay->show();
		m_graph->setEnabled(false);
		m_usrWaveBtn->hide();
		m_waveInterpolate->hide();
		break;
	case O2_EXPR:
		m_output_expr=true;
		m_graph->setModel(&e->graphO2(), true);
		m_raw_graph=&(e->graphO2());
		m_expressionEditor->setPlainText(e->outputExpression(1));
		m_smoothKnob->hide();
        m_smoothOverlay->show();
		m_graph->setEnabled(false);
		m_usrWaveBtn->hide();
		m_waveInterpolate->hide();
		break;
	}
}

void XpressiveView::sinWaveClicked() {
	if (m_output_expr)
        m_expressionEditor->append("sinew(integrate(f))");
	else
        m_expressionEditor->append("sinew(t)");
	Engine::getSong()->setModified();
}

void XpressiveView::triangleWaveClicked() {
	if (m_output_expr)
        m_expressionEditor->append("trianglew(integrate(f))");
	else
        m_expressionEditor->append("trianglew(t)");
	Engine::getSong()->setModified();
}

void XpressiveView::sawWaveClicked() {
	if (m_output_expr)
        m_expressionEditor->append("saww(integrate(f))");
	else
        m_expressionEditor->append("saww(t)");
	Engine::getSong()->setModified();
}

void XpressiveView::sqrWaveClicked() {
	if (m_output_expr)
        m_expressionEditor->append("squarew(integrate(f))");
	else
        m_expressionEditor->append("squarew(t)");
	Engine::getSong()->setModified();
}

void XpressiveView::noiseWaveClicked() {
    m_expressionEditor->append("randsv(t*srate,0)");
	Engine::getSong()->setModified();
}

void XpressiveView::moogSawWaveClicked()
{
	if (m_output_expr)
        m_expressionEditor->append("moogsaww(integrate(f))");
	else
        m_expressionEditor->append("moogsaww(t)");
	Engine::getSong()->setModified();
}
void XpressiveView::expWaveClicked()
{
	if (m_output_expr)
        m_expressionEditor->append("expw(integrate(f))");
	else
        m_expressionEditor->append("expw(t)");
	Engine::getSong()->setModified();
}

void XpressiveView::usrWaveClicked() {
	m_expressionEditor->setPlainText("");
	QString fileName = m_raw_graph->setWaveToUser();
	smoothChanged();
	Engine::getSong()->setModified();
}

QString XpressiveHelpView::s_helpText=
"<b>O1, O2</b> - Two output waves. Panning is controlled by PN1 and PN2.<br>"
"<b>W1, W2, W3</b> - Wave samples evaluated by expression. In these samples, t variable ranges [0,1).<br>"
"These waves can be used as functions inside the output waves (O1, O2). The wave period is 1.<br>"
"<h4>Available variables:</h4><br>"
"<b>t</b> - Time in seconds.<br>"
"<b>f</b> - Note's pitched frequency. Available only in the output expressions.<br>"
#if ENABLE_CUSTOM_KEY_MAPPING
"<b>key</b> - Note's keyboard key. 0 denotes C-1, 60 denotes C4, 127 denotes G9. Available only in the output expressions.<br>"
"<b>bnote</b> - Base note. By default it is 69 which means A4, unless you change it.<br>"
#endif
"<b>key</b> - Note's keyboard key. 0 denotes C-1, 60 denotes C4, 127 denotes G9. Available only in the output expressions.<br>"
"<b>bnote</b> - Base note. By default it is 69 which means A4, unless you change it.<br>"
"<b>srate</b> - Sample rate. In wave expression it returns the wave's number of samples.<br>"
"<b>tempo</b> - Song's Tempo. Available only in the output expressions.<br>"
"<b>v</b> - Note's volume. Note that the output is already multiplied by the volume. Available only in the output expressions.<br>"
"<b>rel</b> - Gives 0.0 while the key is held, and 1.0 after the key release. Available only in the output expressions.<br>"
"<b>trel</b> - Time after release. While the note is held, it gives 0.0. Afterwards, it starts counting seconds.<br>"
"The time it takes to shift from 0.0 to 1.0 after key release is determined by the REL knob<br>"
"<b>seed</b> - A random value that remains consistent in the lifetime of a single wave. Meant to be used with <b>randsv</b><br>"
"<b>A1, A2, A3</b> - General purpose knobs. You can reference them only in O1 and O2. In range [-1,1].<br>"
"<h4>Available functions:</h4><br>"
"<b>W1, W2, W3</b> - As mentioned before. You can reference them only in O1 and O2.<br>"
"<b>cent(x)</b> - Gives pow(2,x/1200), so you can multiply it with the f variable to pitch the frequency.<br>"
"100 cents equals one semitone<br>"
"<b>semitone(x)</b> - Gives pow(2,x/12), so you can multiply it with the f variable to pitch the frequency.<br>"
"<b>last(n)</b> - Gives you the last n'th evaluated sample. In O1 and O2 it keeps a whole second. Thus the argument n must be in the range [1,srate], or else, it will return 0.<br>"
"<b>integrate(x)</b> - Integrates x by delta t (It sums values and divides them by sample rate).<br>"
"If you use notes with automated frequency, you should use:<br>"
"sinew(integrate(f)) instead of sinew(t*f)<br>"
"<b>randv(x)</b> - A random vector. Each cell is reference by an integer index in the range [0,2^31]<br>"
"Each note playback of an expression results in different random vector.<br>"
"Although, it remains consistent in the lifetime of a single note playback.<br>"
"If you want a single random values you can use randv(0),randv(1)... <br>"
"and every reference to randv(a) will give you the same value (for the current note playback). <br>"
"To make it more clear, if you use the randv function in your O1 or O2 formula, each time you activate the formula for a single note, "
"a different random vector will be generated. So if you play a full song that uses Xpressive synth, and use the randv "
"function in O1 or O2 output formula, each playback of the song will never be the same. <br>"
"But as the formula used to generate each time frame (sample) of the wave for the current note, "
"the randv vector remains the same for each sample generated."
"Each random value is in the range [-1,1).<br>"
"If you want a random wave you can use randv(t*srate).<br>"
"<b>randsv(x,seed)</b> - works exactly like randv(x),<br>"
"except that it lets you to select the seed manualy,<br>"
"if you want to try different random values and make it consistent in each evaluation.<br>"
"<b>sinew(x)</b> - A sine wave with period of 1 (In contrast to real sine wave which have a period of 2*pi).<br>"
"<b>trianglew(x)</b> - A triangle wave with period of 1.<br>"
"<b>squarew(x)</b> - A square wave with period of 1.<br>"
"<b>saww(x)</b> - A saw wave with period of 1.<br>"
"<b>clamp(min_val,x,max_val)</b> - If x is in range of (min_val,max_val) it returns x. Otherwise if it's greater than max_val it returns max_val, else returns min_val.<br>"
"<b>abs, sin, cos, tan, cot, asin, acos, atan, atan2, sinh, cosh, tanh, asinh, acosh, atanh, sinc, "
"hypot, exp, log, log2, log10, logn, pow, sqrt, min, max, floor, ceil, round, trunc, frac, "
"avg, sgn, mod, etc. are also available.</b><br>"
"<b>Operands + - * / % ^ &gt; &lt; &gt;= &lt;= == != &amp; | AND OR XOR NAND are also available.</b><br>"
"<b>Ternary operator is also available:  (cond?result_if_true:result_if_false)</b><br>"
"<b>Amplitude Modulation</b> - W1( integrate(f) )*( 1 + W2( integrate(f) ) )<br>"
"<b>Ring Modulation</b> - W1( integrate(f) )*W2( integrate(f) )<br>"
"<b>Mix Modulation</b> - 0.5*( W1( integrate(f) )+W2( integrate(f) ) )<br>"
"<b>Frequency Modulation</b> - [vol1]*W1( integrate( f + srate*[vol2]*W2( integrate(f) ) ) )<br>"
"<b>Phase Modulation</b> - [vol1]*W1( integrate(f)+[vol2]*W2( integrate(f) ) )<br>"
		;

XpressiveHelpView::XpressiveHelpView():QTextEdit(s_helpText)
{
	setWindowTitle ( "Xpressive Help" );
	setTextInteractionFlags ( Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse );
	gui->mainWindow()->addWindowedWidget( this );
	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->setWindowIcon( PLUGIN_NAME::getIconPixmap( "logo" ) );
	parentWidget()->setFixedSize( 300, 500);
	
	// No maximize button
	Qt::WindowFlags flags = parentWidget()->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	parentWidget()->setWindowFlags( flags );
}

void XpressiveView::helpClicked() {
	XpressiveHelpView::getInstance()->show();

}

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordClass1Format.setForeground(Qt::darkBlue);
    keywordClass1Format.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression(QStringLiteral("(?<![A-Za-z])(sin|cos|tan|cot|asin|acos|atan|atan2|sinh|cosh|tanh|asinh|acosh|atanh|"
                                  "sinc|exp|log|log2|log10|logn|pow|sqrt|min|max|abs|floor|ceil|round|trunc|frac|clamp)\\b"));
    rule.format = keywordClass1Format;
    highlightingRules.append(rule);

    keywordClass2Format.setForeground(Qt::darkMagenta);
    keywordClass2Format.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression(QStringLiteral("(?<![A-Za-z])(A1|A2|A3|pi|e|t|f|rel|trel|srate|v|tempo|seed)\\b"));
    rule.format = keywordClass2Format;
    highlightingRules.append(rule);

    keywordClass3Format.setForeground(Qt::darkYellow);
    keywordClass3Format.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression(QStringLiteral("(?<![A-Za-z])(cent|semitone|randv|randsv|integrate|last|W1|W2|W3|sinew|trianglew|squarew|expw|saww)\\b"));
    rule.format = keywordClass3Format;
    highlightingRules.append(rule);

    keywordClass4Format.setForeground(QBrush(0xff98f0));
    keywordClass4Format.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression(QStringLiteral("(?<![A-Za-z])(AND|OR|XOR|NAND)\\b|\\+|\\*|\\||\\&|\\^|\\%|\\/|\\<|\\>|\\:|\\?|\\-"));
    rule.format = keywordClass4Format;
    highlightingRules.append(rule);


    numericFormat.setForeground(Qt::green);
    rule.pattern = QRegularExpression(QStringLiteral("(?<![A-Za-z])(\\d+\\.\\d+|\\d+(\\.\\d+)?[eE][+-]?\\d+|\\d+)"));
    rule.format = numericFormat;
    highlightingRules.append(rule);

    badFormat.setForeground(Qt::red);
    badFormat.setFontItalic(true);
    rule.pattern = QRegularExpression(QStringLiteral("(?<![A-Za-z])t\\*f\\b|(?<![A-Za-z])f\\*t\\b"));
    rule.format = badFormat;
    highlightingRules.append(rule);



}

void Highlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : qAsConst(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
    int depth = 0;
    int from = 0;
    QTextCharFormat pcolors[5]= {QTextCharFormat(),QTextCharFormat(),QTextCharFormat(),QTextCharFormat(),QTextCharFormat()};
    pcolors[0].setForeground(QBrush(0xe07d00));
    pcolors[1].setForeground(QBrush(0xb1e1d0));
    pcolors[2].setForeground(QBrush(0x00e0e0));
    pcolors[3].setForeground(QBrush(0x7000e0));
    pcolors[4].setForeground(QBrush(0xe00056));
    QRegularExpression regex_braces = QRegularExpression(QStringLiteral("(\\()|(\\))"));


    while (from<text.length())
    {
        int usedDepth;
        int nextbraces = text.indexOf(
                    regex_braces, from);

        if (nextbraces == -1)
        {
            break;
        }
        if (text.at(nextbraces) == ')' )
        {
            usedDepth=--depth;
            if (depth<0)
            {
                break;
            }
        }
        else
        {
            usedDepth=depth++;
        }

        setFormat(nextbraces, 1, pcolors[(usedDepth)%5]);
        from = nextbraces + 1;
    }

 }


extern "C" {

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model *m, void *) {
	return (new Xpressive(static_cast<InstrumentTrack *>(m)));
}

}




