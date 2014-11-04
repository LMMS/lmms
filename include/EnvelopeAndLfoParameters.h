/*
 * EnvelopeAndLfoParameters.h - class EnvelopeAndLfoParameters
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _ENVELOPE_AND_LFO_PARAMETERS_H
#define _ENVELOPE_AND_LFO_PARAMETERS_H

#include <QtCore/QVector>

#include "JournallingObject.h"
#include "AutomatableModel.h"
#include "SampleBuffer.h"
#include "TempoSyncKnobModel.h"
#include "lmms_basics.h"


class EXPORT EnvelopeAndLfoParameters : public Model, public JournallingObject
{
	Q_OBJECT
public:
	class LfoInstances
	{
	public:
		LfoInstances()
		{
		}

		~LfoInstances()
		{
		}

		inline bool isEmpty() const
		{
			return m_lfos.isEmpty();
		}

		void trigger();
		void reset();

		void add( EnvelopeAndLfoParameters * lfo );
		void remove( EnvelopeAndLfoParameters * lfo );

	private:
		QMutex m_lfoListMutex;
		typedef QList<EnvelopeAndLfoParameters *> LfoList;
		LfoList m_lfos;

	} ;

	EnvelopeAndLfoParameters( float _value_for_zero_amount,
							Model * _parent );
	virtual ~EnvelopeAndLfoParameters();

	static inline float expKnobVal( float _val )
	{
		return ( ( _val < 0 ) ? -_val : _val ) * _val;
	}

	static LfoInstances * instances()
	{
		return s_lfoInstances;
	}

	void fillLevel( float * _buf, f_cnt_t _frame,
				const f_cnt_t _release_begin,
				const fpp_t _frames );

	inline bool isUsed() const
	{
		return m_used;
	}


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	virtual QString nodeName() const
	{
		return "el";
	}

	inline f_cnt_t PAHD_Frames() const
	{
		return m_pahdFrames;
	}

	inline f_cnt_t releaseFrames() const
	{
		return m_rFrames;
	}


public slots:
	void updateSampleVars();


protected:
	void fillLfoLevel( float * _buf, f_cnt_t _frame, const fpp_t _frames );


private:
	static LfoInstances * s_lfoInstances;
	bool m_used;


	FloatModel m_predelayModel;
	FloatModel m_attackModel;
	FloatModel m_holdModel;
	FloatModel m_decayModel;
	FloatModel m_sustainModel;
	FloatModel m_releaseModel;
	FloatModel m_amountModel;

	float  m_sustainLevel;
	float  m_amount;
	float  m_valueForZeroAmount;
	float  m_amountAdd;
	f_cnt_t m_pahdFrames;
	f_cnt_t m_rFrames;
	sample_t * m_pahdEnv;
	sample_t * m_rEnv;


	FloatModel m_lfoPredelayModel;
	FloatModel m_lfoAttackModel;
	TempoSyncKnobModel m_lfoSpeedModel;
	FloatModel m_lfoAmountModel;
	IntModel m_lfoWaveModel;

	BoolModel m_x100Model;
	BoolModel m_controlEnvAmountModel;


	f_cnt_t m_lfoPredelayFrames;
	f_cnt_t m_lfoAttackFrames;
	f_cnt_t m_lfoOscillationFrames;
	f_cnt_t m_lfoFrame;
	float m_lfoAmount;
	bool m_lfoAmountIsZero;
	sample_t * m_lfoShapeData;
	sample_t m_random;
	bool m_bad_lfoShapeData;
	SampleBuffer m_userWave;

	enum LfoShapes
	{
		SineWave,
		TriangleWave,
		SawWave,
		SquareWave,
		UserDefinedWave,
		RandomWave,
		NumLfoShapes
	} ;

	sample_t lfoShapeSample( fpp_t _frame_offset );
	void updateLfoShapeData();


	friend class EnvelopeAndLfoView;
	friend class FlpImport;

} ;

#endif
