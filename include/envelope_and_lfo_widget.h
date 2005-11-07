/*
 * envelope_and_lfo_widget.h - declaration of class envelopeAndLFOWidget which
 *                             is used by envelope/lfo/filter-tab of
 *                              channel-window
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#else

#include <qwidget.h>

#endif


#include "settings.h"
#include "types.h"
#include "spc_bg_hndl_widget.h"


class QPaintEvent;
class QPixmap;
class envelopeTabWidget;
class knob;
class ledCheckBox;
class pixmapButton;
class tempoSyncKnob;


class envelopeAndLFOWidget : public QWidget, public settings,
				public specialBgHandlingWidget
{
	Q_OBJECT
public:
	envelopeAndLFOWidget( float _value_for_zero_amount, QWidget * _parent );
	~envelopeAndLFOWidget();

	static inline float expKnobVal( float val )
	{
		return( ( ( val < 0 ) ? -1 : 1 ) * val*val );
	}
	static void triggerLFO( void );
	static void resetLFO( void );

	float FASTCALL level( Uint32 _frame, Uint32 _release_begin,
						Uint32 _frame_offset ) const;

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
	void paintEvent( QPaintEvent * _pe );
	void mousePressEvent( QMouseEvent * _me );

	float FASTCALL lfoLevel( Uint32 _frame, Uint32 _frame_offset ) const;


protected slots:
	void updateAfterKnobChange( float );
	void lfoSinWaveCh( bool );
	void lfoTriangleWaveCh( bool );
	void lfoSawWaveCh( bool );
	void lfoSquareWaveCh( bool );
	void x100Toggled( bool );


private:
	static Uint32 s_lfoFrame;

	bool   m_used;

	// envelope-stuff
	knob * m_predelayKnob;
	knob * m_attackKnob;
	knob * m_holdKnob;
	knob * m_decayKnob;
	knob * m_sustainKnob;
	knob * m_releaseKnob;
	knob * m_amountKnob;
	static QPixmap * s_envGraph;

	float  m_sustainLevel;
	float  m_amount;
	float  m_valueForZeroAmount;
	float  m_amountAdd;
	Uint32 m_pahdFrames;
	Uint32 m_rFrames;
	float * m_pahdEnv;
	float * m_rEnv;

	// LFO-stuff
	knob * m_lfoPredelayKnob;
	knob * m_lfoAttackKnob;
	tempoSyncKnob * m_lfoSpeedKnob;
	knob * m_lfoAmountKnob;
	pixmapButton * m_sinLfoBtn;
	pixmapButton * m_triangleLfoBtn;
	pixmapButton * m_sawLfoBtn;
	pixmapButton * m_sqrLfoBtn;
	static QPixmap * s_lfoGraph;

	ledCheckBox * m_x100Cb;
	ledCheckBox * m_controlEnvAmountCb;

	Uint32 m_lfoPredelayFrames;
	Uint32 m_lfoAttackFrames;
	Uint32 m_lfoOscillationFrames;
	float m_lfoAmount;
	bool m_lfoAmountIsZero;
	float * m_lfoShapeData;

	enum lfoShapes
	{
		SIN,
		TRIANGLE,
		SAW,
		SQUARE
	} m_lfoShape;

	volatile bool m_busy;



	friend class envelopeTabWidget;

} ;

#endif
