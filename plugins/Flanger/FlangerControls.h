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
#include "FlangerControlsDialog.h"

namespace lmms
{


class FlangerEffect;

class FlangerControls : public EffectControls
{
	Q_OBJECT
public:
	FlangerControls( FlangerEffect* effect );
	~FlangerControls() override = default;
	void saveSettings ( QDomDocument& doc, QDomElement& parent ) override;
	void loadSettings ( const QDomElement &_this ) override;
	inline QString nodeName() const override
	{
		return "Flanger";
	}
	int controlCount() override
	{
		return 7;
	}
	gui::EffectControlDialog* createView() override
	{
		return new gui::FlangerControlsDialog( this );
	}

private slots:
	void changedSampleRate();
	void changedPlaybackState();

private:
	FlangerEffect* m_effect;
	FloatModel m_delayTimeModel;
	TempoSyncKnobModel m_lfoFrequencyModel;
	FloatModel m_lfoAmountModel;
	FloatModel m_lfoPhaseModel;
	FloatModel m_feedbackModel;
	FloatModel m_whiteNoiseAmountModel;
	BoolModel m_invertFeedbackModel;

	friend class gui::FlangerControlsDialog;
	friend class FlangerEffect;

};


} // namespace lmms

#endif // FLANGERCONTROLS_H
