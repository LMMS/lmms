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

#include "Graph.h"
#include "Instrument.h"



#include "ExprSynth.h"

class QPlainTextEdit;

namespace lmms
{


class oscillator;

const int	W1_EXPR = 0;
const int	W2_EXPR = 1;
const int	W3_EXPR = 2;
const int	O1_EXPR = 3;
const int	O2_EXPR = 4;
const int	NUM_EXPRS = 5;


class ExprFront;


namespace gui
{

class SubWindow;
class XpressiveView;
}



class Xpressive : public Instrument
{
	Q_OBJECT
public:
	Xpressive(InstrumentTrack* instrument_track );

	void playNote(NotePlayHandle* nph,
						sampleFrame* working_buffer ) override;
	void deleteNotePluginData( NotePlayHandle* nph ) override;


	void saveSettings( QDomDocument& _doc,
							QDomElement& _this ) override;
	void loadSettings( const QDomElement& _this ) override;

	QString nodeName() const override;

	gui::PluginView* instantiateView( QWidget * parent ) override;

	graphModel& graphO1() { return m_graphO1; }
	graphModel& graphO2() { return m_graphO2; }
	graphModel& graphW1() { return m_graphW1; } //after smoothing
	graphModel& graphW2() { return m_graphW2; } //after smoothing
	graphModel& graphW3() { return m_graphW3; } //after smoothing
	graphModel& rawgraphW1() { return m_rawgraphW1; } //before smoothing
	graphModel& rawgraphW2() { return m_rawgraphW2; } //before smoothing
	graphModel& rawgraphW3() { return m_rawgraphW3; } //before smoothing
	IntModel& selectedGraph() { return m_selectedGraph; }
	QByteArray& wavesExpression(int i) { return m_wavesExpression[i]; }
	QByteArray& outputExpression(int i) { return m_outputExpression[i]; }
	void recalculateWaves();


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

	float m_previous_frequency;
	NotePlayHandle* m_nph_of_previous;

	
} ;



} // namespace lmms

#endif
