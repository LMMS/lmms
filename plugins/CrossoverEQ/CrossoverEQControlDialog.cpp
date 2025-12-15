/*
 * CrossoverEQControlDialog.cpp - A native 4-band Crossover Equalizer 
 * good for simulating tonestacks or simple peakless (flat-band) equalization
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 

#include "AutomatableButton.h"
#include "CrossoverEQControlDialog.h"
#include "CrossoverEQControls.h"
#include "embed.h"
#include "FontHelper.h"
#include "Knob.h"
#include "Fader.h"

#include <QHBoxLayout>
#include <QVBoxLayout>


namespace lmms::gui
{


CrossoverEQControlDialog::CrossoverEQControlDialog(CrossoverEQControls *controls) :
	EffectControlDialog(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(167, 218);
	auto layout = new QVBoxLayout(this);

	auto knobsLayout = new QHBoxLayout();
	layout->addLayout(knobsLayout);

	const auto makeKnob = [this, knobsLayout](
		FloatModel* model,
		const QString& label,
		const QString& txt_before
	) {
		auto k = new Knob(KnobType::Bright26, label, SMALL_FONT_SIZE, this);
		k->setModel(model);
		k->setHintText(txt_before, "Hz");
		knobsLayout->addWidget(k, 0, Qt::AlignHCenter);
	};

	makeKnob(&controls->m_xover12, "1/2", tr("Band 1/2 crossover"));
	makeKnob(&controls->m_xover23, "2/3", tr("Band 2/3 crossover"));
	makeKnob(&controls->m_xover34, "3/4", tr("Band 3/4 crossover"));
	
	auto bandsLayout = new QGridLayout();
	bandsLayout->setContentsMargins(4, 10, 4, 5);
	layout->addLayout(bandsLayout);

	const auto makeFader = [this, bandsLayout](
		FloatModel* model,
		const QString& label,
		int column
	) {
		auto f = new Fader(model, label, this, false);
		f->setHintText(label, "dBFS");
		f->setDisplayConversion(false);
		f->setRenderUnityLine(false);
		bandsLayout->addWidget(f, 0, column, Qt::AlignHCenter);
	};

	makeFader(&controls->m_gain1, tr("Band 1 gain"), 0);
	makeFader(&controls->m_gain2, tr("Band 2 gain"), 1);
	makeFader(&controls->m_gain3, tr("Band 3 gain"), 2);
	makeFader(&controls->m_gain4, tr("Band 4 gain"), 3);

	const auto makeMuteBtn = [this, bandsLayout](
		BoolModel* model,
		const QString& label,
		int column
	) {
		auto b = new AutomatableButton(this, label);
		b->setCheckable(true);
		b->setModel(model);
		b->setToolTip(label);
		b->setObjectName("btn-mute-inv");
		bandsLayout->addWidget(b, 1, column, Qt::AlignCenter);
	};

	makeMuteBtn(&controls->m_mute1, tr("Mute band 1"), 0);
	makeMuteBtn(&controls->m_mute2, tr("Mute band 2"), 1);
	makeMuteBtn(&controls->m_mute3, tr("Mute band 3"), 2);
	makeMuteBtn(&controls->m_mute4, tr("Mute band 4"), 3);
}


} // namespace lmms::gui
