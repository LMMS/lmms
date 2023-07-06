/*
 * Lv2FxControls.h - Lv2FxControls implementation
 *
 * Copyright (c) 2018-2023 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#ifndef LV2_FX_CONTROLS_H
#define LV2_FX_CONTROLS_H

#include "EffectControls.h"
#include "Lv2ControlBase.h"

namespace lmms
{


class Lv2Effect;

namespace gui
{
class Lv2FxControlDialog;
}


class Lv2FxControls : public EffectControls, public Lv2ControlBase
{
	Q_OBJECT
signals:
	void modelChanged();
public:
	Lv2FxControls(Lv2Effect *effect, const QString &uri);
	void reload();

	void saveSettings(QDomDocument &_doc, QDomElement &_parent) override;
	void loadSettings(const QDomElement &that) override;
	inline QString nodeName() const override
	{
		return Lv2ControlBase::nodeName();
	}

	int controlCount() override;
	gui::EffectControlDialog* createView() override;

private slots:
	void changeControl();

private:
	void onSampleRateChanged();

	friend class gui::Lv2FxControlDialog;
	friend class Lv2Effect;
};


} // namespace lmms

#endif
