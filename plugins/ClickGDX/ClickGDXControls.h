/*
 * ClickGDXControls.h - controls for click remover effect
 *
 * Copyright (c) 2017
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

#ifndef CLICKGDX_CONTROLS_H
#define CLICKGDX_CONTROLS_H

#include "EffectControls.h"
#include "ClickGDXControlDialog.h"
#include "Knob.h"


class ClickGDXEffect;


class ClickGDXControls : public EffectControls
{
	Q_OBJECT
public:
	ClickGDXControls( ClickGDXEffect* effect );
	virtual ~ClickGDXControls()
	{
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return "ClickGDXControls";
	}

	virtual int controlCount()
	{
		return 4;
	}

	virtual EffectControlDialog* createView()
	{
		return new ClickGDXControlDialog( this );
	}


private slots:
	void changeControl();

private:
	ClickGDXEffect* m_effect;
	FloatModel m_attackTimeModel;
	FloatModel m_descentTimeModel;
	FloatModel m_attackTypeModel;   // don't use IntModel, Knob will break!
	FloatModel m_descentTypeModel;  // yes, the design of LMMS is a joke :(
	FloatModel m_attackTempoModel;
	FloatModel m_descentTempoModel;

	friend class ClickGDXControlDialog;
	friend class ClickGDXEffect;

} ;

#endif
