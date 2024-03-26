/*
 * ClapFxControls.h - ClapFxControls implementation
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include <QTimer>

#include "ClapInstance.h"
#include "EffectControls.h"

namespace lmms
{

class ClapEffect;

namespace gui
{

class ClapFxControlDialog;

} // namespace gui


class ClapFxControls : public EffectControls
{
	Q_OBJECT
public:
	ClapFxControls(ClapEffect* effect, const std::string& uri);

	auto isValid() const -> bool;

	void reload();

	void saveSettings(QDomDocument& doc, QDomElement& elem) override;
	void loadSettings(const QDomElement& elem) override;
	auto nodeName() const -> QString override { return ClapInstance::ClapNodeName.data(); }

	auto controlCount() -> int override;
	auto createView() -> gui::EffectControlDialog* override;

signals:
	void modelChanged();

private slots:
	void changeControl();

private:
	std::unique_ptr<ClapInstance> m_instance;

	QTimer m_idleTimer;

	friend class gui::ClapFxControlDialog;
	friend class ClapEffect;
};

} // namespace lmms

#endif // LMMS_CLAP_FX_CONTROLS_H
