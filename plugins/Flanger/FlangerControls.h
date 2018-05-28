/*
 * flangercontrols.h - defination of StereoDelay class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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

#ifndef FLANGERCONTROLS_H
#define FLANGERCONTROLS_H

#include "EffectControls.h"
#include "Knob.h"
#include "FlangerControlsDialog.h"


class FlangerEffect;

class FlangerControls : public EffectControls
{
	Q_OBJECT
public:
	FlangerControls( FlangerEffect* effect );
	virtual ~FlangerControls()
	{
	}
	virtual void saveSettings ( QDomDocument& doc, QDomElement& parent );
	virtual void loadSettings ( const QDomElement &_this );
	inline virtual QString nodeName() const
	{
		return "Flanger";
	}
	virtual int controlCount()
	{
		return 5;
	}
	virtual EffectControlDialog* createView()
	{
		return new FlangerControlsDialog( this );
	}

private slots:
	void changedSampleRate();
	void changedPlaybackState();

private:
	FlangerEffect* m_effect;
	FloatModel m_delayTimeModel;
	TempoSyncKnobModel m_lfoFrequencyModel;
	FloatModel m_lfoAmountModel;
	FloatModel m_feedbackModel;
	FloatModel m_whiteNoiseAmountModel;
	BoolModel m_invertFeedbackModel;

	friend class FlangerControlsDialog;
	friend class FlangerEffect;

};

#endif // FLANGERCONTROLS_H
