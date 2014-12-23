/*
 * delaycontrols.h - declaration of DelayControl class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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

#ifndef DELAYCONTROLS_H
#define DELAYCONTROLS_H

#include "EffectControls.h"
#include "knob.h"
#include "delaycontrolsdialog.h"



class DelayEffect;

class DelayControls : public EffectControls
{
	Q_OBJECT
public:
	DelayControls( DelayEffect* effect );
	virtual ~DelayControls()
	{
	}
	virtual void saveSettings( QDomDocument& doc, QDomElement& parent );
	virtual void loadSettings( const QDomElement& _this );
	inline virtual QString nodeName() const
	{
		return "Delay";
	}
	virtual int controlCount(){
		return 4;
	}
	virtual EffectControlDialog* createView()
	{
		return new DelayControlsDialog( this );
	}

private slots:
	void changeSampleRate();

private:
	DelayEffect* m_effect;
	TempoSyncKnobModel m_delayTimeModel;
	FloatModel m_feedbackModel;
	TempoSyncKnobModel m_lfoTimeModel;
	TempoSyncKnobModel m_lfoAmountModel;

	friend class DelayControlsDialog;
	friend class DelayEffect;
};

#endif // DELAYCONTROLS_H
