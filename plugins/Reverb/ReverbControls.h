/*
 * ReverbControls.h 
 *
 * Copyright (c) 2017 Paul Batchelor
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

#ifndef Reverb_CONTROLS_H
#define Reverb_CONTROLS_H

#include "EffectControls.h"
#include "ReverbControlDialog.h"


namespace lmms
{


class ReverbEffect;

class ReverbControls : public EffectControls
{
	Q_OBJECT
public:
	ReverbControls( ReverbEffect* effect );
	~ReverbControls() override = default;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return "ReverbControls";
	}

	int controlCount() override
	{
		return 4;
	}

	gui::EffectControlDialog* createView() override
	{
		return new gui::ReverbControlDialog( this );
	}
	float m_inPeakL;
	float m_inPeakR;
	float m_outPeakL;
	float m_outPeakR;


private slots:
	void changeControl();
	void changeSampleRate();

private:
	ReverbEffect* m_effect;
	FloatModel m_inputGainModel;
	FloatModel m_sizeModel;
	FloatModel m_colorModel;
	FloatModel m_outputGainModel;

	friend class gui::ReverbControlDialog;
	friend class ReverbEffect;

} ;


} // namespace lmms

#endif
