/*
 * Lv2FxControlDialog.h - Lv2FxControlDialog implementation
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

#ifndef LV2_FX_CONTROL_DIALOG_H
#define LV2_FX_CONTROL_DIALOG_H

#include "EffectControlDialog.h"
#include "Lv2ViewBase.h"

namespace lmms
{

class Lv2FxControls;

namespace gui
{

class Lv2FxControlDialog : public EffectControlDialog, public Lv2ViewBase
{
	Q_OBJECT

public:
	Lv2FxControlDialog(Lv2FxControls *controls);

private:
	Lv2FxControls *lv2Controls();
	void modelChanged() final;
	void hideEvent(QHideEvent *event) override;
};


} // namespace gui

} // namespace lmms

#endif
