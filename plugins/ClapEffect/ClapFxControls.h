/*
 * ClapFxControls.h - ClapFxControls implementation
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_CLAP_FX_CONTROLS_H
#define LMMS_CLAP_FX_CONTROLS_H

#include "EffectControls.h"
#include "ClapControlBase.h"

namespace lmms
{


class ClapEffect;

namespace gui
{
class ClapFxControlDialog;
}


class ClapFxControls : public EffectControls, public ClapControlBase
{
	Q_OBJECT
signals:
	void modelChanged();
public:
	ClapFxControls(ClapEffect* effect, const QString& uri);
	void reload();

	void saveSettings(QDomDocument& _doc, QDomElement& _parent) override;
	void loadSettings(const QDomElement& that) override;
	inline auto nodeName() const -> QString override
	{
		return ClapControlBase::nodeName();
	}

	auto controlCount() -> int override;
	auto createView() -> gui::EffectControlDialog* override;

private slots:
	void changeControl();

private:
	auto settingsType() -> DataFile::Type override;
	void setNameFromFile(const QString& name) override;

	friend class gui::ClapFxControlDialog;
	friend class ClapEffect;
};


} // namespace lmms

#endif // LMMS_CLAP_FX_CONTROLS_H
