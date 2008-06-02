/*
 * envelope_and_lfo_parameters.h - class envelopeAndLFOParameters
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef _ENVELOPE_AND_LFO_PARAMETERS_H
#define _ENVELOPE_AND_LFO_PARAMETERS_H

#include <QtCore/QVector>

#include "journalling_object.h"
#include "automatable_model.h"
#include "sample_buffer.h"
#include "tempo_sync_knob.h"
#include "types.h"


class track;



class EXPORT envelopeAndLFOParameters : public model, public journallingObject
{
	Q_OBJECT
public:
	envelopeAndLFOParameters( float _value_for_zero_amount,
							track * _track,
							model * _parent );
	virtual ~envelopeAndLFOParameters();

	static inline float expKnobVal( float _val )
	{
		return( ( ( _val < 0 ) ? -_val : _val ) * _val );
	}

	static void triggerLFO( void );
	static void resetLFO( void );

	void fillLevel( float * _buf, f_cnt_t _frame,
				const f_cnt_t _release_begin,
				const fpp_t _frames );

	inline bool used( void ) const
	{
		return( m_used );
	}


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	virtual QString nodeName( void ) const
	{
		return( "el" );
	}

	inline f_cnt_t PAHD_Frames( void ) const
	{
		return( m_pahdFrames );
	}

	inline f_cnt_t releaseFrames( void ) const
	{
		return( m_rFrames );
	}


public slots:
	void updateSampleVars( void );


protected:
	void fillLFOLevel( float * _buf, f_cnt_t _frame, const fpp_t _frames );


private:
	static QVector<envelopeAndLFOParameters *> s_EaLParametersInstances;

	bool m_used;


	floatModel m_predelayModel;
	floatModel m_attackModel;
	floatModel m_holdModel;
	floatModel m_decayModel;
	floatModel m_sustainModel;
	floatModel m_releaseModel;
	floatModel m_amountModel;

	float  m_sustainLevel;
	float  m_amount;
	float  m_valueForZeroAmount;
	float  m_amountAdd;
	f_cnt_t m_pahdFrames;
	f_cnt_t m_rFrames;
	sample_t * m_pahdEnv;
	sample_t * m_rEnv;


	floatModel m_lfoPredelayModel;
	floatModel m_lfoAttackModel;
	tempoSyncKnobModel m_lfoSpeedModel;
	floatModel m_lfoAmountModel;
	intModel m_lfoWaveModel;

	boolModel m_x100Model;
	boolModel m_controlEnvAmountModel;


	f_cnt_t m_lfoPredelayFrames;
	f_cnt_t m_lfoAttackFrames;
	f_cnt_t m_lfoOscillationFrames;
	f_cnt_t m_lfoFrame;
	float m_lfoAmount;
	bool m_lfoAmountIsZero;
	sample_t * m_lfoShapeData;
	bool m_bad_lfoShapeData;
	sampleBuffer m_userWave;

	enum LfoShapes
	{
		SineWave,
		TriangleWave,
		SawWave,
		SquareWave,
		UserDefinedWave,
		NumLfoShapes
	} ;

	sample_t lfoShapeSample( fpp_t _frame_offset );
	void updateLFOShapeData( void );



	friend class envelopeAndLFOView;
	friend class flpImport;

} ;

#endif
