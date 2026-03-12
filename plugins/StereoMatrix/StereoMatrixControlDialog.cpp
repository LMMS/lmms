/*
 * StereoMatrixControlDialog.cpp - control dialog for StereoMatrix effect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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


#include "StereoMatrixControlDialog.h"

#include <QHBoxLayout>
#include "embed.h"
#include "Knob.h"
#include "StereoMatrixControls.h"

namespace lmms::gui
{


StereoMatrixControlDialog::StereoMatrixControlDialog(StereoMatrixControls* controls) :
	EffectControlDialog(controls)
{
	QPalette pal;
	setAutoFillBackground(true);
	setFixedSize(160, 185);
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);

	auto layout = new QHBoxLayout(this);

	const auto makeKnob = [this, layout](
		FloatModel *model,
		const QString &txt_before
	) {
		auto k = new Knob(KnobType::Bright26, this);
		k->setModel(model);
		k->setHintText(txt_before, "");
		layout->addWidget(k, 0, Qt::AlignHCenter);
		return k;
	};

	makeKnob(&controls->m_llModel, tr("Left to Left Vol:"));
	makeKnob(&controls->m_lrModel, tr("Left to Right Vol:"));
	makeKnob(&controls->m_rlModel, tr("Right to Left Vol:"));
	makeKnob(&controls->m_rrModel, tr("Right to Right Vol:"));
}


} // namespace lmms::gui
