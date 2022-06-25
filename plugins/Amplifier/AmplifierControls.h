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

#ifndef AMPLIFIER_CONTROLS_H
#define AMPLIFIER_CONTROLS_H

#include "EffectControls.h"
#include "AmplifierControlDialog.h"

namespace lmms
{

class AmplifierEffect;

namespace gui
{
class AmplifierControlDialog;
}


class AmplifierControls : public EffectControls
{
	Q_OBJECT
public:
	AmplifierControls( AmplifierEffect* effect );
	~AmplifierControls() override = default;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return "AmplifierControls";
	}

	int controlCount() override
	{
		return 4;
	}

	gui::EffectControlDialog* createView() override
	{
		return new gui::AmplifierControlDialog( this );
	}


private slots:
	void changeControl();

private:
	AmplifierEffect* m_effect;
	FloatModel m_volumeModel;
	FloatModel m_panModel;
	FloatModel m_leftModel;
	FloatModel m_rightModel;

	friend class gui::AmplifierControlDialog;
	friend class AmplifierEffect;

} ;


} // namespace lmms

#endif
