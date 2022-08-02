/*
 * DualFilterControls.h - controls for dual filter -effect
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

#ifndef DUALFILTER_CONTROLS_H
#define DUALFILTER_CONTROLS_H

#include "EffectControls.h"
#include "DualFilterControlDialog.h"
#include "ComboBoxModel.h"

namespace lmms
{


class DualFilterEffect;


class DualFilterControls : public EffectControls
{
	Q_OBJECT
public:
	DualFilterControls( DualFilterEffect* effect );
	~DualFilterControls() override = default;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return "DualFilterControls";
	}

	int controlCount() override
	{
		return 11;
	}

	gui::EffectControlDialog* createView() override
	{
		return new gui::DualFilterControlDialog( this );
	}


private slots:
	void updateFilters();
	
private:
	DualFilterEffect* m_effect;

	BoolModel m_enabled1Model;
	ComboBoxModel m_filter1Model;
	FloatModel m_cut1Model;
	FloatModel m_res1Model;
	FloatModel m_gain1Model;

	FloatModel m_mixModel;

	BoolModel m_enabled2Model;
	ComboBoxModel m_filter2Model;
	FloatModel m_cut2Model;
	FloatModel m_res2Model;
	FloatModel m_gain2Model;

	friend class gui::DualFilterControlDialog;
	friend class DualFilterEffect;

} ;


} // namespace lmms

#endif
