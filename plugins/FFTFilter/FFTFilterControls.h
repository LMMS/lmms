/*
 * AmplifierControls.h - controls for bassboosterx -effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_FFTFILTER_CONTROLS_H
#define LMMS_FFTFILTER_CONTROLS_H

#include "EffectControls.h"
#include "FFTFilterControlDialog.h"
#include "VectorGraph.h"

namespace lmms
{

class FFTFilterEffect;

namespace gui
{
class FFTFilterControlDialog;
}

class FFTFilterControls : public EffectControls
{
	Q_OBJECT
public:
	FFTFilterControls(FFTFilterEffect* effect);
	~FFTFilterControls() override = default;

	void saveSettings(QDomDocument& doc, QDomElement& parent) override;
	void loadSettings(const QDomElement& parent) override;
	inline QString nodeName() const override
	{
		return "FFTFilterControls";
	}
	gui::EffectControlDialog* createView() override
	{
		return new gui::FFTFilterControlDialog(this);
	}
	int controlCount() override { return 6; }

private slots:
	void resetClicked();

private:
	FFTFilterEffect* m_effect;
	FloatModel m_volumeModel;
	//FloatModel m_panModel;
	//FloatModel m_leftModel;
	//FloatModel m_rightModel;
	FloatModel m_freqControlModel;
	FloatModel m_effectControlModel;
	IntModel m_bufferModel;
	
	BoolModel m_displayFFTModel;
	BoolModel m_resetModel;

	VectorGraphModel m_graphModel;

	//graphModel m_filterModel;

	friend class gui::FFTFilterControlDialog;
	friend class FFTFilterEffect;
};

} // namespace lmms

#endif // LMMS_FFTFILTER_CONTROLS_H
