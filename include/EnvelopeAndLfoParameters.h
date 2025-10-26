/*
 * EnvelopeAndLfoParameters.h - class EnvelopeAndLfoParameters
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

#ifndef LMMS_ENVELOPE_AND_LFO_PARAMETERS_H
#define LMMS_ENVELOPE_AND_LFO_PARAMETERS_H

#include <memory>

#include "JournallingObject.h"
#include "AutomatableModel.h"
#include "SampleBuffer.h"
#include "TempoSyncKnobModel.h"
#include "LmmsTypes.h"

namespace lmms
{

namespace gui
{

class EnvelopeAndLfoView;

}

class LMMS_EXPORT EnvelopeAndLfoParameters : public Model, public JournallingObject
{
	Q_OBJECT
public:
	class LfoInstances
	{
	public:
		LfoInstances() = default;

		~LfoInstances() = default;

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
		using LfoList = QList<EnvelopeAndLfoParameters*>;
		LfoList m_lfos;

	};

	enum class LfoShape
	{
		SineWave,
		TriangleWave,
		SawWave,
		SquareWave,
		UserDefinedWave,
		RandomWave,
		Count
	};

	EnvelopeAndLfoParameters( float _value_for_zero_amount,
							Model * _parent );
	~EnvelopeAndLfoParameters() override;

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


	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	QString nodeName() const override
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

	// Envelope
	const FloatModel& getPredelayModel() const { return m_predelayModel; }
	const FloatModel& getAttackModel() const { return m_attackModel; }
	const FloatModel& getHoldModel() const { return m_holdModel; }
	const FloatModel& getDecayModel() const { return m_decayModel; }
	const FloatModel& getSustainModel() const { return m_sustainModel; }
	const FloatModel& getReleaseModel() const { return m_releaseModel; }
	const FloatModel& getAmountModel() const { return m_amountModel; }
	FloatModel& getAmountModel() { return m_amountModel; }


	// LFO
	inline f_cnt_t getLfoPredelayFrames() const { return m_lfoPredelayFrames; }
	inline f_cnt_t getLfoAttackFrames() const { return m_lfoAttackFrames; }
	inline f_cnt_t getLfoOscillationFrames() const { return m_lfoOscillationFrames; }

	const FloatModel& getLfoAmountModel() const { return m_lfoAmountModel; }
	FloatModel& getLfoAmountModel() { return m_lfoAmountModel; }
	const TempoSyncKnobModel& getLfoSpeedModel() const { return m_lfoSpeedModel; }
	const BoolModel& getX100Model() const { return m_x100Model; }
	const IntModel& getLfoWaveModel() const { return m_lfoWaveModel; }
	std::shared_ptr<const SampleBuffer> getLfoUserWave() const { return m_userWave; }

public slots:
	void updateSampleVars();


protected:
	void fillLfoLevel( float * _buf, f_cnt_t _frame, const fpp_t _frames );


private:
	static LfoInstances * s_lfoInstances;
	bool m_used;

	QMutex m_paramMutex;

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
	f_cnt_t m_pahdBufSize;
	f_cnt_t m_rBufSize;


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
	std::shared_ptr<const SampleBuffer> m_userWave = SampleBuffer::emptyBuffer();

	constexpr static auto NumLfoShapes = static_cast<std::size_t>(LfoShape::Count);

	sample_t lfoShapeSample( fpp_t _frame_offset );
	void updateLfoShapeData();


	friend class gui::EnvelopeAndLfoView;

} ;

} // namespace lmms

#endif // LMMS_ENVELOPE_AND_LFO_PARAMETERS_H
