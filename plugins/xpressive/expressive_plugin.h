/*
 * expressive_plugin.h - instrument which uses a mathematical formula
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


#ifndef EXPRESSIVE_PLUGIN_H
#define EXPRESSIVE_PLUGIN_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "Graph.h"
#include "Knob.h"
#include <QPlainTextEdit>
#include "PixmapButton.h"
#include "LedCheckbox.h"
#include "MemoryManager.h"

class oscillator;
class expressiveView;

const int	W1_EXPR = 0;
const int	W2_EXPR = 1;
const int	W3_EXPR = 2;
const int	O1_EXPR = 3;
const int	O2_EXPR = 4;
const int	NUM_EXPRS = 5;


class ExprFront;
class SubWindow;

class WaveSample
{
public:
	WaveSample(int length)
	{
		m_length = length;
		m_samples = new float[m_length];
		for(int i = 0 ; i < m_length ; ++i)
			m_samples[i] = 0;
	}
	WaveSample(const graphModel * graph)
	{
		m_length = graph->length();
		m_samples = new float[m_length];
		memcpy(m_samples, graph->samples(), m_length * sizeof(float));
	}
	inline void copyFrom(const graphModel * graph)
	{
		memcpy(m_samples, graph->samples(), m_length * sizeof(float));
	}
	~WaveSample()
	{
		delete [] m_samples;
	}
	inline void setInterpolate(bool _interpolate) { m_interpolate = _interpolate; }
	float *m_samples;
	int m_length;
	bool m_interpolate;
};
class ExprSynth
{

	MM_OPERATORS
public:
	ExprSynth(const WaveSample* gW1, const WaveSample* gW2, const WaveSample* gW3, ExprFront* exprO1, ExprFront* exprO2, NotePlayHandle* nph,
			const sample_rate_t sample_rate, const FloatModel* pan1, const FloatModel* pan2, float rel_trans);
	virtual ~ExprSynth();

	void renderOutput(fpp_t frames, sampleFrame* buf );


private:
	ExprFront *m_exprO1, *m_exprO2;
	const WaveSample *m_W1, *m_W2, *m_W3;
	unsigned int m_note_sample;
	unsigned int m_note_rel_sample;
	float m_note_sample_sec;
	float m_note_rel_sec;
	float m_frequency;
	float m_released;
	NotePlayHandle* m_nph;
	const sample_rate_t m_sample_rate;
	const FloatModel *m_pan1,*m_pan2;
	float m_rel_transition;
	float m_rel_inc;

} ;

class Expressive : public Instrument
{
	Q_OBJECT
public:
	Expressive(InstrumentTrack* instrument_track );
	virtual ~Expressive();

	virtual void playNote(NotePlayHandle* nph,
						sampleFrame* working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle* nph );


	virtual void saveSettings( QDomDocument& _doc,
							QDomElement& _this );
	virtual void loadSettings( const QDomElement& _this );

	virtual QString nodeName() const;

	virtual PluginView* instantiateView( QWidget * parent );

protected:
	static void smooth(float smoothness,const graphModel* in,graphModel* out);
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
	
	friend class expressiveView;
} ;

class expressiveHelpView: public QTextEdit
{
	Q_OBJECT
public:
	static expressiveHelpView* getInstance()
	{
		if (!s_instance)
		{
			s_instance = new expressiveHelpView();
		}
		return s_instance;
	}
	static void finalize()
	{
		if (s_instance) { delete s_instance; }
	}

private:
	expressiveHelpView();
	static expressiveHelpView *s_instance;
	static QString s_helpText;

};

class expressiveView : public InstrumentView
{
	Q_OBJECT
public:
	expressiveView( Instrument* _instrument,
					QWidget* _parent );

	virtual ~expressiveView();
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
	void expressionChanged( );
	void smoothChanged( );

private:
	virtual void modelChanged();

	Knob *m_generalPurposeKnob[3];
	Knob *m_panningKnob[2];
	Knob *m_relKnob;
	Knob *m_smoothKnob;
	QPlainTextEdit * m_expressionEditor;

	automatableButtonGroup *m_selectedGraphGroup;
	PixmapButton *m_w1Btn;
	PixmapButton *m_w2Btn;
	PixmapButton *m_w3Btn;
	PixmapButton *m_o1Btn;
	PixmapButton *m_o2Btn;
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



#endif
