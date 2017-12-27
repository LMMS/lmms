/*
 * PadsGDXView.cpp - sample player for pads
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

#include <QPainter>

#include "PadsGDXWaveView.h"
#include "ConfigManager.h"


PadsGDXWaveView::PadsGDXWaveView(QWidget * _parent, int _w, int _h) :
	QWidget( _parent ),
	m_sampleBuffer( NULL ),
	m_graph( QPixmap( _w - 2 * s_padding, _h - 2 * s_padding ) ),
	m_from( 0 ),
	m_to( 0 ),
	m_last_from( 0 ),
	m_last_to( 0 ),
	//m_last_amp( 0 ),
	m_startKnob( NULL ),
	m_endKnob( NULL ),
	m_loopStartKnob( NULL ),
	m_loopEndKnob( NULL ),
	//m_isDragging( false ),
	m_reversed( false ),
	m_framesPlayed( 0 ),
	m_animation(ConfigManager::inst()->value("ui", "animateafp").toInt())
{
	setFixedSize( _w, _h );
	setMouseTracking( true );
}


SampleBuffer* PadsGDXWaveView::sample()
{
        return m_sampleBuffer;
}


void PadsGDXWaveView::setSample(SampleBuffer* _sample)
{
        if(m_sampleBuffer!=_sample)
        {
                m_sampleBuffer=_sample;
                fullUpdate();
        }
}


void PadsGDXWaveView::update()
{
        //qInfo("PadsGDXWaveView::update");
        updateRange();
        updateGraph(false);
        QWidget::update();
}


void PadsGDXWaveView::fullUpdate()
{
        //qInfo("PadsGDXWaveView::fullUpdate");
        updateRange();
        updateGraph(true);
        QWidget::update();
}


void PadsGDXWaveView::updateRange()
{
        if(m_sampleBuffer &&  m_sampleBuffer->frames()>1 )
	{
		const f_cnt_t marging=(m_sampleBuffer->endFrame()-m_sampleBuffer->startFrame())*0.1;
		m_from=qMax(0, m_sampleBuffer->startFrame()-marging);
		m_to  =qMin(m_sampleBuffer->endFrame()+marging, m_sampleBuffer->frames());
	}
        updateCursor();
}


void PadsGDXWaveView::onPlaying( f_cnt_t _current_frame )
{
        //qInfo("PadsGDXWaveView::onPlaying");
	m_framesPlayed = _current_frame;
        if(m_animation) update();
}


void PadsGDXWaveView::updateCursor(QMouseEvent* _me)
{
        if(!m_sampleBuffer)
        {
                setCursor(Qt::ArrowCursor);
                return;
        }

        /*
	if( //!m_isDragging &&
	    (_me != NULL ) &&
	    ( isCloseTo( _me->x(), m_startFrameX     ) ||
	      isCloseTo( _me->x(), m_endFrameX       ) ||
	      isCloseTo( _me->x(), m_loopStartFrameX ) ||
	      isCloseTo( _me->x(), m_loopEndFrameX   ) ))
		setCursor(Qt::SizeHorCursor);
	//else
	//if( m_isDragging && (m_draggingType == wave) )
	//	setCursor(Qt::ClosedHandCursor);
	else
        */
        setCursor(Qt::ArrowCursor);
}


void PadsGDXWaveView::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );

	p.drawPixmap( s_padding, s_padding, m_graph );

        if(m_sampleBuffer)
        {
                const QRect graph_rect( s_padding, s_padding, width() - 2 * s_padding, height() - 2 * s_padding );
                const f_cnt_t frames = m_to - m_from;
                m_startFrameX = graph_rect.x() + ( m_sampleBuffer->startFrame() - m_from ) *
                        double( graph_rect.width() ) / frames;
                m_endFrameX = graph_rect.x() + ( m_sampleBuffer->endFrame() - m_from ) *
                        double( graph_rect.width() ) / frames;
                m_loopStartFrameX = graph_rect.x() + ( m_sampleBuffer->loopStartFrame() - m_from ) *
                        double( graph_rect.width() ) / frames;
                m_loopEndFrameX = graph_rect.x() + ( m_sampleBuffer->loopEndFrame() - m_from ) *
                        double( graph_rect.width() ) / frames;

                const int xw=(m_framesPlayed-m_from)*double(graph_rect.width())/frames;
                const int yw=graph_rect.y();
                const int hw=graph_rect.height() + graph_rect.y();

                // z 1
                p.fillRect(m_startFrameX+1,yw,
                           m_loopStartFrameX-m_startFrameX-1,hw,
                           QColor(0,255,255,128));
                // z 2
                p.fillRect(m_loopStartFrameX+1,yw,
                           m_loopEndFrameX-m_loopStartFrameX-1,hw,
                           QColor(0,255,0,128));
                // z 3
                p.fillRect(m_loopEndFrameX+1,yw,
                           m_endFrameX-m_loopEndFrameX-1,hw,
                           QColor(255,0,0,128));

                // start line
                p.setPen(Qt::cyan);
                p.drawLine(m_startFrameX,yw,m_startFrameX,hw);
                // loop end line
                p.setPen(Qt::green);
                p.drawLine(m_loopStartFrameX,yw,m_loopStartFrameX,hw);
                // loop end line
                p.setPen(Qt::red);
                p.drawLine(m_loopEndFrameX,yw,m_loopEndFrameX,hw);
                // end line
                p.setPen(Qt::black);
                p.drawLine(m_endFrameX,yw,m_endFrameX,hw);

		if( m_framesPlayed && m_animation)
		{
                        p.setPen(Qt::white);
			p.drawLine(xw,yw,xw,hw);
			m_framesPlayed = 0;
		}
        }
}


void PadsGDXWaveView::updateGraph(bool _full)
{
        if(!m_sampleBuffer)
        {
                m_graph.fill( Qt::transparent );
                return;
        }

        /*
	if( m_to == 1 )
	{
		m_to = m_sampleBuffer->frames() * 0.7;
		slideSamplePointToFrames( end, m_to * 0.7 );
	}
        */

	if( m_from > m_sampleBuffer->startFrame() )
	{
		m_from=m_sampleBuffer->startFrame()*9/10;
	}

	if( m_to < m_sampleBuffer->endFrame() )
	{
		m_to=qMin(m_sampleBuffer->frames(),m_sampleBuffer->endFrame()*11/10);
	}

	if( m_sampleBuffer->reversed() != m_reversed )
	{
		reverse();
                _full=true;
	}

	if( !_full &&
            m_last_from==m_from &&
            m_last_to==m_to )
                //&& m_sampleBuffer->amplification()==m_last_amp )
		return;

	m_last_from=m_from;
	m_last_to  =m_to;
	//m_last_amp =m_sampleBuffer->amplification();

	m_graph.fill( Qt::transparent );
	QPainter p( &m_graph );
        //QRect r(0,0,m_graph.width(),m_graph.height());
	m_sampleBuffer->visualize(p,rect(),rect(),m_from,m_to);
        p.end();
}


void PadsGDXWaveView::reverse()
{
        if(!m_sampleBuffer) return;

        /*
	slideSampleByFrames(m_sampleBuffer->frames()
                            - m_sampleBuffer->endFrame()
                            - m_sampleBuffer->startFrame());
        */
	const f_cnt_t tmp=m_from;
	m_from=m_sampleBuffer->frames()-m_to;
	m_to  =m_sampleBuffer->frames()-tmp;

	m_reversed=!m_reversed;
}


void PadsGDXWaveView::setKnobs(knob* _start, knob* _end, knob* _loopStart, knob* _loopEnd)
{
	m_startKnob    =_start;
	m_endKnob      =_end;
	m_loopStartKnob=_loopStart;
	m_loopEndKnob  =_loopEnd;

        m_startKnob->setWaveView(this);
        //m_startKnob->setRelatedKnob(m_endKnob);
        m_endKnob->setWaveView(this);
        //m_endKnob->setRelatedKnob(m_startKnob);
        m_loopStartKnob->setWaveView(this);
        //m_loopStartKnob->setRelatedKnob(m_loopEndKnob);
        m_loopEndKnob->setWaveView(this);
        //m_loopEndKnob->setRelatedKnob(m_loopStartKnob);
}


/*
void PadsGDXWaveView::enterEvent( QEvent * _e )
{
	updateCursor();
}


void PadsGDXWaveView::leaveEvent( QEvent * _e )
{
	updateCursor();
}


void PadsGDXWaveView::mousePressEvent( QMouseEvent * _me )
{
        if(!m_sampleBuffer) return;

	m_isDragging       =true;
	m_draggingLastPoint=_me->pos();

	const int x=_me->x();

	const int start_dist     =qAbs( m_startFrameX    -x);
	const int end_dist       =qAbs( m_endFrameX      -x);
	const int loopStart_dist =qAbs( m_loopStartFrameX-x);
	const int loopEnd_dist   =qAbs( m_loopEndFrameX  -x);

	draggingType dt=sample_start;
        int          md=start_dist;
	     if(loopStart_dist<md) { dt=sample_loopStart; md=loopStart_dist; }
	     if(loopEnd_dist  <md) { dt=sample_loopEnd;   md=loopEnd_dist;   }
	     if(end_dist      <md) { dt=sample_end;       md=end_dist;       }

	if(md<4)
	{
		m_draggingType=dt;
	}
	else
	{
		m_draggingType=wave;
		updateCursor(_me);
	}
}




void PadsGDXWaveView::mouseReleaseEvent( QMouseEvent * _me )
{
        if(!m_sampleBuffer) return;

	m_isDragging = false;
	if( m_draggingType == wave )
	{
		updateCursor(_me);
	}
}




void PadsGDXWaveView::mouseMoveEvent( QMouseEvent * _me )
{
        if(!m_sampleBuffer) return;

	if( ! m_isDragging )
	{
		updateCursor(_me);
		return;
	}

	const int step = _me->x() - m_draggingLastPoint.x();
	switch( m_draggingType )
	{
		case sample_start:
			slideSamplePointByPx( start, step );
			break;
		case sample_end:
			slideSamplePointByPx( end, step );
			break;
		case sample_loopStart:
			slideSamplePointByPx( loopStart, step );
			break;
		case sample_loopEnd:
			slideSamplePointByPx( loopEnd, step );
			break;
		case wave:
		default:
			if( qAbs( _me->y() - m_draggingLastPoint.y() )
				< 2 * qAbs( _me->x() - m_draggingLastPoint.x() ) )
			{
				slide( step );
			}
			else
			{
				zoom( _me->y() < m_draggingLastPoint.y() );
			}
	}

	m_draggingLastPoint = _me->pos();
	update();
}


void PadsGDXWaveView::mouseDoubleClickEvent ( QMouseEvent * _me )
{
        if(!m_sampleBuffer) return;

        f_cnt_t step0,step1;
        switch( m_draggingType )
        {
        case sample_start:
                step0=m_sampleBuffer->startFrame();
                step1=m_sampleBuffer->findClosestZero(step0);
                qInfo("mouseDoubleClickEvent: start=%d, step=%d",step0,step1);
                slideSamplePointByFrames(start,step1-step0);
                break;
        case sample_end:
                step0=m_sampleBuffer->endFrame();
                step1=m_sampleBuffer->findClosestZero(step0);
                qInfo("mouseDoubleClickEvent: end=%d, step=%d",step0,step1);
                slideSamplePointByFrames(end,step1-step0);
                break;
        case sample_loopStart:
                step0=m_sampleBuffer->loopStartFrame();
                step1=m_sampleBuffer->findClosestZero(step0);
                qInfo("mouseDoubleClickEvent: loopStart=%d, step=%d",step0,step1);
                slideSamplePointByFrames(loopStart,step1-step0);
                break;
        case sample_loopEnd:
                step0=m_sampleBuffer->loopEndFrame();
                step1=m_sampleBuffer->findClosestZero(step0);
                qInfo("mouseDoubleClickEvent: loopEnd=%d, step=%d",step0,step1);
                slideSamplePointByFrames(loopEnd,step1-step0);
                break;
        default:
                break;
        }

	updateCursor(_me);
};


void PadsGDXWaveView::wheelEvent( QWheelEvent * _we )
{
        if(!m_sampleBuffer) return;

	zoom( _we->delta() > 0 );
	update();
}


void PadsGDXWaveView::zoom( const bool _out )
{
        if(!m_sampleBuffer) return;

	const f_cnt_t start = m_sampleBuffer->startFrame();
	const f_cnt_t end = m_sampleBuffer->endFrame();
	const f_cnt_t frames = m_sampleBuffer->frames();
	const f_cnt_t d_from = start - m_from;
	const f_cnt_t d_to = m_to - end;

	const f_cnt_t step = qMax( 1, qMax( d_from, d_to ) / 10 );
	const f_cnt_t step_from = ( _out ? - step : step );
	const f_cnt_t step_to = ( _out ? step : - step );

	const double comp_ratio = double( qMin( d_from, d_to ) )
                / qMax( 1, qMax( d_from, d_to ) );

	f_cnt_t new_from;
	f_cnt_t new_to;

	if( ( _out && d_from < d_to ) || ( ! _out && d_to < d_from ) )
	{
		new_from = qBound( 0, m_from + step_from, start );
		new_to = qBound(
			end,
			m_to + f_cnt_t( step_to * ( new_from == m_from ? 1 : comp_ratio ) ),
			frames
		);
	}
	else
	{
		new_to = qBound( end, m_to + step_to, frames );
		new_from = qBound(
			0,
			m_from + f_cnt_t( step_from * ( new_to == m_to ? 1 : comp_ratio ) ),
			start
		);
	}

	if( double( new_to - new_from ) / m_sampleBuffer->sampleRate() > 0.05  )
	{
		m_from = new_from;
		m_to = new_to;
	}
}




void PadsGDXWaveView::slide( int _px )
{
        if(!m_sampleBuffer) return;

	const double fact = qAbs( double( _px ) / width() );
	f_cnt_t step = ( m_to - m_from ) * fact;
	if( _px > 0 )
	{
		step = -step;
	}

	f_cnt_t step_from = qBound( 0, m_from + step, m_sampleBuffer->frames() ) - m_from;
	f_cnt_t step_to = qBound( m_from + 1, m_to + step, m_sampleBuffer->frames() ) - m_to;

	step = qAbs( step_from ) < qAbs( step_to ) ? step_from : step_to;

	m_from += step;
	m_to += step;
	slideSampleByFrames( step );
}


void PadsGDXWaveView::slideSamplePointByPx( knobType _point, int _px )
{
	slideSamplePointByFrames(_point,
                                 f_cnt_t( double(_px)/width() * (m_to-m_from) ));
}


void PadsGDXWaveView::slideSamplePointByFrames(knobType _point, f_cnt_t _frames, bool _slideTo)
{
        if(!m_sampleBuffer) return;

	knob* a_knob;
	switch( _point )
	{
        case start:
                a_knob=m_startKnob;
                break;
        case end:
                a_knob=m_endKnob;
                break;
        case loopStart:
                a_knob=m_loopStartKnob;
                break;
        case loopEnd:
                a_knob=m_loopEndKnob;
                break;
        default:
                qFatal("PadsGDXWaveView::slideSamplePointByFrames invalid KnobType: %d",_point);
	}

        const double v=static_cast<double>(_frames)/m_sampleBuffer->frames();
        if(_slideTo) a_knob->slideTo(v);
        else         a_knob->slideBy(v);
}


void PadsGDXWaveView::slideSampleByFrames( f_cnt_t _frames )
{
	if( !m_sampleBuffer || m_sampleBuffer->frames()<= 1 )
		return;

	const double v=static_cast<double>(_frames)/m_sampleBuffer->frames();
	if(m_startKnob    ) m_startKnob    ->slideBy(v,false);
	if(m_endKnob      ) m_endKnob      ->slideBy(v,false);
	if(m_loopStartKnob) m_loopStartKnob->slideBy(v,false);
	if(m_loopEndKnob  ) m_loopEndKnob  ->slideBy(v,false);
}

void PadsGDXWaveView::knob::slideTo(double _v, bool _checkBound)
{
	//if(_checkBound && !checkBound(_v)) return;
	model()->setValue(_v);
	emit sliderMoved(model()->value());
}


void PadsGDXWaveView::knob::convert(const QPoint& _p, float& value_, float& dist_)
{
        if(!m_waveView) return;
        if(!m_waveView->m_sampleBuffer) return;

	const double dec_fact = ! m_waveView ? 1 :
		double( m_waveView->m_to - m_waveView->m_from )
			/ m_waveView->m_sampleBuffer->frames();
	//const float inc = ::Knob::getValue( _p ) * dec_fact;
	::Knob::convert(_p,value_,dist_);
	value_*=dec_fact;
}

bool PadsGDXWaveView::knob::checkBound(double _v) const
{
        if(!m_waveView) return true;
        if(!m_waveView->m_sampleBuffer) return true;
	if(! m_relatedKnob) return true;

	if( ( m_relatedKnob->model()->value() - _v > 0 ) !=
            ( m_relatedKnob->model()->value() - model()->value() >= 0 ) )
		return false;

	const double d1 = qAbs( m_relatedKnob->model()->value() - model()->value() )
		* ( m_waveView->m_sampleBuffer->frames() )
		/ m_waveView->m_sampleBuffer->sampleRate();

	const double d2 = qAbs( m_relatedKnob->model()->value() - _v )
		* ( m_waveView->m_sampleBuffer->frames() )
		/ m_waveView->m_sampleBuffer->sampleRate();

	return d1 < d2 || d2 > 0.005;
}
*/

void PadsGDXWaveView::knob::paintEvent(QPaintEvent* _pe)
{
        if(model()->isDefaultConstructed()) return;
        Knob::paintEvent(_pe);
}
