/*
 * LfoGraph.cpp - Displays LFO graphs
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2024-     Michael Gregorius
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

#include "LfoGraph.h"

#include <QMouseEvent>
#include <QPainter>

#include "EnvelopeAndLfoParameters.h"
#include "Oscillator.h"

#include "gui_templates.h"

namespace lmms
{

extern const float SECS_PER_LFO_OSCILLATION;

namespace gui
{

LfoGraph::LfoGraph(QWidget* parent) :
	QWidget(parent),
	ModelView(nullptr, this)
{
	setFixedSize(m_lfoGraph.size());
}

void LfoGraph::modelChanged()
{
	m_params = castModel<EnvelopeAndLfoParameters>();
}

void LfoGraph::mousePressEvent(QMouseEvent* me)
{
	if (me->button() == Qt::LeftButton)
	{
		toggleAmountModel();
	}
}

void LfoGraph::paintEvent(QPaintEvent*)
{
	QPainter p{this};
	p.setRenderHint(QPainter::Antialiasing);

	// Draw the graph background
	p.drawPixmap(rect(), m_lfoGraph);

	const auto* params = castModel<EnvelopeAndLfoParameters>();
	if (!params) { return; }

	const float amount = params->getLfoAmountModel().value();
	const float lfoSpeed = params->getLfoSpeedModel().value();
	const f_cnt_t predelayFrames = params->getLfoPredelayFrames();
	const f_cnt_t attackFrames = params->getLfoAttackFrames();
	const f_cnt_t oscillationFrames = params->getLfoOscillationFrames();
	const bool x100 = params->getX100Model().value();
	const int waveModel = params->getLfoWaveModel().value();

	int LFO_GRAPH_W = m_lfoGraph.width() - 3;	// subtract border
	int LFO_GRAPH_H = m_lfoGraph.height() - 6;	// subtract border
	int graph_x_base = 2;
	int graph_y_base = 3 + LFO_GRAPH_H / 2;

	const float frames_for_graph =
		SECS_PER_LFO_OSCILLATION * Engine::audioEngine()->baseSampleRate() / 10;

	const float gray = 1.0 - fabsf(amount);
	const auto red = static_cast<int>(96 * gray);
	const auto green = static_cast<int>(255 - 159 * gray);
	const auto blue = static_cast<int>(128 - 32 * gray);
	const QColor penColor(red, green, blue);
	p.setPen(QPen(penColor, 1.5));

	float osc_frames = oscillationFrames;

	if (x100) { osc_frames *= 100.0f; }

	float old_y = 0;
	for (int x = 0; x <= LFO_GRAPH_W; ++x)
	{
		float val = 0.0;
		float cur_sample = x * frames_for_graph / LFO_GRAPH_W;
		if (static_cast<f_cnt_t>(cur_sample) > predelayFrames)
		{
			float phase = (cur_sample -= predelayFrames) / osc_frames;
			switch (static_cast<EnvelopeAndLfoParameters::LfoShape>(waveModel))
			{
				case EnvelopeAndLfoParameters::LfoShape::SineWave:
				default:
					val = Oscillator::sinSample(phase);
					break;
				case EnvelopeAndLfoParameters::LfoShape::TriangleWave:
					val = Oscillator::triangleSample(phase);
					break;
				case EnvelopeAndLfoParameters::LfoShape::SawWave:
					val = Oscillator::sawSample(phase);
					break;
				case EnvelopeAndLfoParameters::LfoShape::SquareWave:
					val = Oscillator::squareSample(phase);
					break;
				case EnvelopeAndLfoParameters::LfoShape::RandomWave:
					if (x % (int)(900 * lfoSpeed + 1) == 0)
					{
						m_randomGraph = Oscillator::noiseSample(0.0);
					}
					val = m_randomGraph;
					break;
				case EnvelopeAndLfoParameters::LfoShape::UserDefinedWave:
					val = Oscillator::userWaveSample(m_params->getLfoUserWave().get(), phase);
					break;
			}

			if (static_cast<f_cnt_t>(cur_sample) <= attackFrames)
			{
				val *= cur_sample / attackFrames;
			}
		}

		float cur_y = -LFO_GRAPH_H / 2.0f * val;
		p.drawLine(QLineF(graph_x_base + x - 1, graph_y_base + old_y, graph_x_base + x, graph_y_base + cur_y));
		old_y = cur_y;
	}

	// Draw the info text
	int ms_per_osc = static_cast<int>(SECS_PER_LFO_OSCILLATION * lfoSpeed * 1000.0);

	QFont f = p.font();
	f.setPixelSize(height() * 0.2);
	p.setFont(f);
	p.setPen(QColor(201, 201, 225));
	p.drawText(4, m_lfoGraph.height() - 6, tr("%1 ms/LFO").arg(ms_per_osc));
}

void LfoGraph::toggleAmountModel()
{
	auto* params = castModel<EnvelopeAndLfoParameters>();
	auto& lfoAmountModel = params->getLfoAmountModel();

	lfoAmountModel.setValue(lfoAmountModel.value() < 1.0 ? 1.0 : 0.0);
}

} // namespace gui

} // namespace lmms
