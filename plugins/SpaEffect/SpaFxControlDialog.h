/*
 * SpaControlDialog.h - control dialog for amplifier effect
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

#ifndef SPA_FX_CONTROL_DIALOG_H
#define SPA_FX_CONTROL_DIALOG_H

#include "EffectControlDialog.h"
#include "SpaViewBase.h"

class SpaFxControlDialog : public EffectControlDialog, public SpaViewBase
{
	Q_OBJECT

	class SpaFxControls *spaControls();
	void modelChanged() override;

public:
	SpaFxControlDialog(class SpaFxControls *controls);
	virtual ~SpaFxControlDialog() override {}

private slots:
	void toggleUI();
	void reloadPlugin();
};

#endif
