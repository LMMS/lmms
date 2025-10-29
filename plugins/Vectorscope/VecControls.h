/*
 * VecControls.h - declaration of VecControls class.
 *
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

#ifndef VECCONTROLS_H
#define VECCONTROLS_H


#include "EffectControls.h"

namespace lmms
{


class Vectorscope;

namespace gui
{
class VecControlsDialog;
}

// Holds all the configuration values
class VecControls : public EffectControls
{
	Q_OBJECT
public:
	explicit VecControls(Vectorscope *effect);
	~VecControls() override = default;

	gui::EffectControlDialog* createView() override;

	void saveSettings (QDomDocument &document, QDomElement &element) override;
	void loadSettings (const QDomElement &element) override;

	QString nodeName() const override {return "Vectorscope";}
	int controlCount() override {return 3;}

	const BoolModel& getLogarithmicModel() const { return m_logarithmicModel; }
	const BoolModel& getLinesModel() const { return m_linesModeModel; }

private:
	Vectorscope *m_effect;

	BoolModel m_logarithmicModel;
	BoolModel m_linesModeModel;

	friend class gui::VecControlsDialog;
};


} // namespace lmms

#endif // VECCONTROLS_H
