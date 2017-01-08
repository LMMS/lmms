/*
 * expressive_plugin.h - instrument which uses a mathematical formula
 *
 * Copyright (c) 2016-2017 Orr Dvori
 * 
 * This file is part of LMMS - http://lmms.io
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
const int	HOLD_EXPR = 5;
const int	RELEASE_EXPR = 6;
const int	NUM_EXPRS = 7;


class ExprFront;
class WaveSample
{
public:
	WaveSample(int _length)
	{
		length=_length;
		samples=new float[length];
		for(int i=0;i<length;++i)
			samples[i]=0;
	}
	WaveSample(const graphModel * graph)
	{
		length=graph->length();
		samples=new float[length];
		memcpy(samples,graph->samples(),length*sizeof(float));
	}
	void copyFrom(const graphModel * graph)
	{
		memcpy(samples,graph->samples(),length*sizeof(float));
	}
	~WaveSample()
	{
		delete [] samples;
	}
	float * samples;
	int length;
};
class exprSynth
{

	MM_OPERATORS
public:
	exprSynth(const WaveSample* gW1, const WaveSample* gW2, const WaveSample* gW3,ExprFront *_exprO1,ExprFront *_exprO2, NotePlayHandle * _nph,
			const sample_rate_t _sample_rate,const FloatModel* _pan1,const FloatModel* _pan2);
	virtual ~exprSynth();

	void renderOutput( fpp_t _frames, sampleFrame * _buf );


private:
	ExprFront *exprO1, *exprO2;
	const WaveSample *W1, *W2, *W3;
	quint64 note_sample;
	float note_sample_sec;
	float frequency;
	float released;
	NotePlayHandle* nph;
	const sample_rate_t sample_rate;
	const FloatModel *pan1,*pan2;

} ;

class expressive : public Instrument
{
	Q_OBJECT
public:
	expressive(InstrumentTrack * _instrument_track );
	virtual ~expressive();

	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return( 64 );
	}

	virtual PluginView * instantiateView( QWidget * _parent );

protected:
	static void smooth(float smoothness,const graphModel * in,graphModel * out);
protected slots:

	void samplesChanged( int, int );


	void normalize();


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
	FloatModel m_panning1;
	FloatModel m_panning2;
	float m_A1,m_A2,m_A3;
	WaveSample m_W1, m_W2, m_W3;


	
	BoolModel m_exprValid;
	
	friend class expressiveView;
} ;



class expressiveView : public InstrumentView
{
	Q_OBJECT
public:
	expressiveView( Instrument * _instrument,
					QWidget * _parent );

	virtual ~expressiveView() {};
protected:


protected slots:
	void updateLayout();

	void normalizeToggled( bool value );

	void sinWaveClicked();
	void triangleWaveClicked();
	void sqrWaveClicked();
	void sawWaveClicked();
	void noiseWaveClicked();
	void moogSawWaveClicked();
	void expWaveClicked();
	void usrWaveClicked();
	void expressionChanged( );
	void smoothChanged( );

private:
	virtual void modelChanged();

	Knob * m_generalPurposeKnob[3];
	Knob * m_panningKnob[2];
	Knob * m_smoothKnob;
	QPlainTextEdit * m_expressionEditor;

	automatableButtonGroup * m_selectedGraphGroup;
	PixmapButton * m_w1Btn;
	PixmapButton * m_w2Btn;
	PixmapButton * m_w3Btn;
	PixmapButton * m_o1Btn;
	PixmapButton * m_o2Btn;
	PixmapButton * m_holdBtn;
	PixmapButton * m_relBtn;
	PixmapButton * m_sinWaveBtn;
	PixmapButton * m_triangleWaveBtn;
	PixmapButton * m_sqrWaveBtn;
	PixmapButton * m_sawWaveBtn;
	PixmapButton * m_whiteNoiseWaveBtn;
	PixmapButton * m_usrWaveBtn;
	PixmapButton * m_moogWaveBtn;
	PixmapButton * m_expWaveBtn;

	static QPixmap * s_artwork;

	Graph * m_graph;
	LedCheckBox * m_expressionValidToggle;

} ;



#endif
