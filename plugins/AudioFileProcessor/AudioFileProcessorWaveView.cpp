/*
 * AudioFileProcessorWaveView.cpp - Wave renderer of the AFP
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AudioFileProcessorWaveView.h"

#include "Sample.h"
#include "ConfigManager.h"
#include "DeprecationHelper.h"
#include "SampleThumbnail.h"
#include "FontHelper.h"


#include <QPainter>
#include <QMouseEvent>

#include <algorithm>

namespace lmms
{

namespace gui
{

void AudioFileProcessorWaveView::updateSampleRange()
{
	if (m_sample->sampleSize() > 1)
	{
		const auto marging = (m_sample->endFrame() - m_sample->startFrame()) * 0.1;
		setFrom(m_sample->startFrame() - marging);
		setTo(m_sample->endFrame() + marging);
	}
}

void AudioFileProcessorWaveView::setTo(int to)
{
	m_to = std::min(to, static_cast<int>(m_sample->sampleSize()));
}

void AudioFileProcessorWaveView::setFrom(int from)
{
	m_from = std::max(from, 0);
}

int AudioFileProcessorWaveView::range() const
{
	return m_to - m_from;
}

AudioFileProcessorWaveView::AudioFileProcessorWaveView(QWidget* parent, int w, int h, Sample const* buf,
	knob* start, knob* end, knob* loop) :
	QWidget(parent),
	m_sample(buf),
	m_graph(QPixmap(w - 2 * s_padding, h - 2 * s_padding)),
	m_from(0),
	m_to(m_sample->sampleSize()),
	m_last_from(0),
	m_last_to(0),
	m_last_amp(0),
	m_startKnob(start),
	m_endKnob(end),
	m_loopKnob(loop),
	m_isDragging(false),
	m_reversed(false),
	m_framesPlayed(0),
	m_animation(ConfigManager::inst()->value("ui", "animateafp").toInt()),
	m_sampleThumbnail(*buf)
{
	setFixedSize(w, h);
	setMouseTracking(true);

	configureKnobRelationsAndWaveViews();

	updateSampleRange();

	m_graph.fill(Qt::transparent);
	update();
	updateCursor();
}

void AudioFileProcessorWaveView::isPlaying(f_cnt_t current_frame)
{
	m_framesPlayed = current_frame;
	update();
}

void AudioFileProcessorWaveView::enterEvent(QEvent * e)
{
	updateCursor();
}

void AudioFileProcessorWaveView::leaveEvent(QEvent * e)
{
	updateCursor();
}

void AudioFileProcessorWaveView::mousePressEvent(QMouseEvent * me)
{
	const auto pos = position(me);

	m_isDragging = true;
	m_draggingLastPoint = pos;

	const int x = pos.x();

	const int start_dist =		qAbs(m_startFrameX - x);
	const int end_dist = 		qAbs(m_endFrameX - x);
	const int loop_dist =		qAbs(m_loopFrameX - x);

	DraggingType dt = DraggingType::SampleLoop; int md = loop_dist;
	if (start_dist < loop_dist) { dt = DraggingType::SampleStart; md = start_dist; }
	else if (end_dist < loop_dist) { dt = DraggingType::SampleEnd; md = end_dist; }

	if (md < 4)
	{
		m_draggingType = dt;
	}
	else
	{
		m_draggingType = DraggingType::Wave;
		updateCursor(me);
	}
}

void AudioFileProcessorWaveView::mouseReleaseEvent(QMouseEvent * me)
{
	m_isDragging = false;
	if (m_draggingType == DraggingType::Wave)
	{
		updateCursor(me);
	}
}

void AudioFileProcessorWaveView::mouseMoveEvent(QMouseEvent * me)
{
	if (! m_isDragging)
	{
		updateCursor(me);
		return;
	}

	const auto pos = position(me);

	const int step = pos.x() - m_draggingLastPoint.x();
	switch(m_draggingType)
	{
		case DraggingType::SampleStart:
			slideSamplePointByPx(Point::Start, step);
			break;
		case DraggingType::SampleEnd:
			slideSamplePointByPx(Point::End, step);
			break;
		case DraggingType::SampleLoop:
			slideSamplePointByPx(Point::Loop, step);
			break;
		case DraggingType::SlideWave:
			slide(step);
			break;
		case DraggingType::ZoomWave:
			zoom(pos.y() < m_draggingLastPoint.y());
			break;
		case DraggingType::Wave:
		default:
			if (qAbs(pos.y() - m_draggingLastPoint.y())
				< 2 * qAbs(pos.x() - m_draggingLastPoint.x()))
			{
				m_draggingType = DraggingType::SlideWave;
			}
			else
			{
				m_draggingType = DraggingType::ZoomWave;
			}
	}

	m_draggingLastPoint = pos;
	update();
}

void AudioFileProcessorWaveView::wheelEvent(QWheelEvent * we)
{
	zoom(we->angleDelta().y() > 0);
	update();
}

void AudioFileProcessorWaveView::paintEvent(QPaintEvent * pe)
{
	QPainter p(this);

	p.drawPixmap(s_padding, s_padding, m_graph);

	const QRect graph_rect(s_padding, s_padding, width() - 2 * s_padding, height() - 2 * s_padding);
	const auto frames = range();
	m_startFrameX = graph_rect.x() + (m_sample->startFrame() - m_from) *
						double(graph_rect.width()) / frames;
	m_endFrameX = graph_rect.x() + (m_sample->endFrame() - m_from) *
						double(graph_rect.width()) / frames;
	m_loopFrameX = graph_rect.x() + (m_sample->loopStartFrame() - m_from) *
						double(graph_rect.width()) / frames;
	const int played_width_px = (m_framesPlayed - m_from) *
						double(graph_rect.width()) / frames;

	// loop point line
	p.setPen(QColor(0x7F, 0xFF, 0xFF)); //TODO: put into a qproperty
	p.drawLine(m_loopFrameX, graph_rect.y(),
					m_loopFrameX,
					graph_rect.height() + graph_rect.y());

	// start/end lines
	p.setPen(QColor(0xFF, 0xFF, 0xFF));  //TODO: put into a qproperty
	p.drawLine(m_startFrameX, graph_rect.y(),
					m_startFrameX,
					graph_rect.height() + graph_rect.y());
	p.drawLine(m_endFrameX, graph_rect.y(),
					m_endFrameX,
					graph_rect.height() + graph_rect.y());


	if (m_endFrameX - m_startFrameX > 2)
	{
		p.fillRect(
			m_startFrameX + 1,
			graph_rect.y(),
			m_endFrameX - m_startFrameX - 1,
			graph_rect.height() + graph_rect.y(),
			QColor(95, 175, 255, 50) //TODO: put into a qproperty
		);
		if (m_endFrameX - m_loopFrameX > 2)
			p.fillRect(
				m_loopFrameX + 1,
				graph_rect.y(),
				m_endFrameX - m_loopFrameX - 1,
				graph_rect.height() + graph_rect.y(),
				QColor(95, 205, 255, 65) //TODO: put into a qproperty
		);

		if (m_framesPlayed && m_animation)
		{
			QLinearGradient g(m_startFrameX, 0, played_width_px, 0);
			const QColor c(0, 120, 255, 180); //TODO: put into a qproperty
			g.setColorAt(0, Qt::transparent);
			g.setColorAt(0.8, c);
			g.setColorAt(1,  c);
			p.fillRect(
				m_startFrameX + 1,
				graph_rect.y(),
				played_width_px - (m_startFrameX + 1),
				graph_rect.height() + graph_rect.y(),
				g
			);
			p.setPen(QColor(255, 255, 255)); //TODO: put into a qproperty
			p.drawLine(
				played_width_px,
				graph_rect.y(),
				played_width_px,
				graph_rect.height() + graph_rect.y()
			);
			m_framesPlayed = 0;
		}
	}

	QLinearGradient g(0, 0, width() * 0.7, 0);
	const QColor c(16, 111, 170, 180);
	g.setColorAt(0, c);
	g.setColorAt(0.4, c);
	g.setColorAt(1,  Qt::transparent);
	p.fillRect(s_padding, s_padding, m_graph.width(), 14, g);

	p.setPen(QColor(255, 255, 255));
	p.setFont(adjustedToPixelSize(font(), SMALL_FONT_SIZE));

	QString length_text;
	const int length = m_sample->sampleDuration().count();

	if (length > 20000)
	{
		length_text = QString::number(length / 1000) + "s";
	}
	else if (length > 2000)
	{
		length_text = QString::number((length / 100) / 10.0) + "s";
	}
	else
	{
		length_text = QString::number(length) + "ms";
	}

	p.drawText(
		s_padding + 2,
		s_padding + 10,
		tr("Sample length:") + " " + length_text
	);
}

void AudioFileProcessorWaveView::updateGraph()
{
	if (m_to == 1)
	{
		setTo(m_sample->sampleSize() * 0.7);
		slideSamplePointToFrames(Point::End, m_to * 0.7);
	}

	if (m_from > m_sample->startFrame())
	{
		setFrom(m_sample->startFrame());
	}

	if (m_to < m_sample->endFrame())
	{
		setTo(m_sample->endFrame());
	}

	if (m_sample->reversed() != m_reversed)
	{
		reverse();
	}
	else if (m_last_from == m_from && m_last_to == m_to && m_sample->amplification() == m_last_amp)
	{
		return;
	}

	m_last_from = m_from;
	m_last_to = m_to;
	m_last_amp = m_sample->amplification();

	m_graph.fill(Qt::transparent);
	QPainter p(&m_graph);
	p.setPen(QColor(255, 255, 255));

	m_sampleThumbnail = SampleThumbnail{*m_sample};

	const auto param = SampleThumbnail::VisualizeParameters{
		.sampleRect = m_graph.rect(),
		.amplification = m_sample->amplification(),
		.sampleStart = static_cast<float>(m_from) / m_sample->sampleSize(),
		.sampleEnd = static_cast<float>(m_to) / m_sample->sampleSize(),
		.reversed = m_sample->reversed(),
	};

	m_sampleThumbnail.visualize(param, p);
}

void AudioFileProcessorWaveView::zoom(const bool out)
{
	const auto start = m_sample->startFrame();
	const auto end = m_sample->endFrame();
	const auto frames = m_sample->sampleSize();

	const auto dFrom = start - m_from;
	const auto dTo = m_to - end;

	const auto step = std::max(1.0, std::max(dFrom, dTo) / 10.0);
	const auto stepFrom = out ? -step : step;
	const auto stepTo = out ? step : -step;

	const auto boundedFrom = std::clamp(m_from + stepFrom, 0.0, static_cast<double>(start));
	const auto boundedTo = std::clamp(m_to + stepTo, static_cast<double>(end), static_cast<double>(frames));

	const auto compRatio = std::min(dFrom, dTo) / static_cast<double>(std::max(1, std::max(dFrom, dTo)));
	const auto toStep = stepFrom * (boundedTo == m_to ? 1 : compRatio);
	const auto newFrom = (out && dFrom < dTo) || (!out && dTo < dFrom)
		? boundedFrom
		: std::clamp(m_from + toStep, 0.0, static_cast<double>(start));

	const auto fromStep = stepTo * (boundedFrom == m_from ? 1 : compRatio);
	const auto newTo = (out && dFrom < dTo) || (!out && dTo < dFrom)
		? std::clamp(m_to + fromStep, static_cast<double>(end), static_cast<double>(frames))
		: boundedTo;

	if ((newTo - newFrom) / m_sample->sampleRate() > 0.05)
	{
		setFrom(newFrom);
		setTo(newTo);
	}
}

void AudioFileProcessorWaveView::slide(int px)
{
	const double fact = qAbs(double(px) / width());
	auto step = range() * fact * (px > 0 ? 1 : -1);

	const auto sampleStart = static_cast<double>(m_sample->startFrame());
	const auto sampleEnd = static_cast<double>(m_sample->endFrame());

	const auto stepFrom = std::clamp(sampleStart + step, 0.0, static_cast<double>(m_sample->sampleSize())) - sampleStart;
	const auto stepTo = std::clamp(sampleEnd + step, sampleStart + 1.0, static_cast<double>(m_sample->sampleSize())) - sampleEnd;
	step = std::abs(stepFrom) < std::abs(stepTo) ? stepFrom : stepTo;

	slideSampleByFrames(step);
}

void AudioFileProcessorWaveView::slideSamplePointByPx(Point point, int px)
{
	slideSamplePointByFrames(
		point,
		f_cnt_t((double(px) / width()) * range())
	);
}

void AudioFileProcessorWaveView::slideSamplePointByFrames(Point point, long frameOffset, bool slideTo)
{
	knob * a_knob = m_startKnob;
	switch(point)
	{
		case Point::End:
			a_knob = m_endKnob;
			break;
		case Point::Loop:
			a_knob = m_loopKnob;
			break;
		case Point::Start:
			break;
	}
	if (a_knob == nullptr)
	{
		return;
	}
	else
	{
		const double v = static_cast<double>(frameOffset) / m_sample->sampleSize();
		if (slideTo)
		{
			a_knob->slideTo(v);
		}
		else
		{
			a_knob->slideBy(v);
		}
	}
}




void AudioFileProcessorWaveView::slideSampleByFrames(long frameOffset)
{
	if (m_sample->sampleSize() <= 1)
	{
		return;
	}
	const double v = static_cast<double>(frameOffset) / m_sample->sampleSize();
	// update knobs in the right order
	// to avoid them clamping each other
	if (v < 0)
	{
		m_startKnob->slideBy(v, false);
		m_loopKnob->slideBy(v, false);
		m_endKnob->slideBy(v, false);
	}
	else
	{
		m_endKnob->slideBy(v, false);
		m_loopKnob->slideBy(v, false);
		m_startKnob->slideBy(v, false);
	}
}

void AudioFileProcessorWaveView::reverse()
{
	slideSampleByFrames(
		m_sample->sampleSize()
			- m_sample->endFrame()
			- m_sample->startFrame()
	);
	
	const int fromTmp = m_from;

	setFrom(m_sample->sampleSize() - m_to);
	setTo(m_sample->sampleSize() - fromTmp);
	m_reversed = ! m_reversed;
}

void AudioFileProcessorWaveView::updateCursor(const QMouseEvent* me)
{
	bool const waveIsDragged = m_isDragging && (m_draggingType == DraggingType::Wave);

	if (!m_isDragging && pointerCloseToStartEndOrLoop(me))
	{
		setCursor(Qt::SizeHorCursor);
	}
	else if (waveIsDragged)
	{
		setCursor(Qt::ClosedHandCursor);
	}
	else
	{
		setCursor(Qt::OpenHandCursor);
	}
}

bool AudioFileProcessorWaveView::pointerCloseToStartEndOrLoop(const QMouseEvent* me) const
{
	if (!me) { return false; }

	const QPoint pos = position(me);
	return isCloseTo(pos.x(), m_startFrameX) || isCloseTo(pos.x(), m_endFrameX) || isCloseTo(pos.x(), m_loopFrameX);
}

void AudioFileProcessorWaveView::configureKnobRelationsAndWaveViews()
{
	m_startKnob->setWaveView(this);
	m_startKnob->setRelatedKnob(m_endKnob);

	m_endKnob->setWaveView(this);
	m_endKnob->setRelatedKnob(m_startKnob);

	m_loopKnob->setWaveView(this);
}

void AudioFileProcessorWaveView::knob::slideTo(double v, bool check_bound)
{
	if (check_bound && ! checkBound(v))
	{
		return;
	}
	model()->setValue(v);
	emit sliderMoved(model()->value());
}

float AudioFileProcessorWaveView::knob::getValue(const QPoint & p)
{
	const double dec_fact = ! m_waveView ? 1 :
		static_cast<double>(m_waveView->m_to - m_waveView->m_from) / m_waveView->m_sample->sampleSize();
	const float inc = Knob::getValue(p) * dec_fact;

	return inc;
}

bool AudioFileProcessorWaveView::knob::checkBound(double v) const
{
	if (! m_relatedKnob || ! m_waveView)
	{
		return true;
	}

	if ((m_relatedKnob->model()->value() - v > 0) !=
		(m_relatedKnob->model()->value() - model()->value() >= 0))
		return false;

	const double d1 = qAbs(m_relatedKnob->model()->value() - model()->value())
		* (m_waveView->m_sample->sampleSize())
		/ m_waveView->m_sample->sampleRate();

	const double d2 = qAbs(m_relatedKnob->model()->value() - v)
		* (m_waveView->m_sample->sampleSize())
		/ m_waveView->m_sample->sampleRate();

	return d1 < d2 || d2 > 0.005;
}

} // namespace gui

} // namespace lmms
