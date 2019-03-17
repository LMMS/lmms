/*
 * Lv2Controls.h - controls for bassboosterx -effect
 *
 * Copyright (c) 2018-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

class Lv2Effect;


class Lv2FxControls : public EffectControls, public Lv2ControlBase
{
	Q_OBJECT
public:
	Lv2FxControls(Lv2Effect *effect, const QString &uri);
	~Lv2FxControls() override {}

	void saveSettings(QDomDocument &_doc, QDomElement &_parent) override;
	void loadSettings(const QDomElement &that) override;
	inline QString nodeName() const override
	{
		return Lv2ControlBase::nodeName();
	}

	int controlCount() override;
	EffectControlDialog *createView() override;

private slots:
	void changeControl();
	void reloadPlugin() { Lv2ControlBase::reloadPlugin(); }
	void updateLinkStatesFromGlobal() {
		Lv2ControlBase::updateLinkStatesFromGlobal(); }
	void linkPort(int id, bool state) { Lv2ControlBase::linkPort(id, state); }


private:
	DataFile::Types settingsType() override;
	void setNameFromFile(const QString &name) override;

	Lv2Effect *m_effect;
	friend class Lv2FxControlDialog;
	friend class Lv2Effect;
};

#endif
