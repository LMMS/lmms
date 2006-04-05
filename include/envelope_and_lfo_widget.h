/*
 * envelope_and_lfo_widget.h - declaration of class envelopeAndLFOWidget which
 *                             is used by envelope/lfo/filter-tab of
 *                              channel-window
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _ENVELOPE_AND_LFO_WIDGET_H
#define _ENVELOPE_AND_LFO_WIDGET_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <QWidget>
#include <QMap>
#include <QVector>

#else

#include <qwidget.h>
#include <qmap.h>
#include <qvaluevector.h>

#endif


#include "journalling_object.h"
#include "types.h"
#include "spc_bg_hndl_widget.h"
#include "sample_buffer.h"


class QPaintEvent;
class QPixmap;

class automatableButtonGroup;
class envelopeTabWidget;
class knob;
class ledCheckBox;
class pixmapButton;
class tempoSyncKnob;


class flpImport;


class envelopeAndLFOWidget : public QWidget, public journallingObject,
				public specialBgHandlingWidget
{
	Q_OBJECT
public:
	envelopeAndLFOWidget( float _value_for_zero_amount, QWidget * _parent,
							engine * _engine );
	virtual ~envelopeAndLFOWidget();

	static inline float expKnobVal( float val )
	{
		return( ( ( val < 0 ) ? -1 : 1 ) * val*val );
	}

	static void triggerLFO( engine * _engine );
	static void resetLFO( engine * _engine );

	float FASTCALL level( f_cnt_t _frame,
				const f_cnt_t _release_begin,
				const f_cnt_t _frame_offset ) const;

	inline bool used( void ) const
	{
		return( m_used );
	}


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "el" );
	}


public slots:
	void updateSampleVars( void );


protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );

	float FASTCALL lfoLevel( f_cnt_t _frame,
					const f_cnt_t _frame_offset ) const;


protected slots:
	void updateAfterKnobChange( float );
	void lfoWaveCh( int );
	void lfoUserWaveCh( bool );
	void x100Toggled( bool );


private:
	static QPixmap * s_envGraph;
	static QPixmap * s_lfoGraph;

	static QMap<engine *, vvector<envelopeAndLFOWidget *> > s_EaLWidgets;

	bool   m_used;


	// envelope-stuff
	knob * m_predelayKnob;
	knob * m_attackKnob;
	knob * m_holdKnob;
	knob * m_decayKnob;
	knob * m_sustainKnob;
	knob * m_releaseKnob;
	knob * m_amountKnob;

	float  m_sustainLevel;
	float  m_amount;
	float  m_valueForZeroAmount;
	float  m_amountAdd;
	f_cnt_t m_pahdFrames;
	f_cnt_t m_rFrames;
	sample_t * m_pahdEnv;
	sample_t * m_rEnv;

	// LFO-stuff
	knob * m_lfoPredelayKnob;
	knob * m_lfoAttackKnob;
	tempoSyncKnob * m_lfoSpeedKnob;
	knob * m_lfoAmountKnob;
	pixmapButton * m_userLfoBtn;
	automatableButtonGroup * m_lfoWaveBtnGrp;

	ledCheckBox * m_x100Cb;
	ledCheckBox * m_controlEnvAmountCb;

	f_cnt_t m_lfoPredelayFrames;
	f_cnt_t m_lfoAttackFrames;
	f_cnt_t m_lfoOscillationFrames;
	f_cnt_t m_lfoFrame;
	float m_lfoAmount;
	bool m_lfoAmountIsZero;
	sample_t * m_lfoShapeData;
	sampleBuffer m_userWave;

	enum lfoShapes
	{
		SIN,
		TRIANGLE,
		SAW,
		SQUARE,
		USER
	} m_lfoShape;

	volatile bool m_busy;



	friend class envelopeTabWidget;
	friend class flpImport;

} ;

#endif
