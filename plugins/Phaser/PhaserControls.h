/*
 * PhaserControls.h
 *
 * Copyright (c) 2019 Lost Robot <r94231@gmail.com>
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

#ifndef PHASER_CONTROLS_H
#define PHASER_CONTROLS_H

#include "EffectControls.h"
#include "PhaserControlDialog.h"
#include "Knob.h"
#include "LcdSpinBox.h"


class PhaserEffect;


class PhaserControls : public EffectControls
{
	Q_OBJECT
public:
	PhaserControls( PhaserEffect* effect );
	virtual ~PhaserControls();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return "PhaserControls";
	}

	virtual int controlCount()
	{
		return 10;
	}

	virtual EffectControlDialog * createView()
	{
		m_pluginView = new PhaserControlDialog( this );
		return m_pluginView;
	}

	float m_inPeakL;
	float m_inPeakR;
	float m_outPeakL;
	float m_outPeakR;

private slots:
	void changedSampleRate();
	void changedPlaybackState();
	void updatePhase();

private:
	PhaserEffect* m_effect;

	FloatModel m_cutoffModel;
	FloatModel m_resonanceModel;
	FloatModel m_feedbackModel;
	IntModel m_orderModel;
	FloatModel m_delayModel;
	TempoSyncKnobModel m_rateModel;
	FloatModel m_amountModel;
	BoolModel m_enableLFOModel;
	FloatModel m_phaseModel;
	FloatModel m_wetDryModel;
	FloatModel m_inFollowModel;
	FloatModel m_attackModel;
	FloatModel m_releaseModel;
	FloatModel m_outGainModel;
	FloatModel m_inGainModel;

	PhaserControlDialog * m_pluginView;

	friend class PhaserControlDialog;
	friend class PhaserEffect;

} ;

#endif
