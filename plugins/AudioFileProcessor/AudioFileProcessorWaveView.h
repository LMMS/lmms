/*
 * AudioFileProcessorWaveView.h - Wave renderer of the AFP
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

#ifndef LMMS_AUDIO_FILE_PROCESSOR_WAVE_VIEW_H
#define LMMS_AUDIO_FILE_PROCESSOR_WAVE_VIEW_H


#include "Knob.h"
#include "SampleThumbnail.h"


namespace lmms
{

class Sample;

namespace gui
{

class AudioFileProcessorWaveView : public QWidget
{
	Q_OBJECT
protected:
	virtual void enterEvent(QEvent* e);
	virtual void leaveEvent(QEvent* e);
	virtual void mousePressEvent(QMouseEvent* me);
	virtual void mouseReleaseEvent(QMouseEvent* me);
	virtual void mouseMoveEvent(QMouseEvent* me);
	virtual void wheelEvent(QWheelEvent* we);
	virtual void paintEvent(QPaintEvent* pe);


public:
	enum class Point
	{
		Start,
		End,
		Loop
	} ;

	class knob : public Knob
	{
		const AudioFileProcessorWaveView* m_waveView;
		const Knob* m_relatedKnob;


	public:
		knob(QWidget* parent) :
			Knob(KnobType::Bright26, parent),
			m_waveView(0),
			m_relatedKnob(0)
		{
		}

		void setWaveView(const AudioFileProcessorWaveView* wv)
		{
			m_waveView = wv;
		}

		void setRelatedKnob(const Knob* knob)
		{
			m_relatedKnob = knob;
		}

		void slideBy(double v, bool check_bound = true)
		{
			slideTo(model()->value() + v, check_bound);
		}

		void slideTo(double v, bool check_bound = true);


	protected:
		float getValue(const QPoint & p);


	private:
		bool checkBound(double v) const;
	} ;


public slots:
	void update()
	{
		updateGraph();
		QWidget::update();
	}

	void isPlaying(lmms::f_cnt_t current_frame);


private:
	static const int s_padding = 2;

	enum class DraggingType
	{
		Wave,
		SlideWave,
		ZoomWave,
		SampleStart,
		SampleEnd,
		SampleLoop
	} ;

	Sample const* m_sample;
	QPixmap m_graph;
	int m_from;
	int m_to;
	int m_last_from;
	int m_last_to;
	float m_last_amp;
	knob* m_startKnob;
	knob* m_endKnob;
	knob* m_loopKnob;
	int m_startFrameX;
	int m_endFrameX;
	int m_loopFrameX;
	bool m_isDragging;
	QPoint m_draggingLastPoint;
	DraggingType m_draggingType;
	bool m_reversed;
	f_cnt_t m_framesPlayed;
	bool m_animation;
	SampleThumbnail m_sampleThumbnail;

	friend class AudioFileProcessorView;

public:
	AudioFileProcessorWaveView(QWidget* parent, int w, int h, Sample const* buf,
		knob* start, knob* end, knob* loop);


	void updateSampleRange();
private:
	void setTo(int to);
	void setFrom(int from);
	int range() const;
	void zoom(const bool out = false);
	void slide(int px);
	void slideSamplePointByPx(Point point, int px);
	void slideSamplePointByFrames(Point point, long frameOffset, bool slideTo = false);
	void slideSampleByFrames(long frameOffset);

	void slideSamplePointToFrames(Point point, f_cnt_t frames)
	{
		slideSamplePointByFrames(point, frames, true);
	}

	void updateGraph();
	void reverse();
	void updateCursor(QMouseEvent* me = nullptr);

	void configureKnobRelationsAndWaveViews();

	static bool isCloseTo(int a, int b)
	{
		return qAbs(a - b) < 4;
	}

} ;

} // namespace gui

} // namespace lmms

#endif // LMMS_AUDIO_FILE_PROCESSOR_WAVE_VIEW_H
