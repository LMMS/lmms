/*
 * SpaControls.h - controls for bassboosterx -effect
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

#ifndef SPA_FX_CONTROLS_H
#define SPA_FX_CONTROLS_H

#include "EffectControls.h"
#include "SpaControlBase.h"

class SpaFxControls : public EffectControls, public SpaControlBase
{
	Q_OBJECT

	//DataFile::Types settingsType() override;
	void setNameFromFile(const QString &name) override;

public:
	SpaFxControls(class SpaEffect *effect, const QString &uniqueName);
	~SpaFxControls() override {}

	void saveSettings(QDomDocument &_doc, QDomElement &_parent) override;
	void loadSettings(const QDomElement &that) override;
	inline QString nodeName() const override
	{
		return SpaControlBase::nodeName();
	}

	int controlCount() override;

	EffectControlDialog *createView() override;

private slots:
	void changeControl();

	void reloadPlugin() { SpaControlBase::reloadPlugin(); }

private:
	class SpaEffect *m_effect;
	friend class SpaFxControlDialog;
	friend class SpaEffect;
};

#endif
