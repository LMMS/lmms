/*
 * DispersionControls.h
 *
 * Copyright (c) 2023 Lost Robot <r94231/at/gmail/dot/com>
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

#ifndef LMMS_DISPERSION_CONTROLS_H
#define LMMS_DISPERSION_CONTROLS_H

#include "DispersionControlDialog.h"
#include "EffectControls.h"

namespace lmms
{

class DispersionEffect;

class DispersionControls : public EffectControls
{
	Q_OBJECT
public:
	DispersionControls(DispersionEffect* effect);
	~DispersionControls() override = default;

	void saveSettings(QDomDocument & doc, QDomElement & parent) override;
	void loadSettings(const QDomElement & parent) override;
	inline QString nodeName() const override
	{
		return "DispersionControls";
	}

	int controlCount() override
	{
		return 5;
	}

	gui::EffectControlDialog* createView() override
	{
		return new gui::DispersionControlDialog(this);
	}

private:
	DispersionEffect* m_effect;
	IntModel m_amountModel;
	FloatModel m_freqModel;
	FloatModel m_resoModel;
	FloatModel m_feedbackModel;
	BoolModel m_dcModel;

	friend class gui::DispersionControlDialog;
	friend class DispersionEffect;
};


} // namespace lmms

#endif // LMMS_DISPERSION_CONTROLS_H
