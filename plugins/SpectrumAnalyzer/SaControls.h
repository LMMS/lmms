/*
 * SaControls.h - declaration of SaControls class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
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

#ifndef SACONTROLS_H
#define SACONTROLS_H

#include "EffectControls.h"
#include "SaSpectrumView.h"


class Analyzer;

class SaControls : public EffectControls
{
	Q_OBJECT
public:
	explicit SaControls(Analyzer* effect);
	virtual ~SaControls()
	{
	}

	virtual void saveSettings (QDomDocument& doc, QDomElement& parent);

	virtual void loadSettings (const QDomElement &_this);

	inline virtual QString nodeName() const
	{
		return "Analyzer";
	}

	virtual int controlCount()
	{
		return 5;
	}

	virtual EffectControlDialog* createView();

	SaProcessor m_fftBands;

	bool m_inProgress;
	bool visible();

private:
	Analyzer *m_effect;

	BoolModel m_stereo;
	BoolModel m_smooth;
	BoolModel m_waterfall;
	BoolModel m_log_x;
	BoolModel m_log_y;

	friend class SaControlsDialog;
	friend class Analyzer;
};
#endif // SACONTROLS_H
