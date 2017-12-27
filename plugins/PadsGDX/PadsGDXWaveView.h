/*
 * PadsGDXWaveView.h - sample player for pads
 *
 * Copyright (c) 2017 gi0e5b06
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


#ifndef PADS_GDX_WAVE_VIEW_H
#define PADS_GDX_WAVE_VIEW_H

#include "embed.h"

#include "PadsGDX.h"
#include "PadsGDXView.h"
#include "PadsGDXWaveView.h"

#include <QPixmap>
#include <QWidget>

#include "Instrument.h"
#include "InstrumentView.h"
#include "SampleBuffer.h"
#include "Knob.h"
#include "PixmapButton.h"
#include "AutomatableButton.h"
#include "ComboBox.h"


class PadsGDXWaveView : public QWidget
{
	Q_OBJECT


 public:
	enum knobType
	{
		start,
		end,
		loopStart,
		loopEnd
	} ;

	class knob : public ::Knob
	{
		const PadsGDXWaveView * m_waveView;
		//const Knob * m_relatedKnob;


	public:
		knob( QWidget * _parent ) :
			::Knob( knobBright_26, _parent ),
			m_waveView( 0 )//,
			//m_relatedKnob( 0 )
		{
			//setFixedSize( 37, 47 );
		}

                void setWaveView( const PadsGDXWaveView * _wv )
		{
			m_waveView = _wv;
		}

                virtual void paintEvent(QPaintEvent* _pe);

                /*
		void setRelatedKnob( const Knob * _knob )
		{
			m_relatedKnob = _knob;
		}

		void slideBy(double _v, bool _checkBounds=true)
		{
			slideTo(model()->value()+_v, _checkBounds);
		}

		void slideTo(double _v, bool _checkBounds=true);


	protected:
		//float getValue( const QPoint & _p );
		void convert(const QPoint& _p, float& value_, float& dist_);


	private:
		bool checkBound( double _v ) const;
                */

	};

 public:
	PadsGDXWaveView(QWidget* _parent, int _w, int _h);
	void setKnobs(knob* _start, knob* _end, knob* _loopStart,knob* _loopEnd);
        virtual SampleBuffer* sample();
        virtual void setSample(SampleBuffer* _sample);
	virtual void update();
	virtual void fullUpdate();

 signals:

 public slots:
        //virtual void onModelChanged();
        //virtual void onSampleChanged();
        virtual void onPlaying( f_cnt_t _currentFrame );

 protected:
	virtual void updateCursor(QMouseEvent*  _me = NULL);
	virtual void paintEvent(QPaintEvent*  _pe);

	/*
          virtual void enterEvent(QEvent*  _ee);
          virtual void leaveEvent(QEvent*  _le);
          virtual void mousePressEvent(QMouseEvent*  _me);
          virtual void mouseReleaseEvent(QMouseEvent*  _me);
          virtual void mouseMoveEvent(QMouseEvent*  _me);
          virtual void mouseDoubleClickEvent(QMouseEvent*  _me);
          virtual void wheelEvent(QWheelEvent*  _we);
        */

 private:
	static const int s_padding=2;

	enum draggingType
	{
		wave,
		sample_start,
		sample_end,
		sample_loopStart,
		sample_loopEnd
	} ;

	SampleBuffer* m_sampleBuffer;
	QPixmap m_graph;
	f_cnt_t m_from;
	f_cnt_t m_to;
	f_cnt_t m_last_from;
	f_cnt_t m_last_to;
	//float m_last_amp;
	knob* m_startKnob;
	knob* m_endKnob;
	knob* m_loopStartKnob;
	knob* m_loopEndKnob;
	f_cnt_t m_startFrameX;
	f_cnt_t m_endFrameX;
	f_cnt_t m_loopStartFrameX;
	f_cnt_t m_loopEndFrameX;

        /*
	bool m_isDragging;
	QPoint m_draggingLastPoint;
	draggingType m_draggingType;
        */

	bool m_reversed;
	f_cnt_t m_framesPlayed;
	bool m_animation;

        /*
	void zoom(const bool _out=false);
	void slide(int _px);
	void slideSamplePointByPx(knobType _point, int _px);
	void slideSamplePointByFrames(knobType _point, f_cnt_t _frames, bool _slide_to = false);
	void slideSampleByFrames(f_cnt_t _frames);

	void slideSamplePointToFrames(knobType _point, f_cnt_t _frames)
	{
		slideSamplePointByFrames(_point, _frames, true);
	}
        */

	void updateRange();
	void updateGraph(bool _full);
	void reverse();

        /*
	static bool isCloseTo(int _a, int _b)
	{
		return qAbs(_a-_b)<4;
	}
        */
} ;


#endif
