/*
 * Xpressive.h - Instrument which uses a mathematical formula parser
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


#ifndef XPRESSIVE_H
#define XPRESSIVE_H


#include <QTextEdit>

#include "AutomatableModel.h"
#include "Graph.h"
#include "Instrument.h"
#include "InstrumentView.h"

#include "ExprSynth.h"

class QPlainTextEdit;

namespace lmms
{


const int	W1_EXPR = 0;
const int	W2_EXPR = 1;
const int	W3_EXPR = 2;
const int	O1_EXPR = 3;
const int	O2_EXPR = 4;
const int	NUM_EXPRS = 5;


namespace gui
{
class AutomatableButtonGroup;
class Knob;
class LedCheckBox;
class PixmapButton;
}


class Xpressive : public Instrument
{
	Q_OBJECT
public:
	Xpressive(InstrumentTrack* instrument_track );

	void playNote(NotePlayHandle* nph,
						SampleFrame* working_buffer ) override;
	void deleteNotePluginData( NotePlayHandle* nph ) override;


	void saveSettings( QDomDocument& _doc,
							QDomElement& _this ) override;
	void loadSettings( const QDomElement& _this ) override;

	QString nodeName() const override;

	gui::PluginView* instantiateView( QWidget * parent ) override;

	graphModel& graphO1() { return m_graphO1; }
	graphModel& graphO2() { return m_graphO2; }
	graphModel& graphW1() { return m_graphW1; }
	graphModel& graphW2() { return m_graphW2; }
	graphModel& graphW3() { return m_graphW3; }
	graphModel& rawgraphW1() { return m_rawgraphW1; }
	graphModel& rawgraphW2() { return m_rawgraphW2; }
	graphModel& rawgraphW3() { return m_rawgraphW3; }
	IntModel& selectedGraph() { return m_selectedGraph; }
	QByteArray& wavesExpression(int i) { return m_wavesExpression[i]; }
	QByteArray& outputExpression(int i) { return m_outputExpression[i]; }

	FloatModel& parameterA1() { return m_parameterA1; }
	FloatModel& parameterA2() { return m_parameterA2; }
	FloatModel& parameterA3() { return m_parameterA3; }
	FloatModel& smoothW1() { return m_smoothW1; }
	FloatModel& smoothW2() { return m_smoothW2; }
	FloatModel& smoothW3() { return m_smoothW3; }
	BoolModel& interpolateW1() { return m_interpolateW1; }
	BoolModel& interpolateW2() { return m_interpolateW2; }
	BoolModel& interpolateW3() { return m_interpolateW3; }
	FloatModel& panning1() { return m_panning1; }
	FloatModel& panning2() { return m_panning2; }
	FloatModel& relTransition() { return m_relTransition; }
	WaveSample& W1() { return m_W1; }
	WaveSample& W2() { return m_W2; }
	WaveSample& W3() { return m_W3; }
	BoolModel& exprValid() { return m_exprValid; }
	static void smooth(float smoothness,const graphModel* in,graphModel* out);
protected:
	
protected slots:


private:
	graphModel  m_graphO1;
	graphModel  m_graphO2;
	graphModel  m_graphW1;
	graphModel  m_graphW2;
	graphModel  m_graphW3;
	graphModel  m_rawgraphW1;
	graphModel  m_rawgraphW2;
	graphModel  m_rawgraphW3;
	IntModel m_selectedGraph;
	QByteArray m_wavesExpression[3];
	QByteArray m_outputExpression[2];
	FloatModel m_parameterA1;
	FloatModel m_parameterA2;
	FloatModel m_parameterA3;
	FloatModel m_smoothW1;
	FloatModel m_smoothW2;
	FloatModel m_smoothW3;
	BoolModel m_interpolateW1;
	BoolModel m_interpolateW2;
	BoolModel m_interpolateW3;
	FloatModel m_panning1;
	FloatModel m_panning2;
	FloatModel m_relTransition;
	float m_A1,m_A2,m_A3;
	WaveSample m_W1, m_W2, m_W3;

	BoolModel m_exprValid;
	
} ;

namespace gui
{


class XpressiveView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	XpressiveView( Instrument* _instrument,
					QWidget* _parent );

protected:


protected slots:
	void updateLayout();

	void sinWaveClicked();
	void triangleWaveClicked();
	void sqrWaveClicked();
	void sawWaveClicked();
	void noiseWaveClicked();
	void moogSawWaveClicked();
	void expWaveClicked();
	void usrWaveClicked();
	void helpClicked();
	void expressionChanged();
	void smoothChanged();
	void graphDrawn();

private:
	void modelChanged() override;

	Knob *m_generalPurposeKnob[3];
	Knob *m_panningKnob[2];
	Knob *m_relKnob;
	Knob *m_smoothKnob;
	QPlainTextEdit * m_expressionEditor;

	AutomatableButtonGroup *m_selectedGraphGroup;
	PixmapButton *m_w1Btn;
	PixmapButton *m_w2Btn;
	PixmapButton *m_w3Btn;
	PixmapButton *m_o1Btn;
	PixmapButton *m_o2Btn;
	PixmapButton *m_helpBtn;
	PixmapButton *m_sinWaveBtn;
	PixmapButton *m_triangleWaveBtn;
	PixmapButton *m_sqrWaveBtn;
	PixmapButton *m_sawWaveBtn;
	PixmapButton *m_whiteNoiseWaveBtn;
	PixmapButton *m_usrWaveBtn;
	PixmapButton *m_moogWaveBtn;
	PixmapButton *m_expWaveBtn;

	static QPixmap *s_artwork;

	Graph *m_graph;
	graphModel *m_raw_graph;
	LedCheckBox *m_expressionValidToggle;
	LedCheckBox *m_waveInterpolate;
	bool m_output_expr;
	bool m_wave_expr;
} ;

class XpressiveHelpView: public QTextEdit
{
	Q_OBJECT
public:
	static XpressiveHelpView* getInstance()
	{
		static XpressiveHelpView instance;
		return &instance;
	}
	static void finalize()
	{
	}

private:
	XpressiveHelpView();
	static QString s_helpText;
};


} // namespace gui

} // namespace lmms

#endif
