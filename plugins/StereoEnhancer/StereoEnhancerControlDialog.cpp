/*
 * StereoEnhancerControlDialog.cpp - control-dialog for StereoEnhancer effect
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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




#include "StereoEnhancerControlDialog.h"
#include <QHBoxLayout>
#include "Knob.h"
#include "StereoEnhancerControls.h"

namespace lmms::gui
{


StereoEnhancerControlDialog::StereoEnhancerControlDialog(
	StereoEnhancerControls * _controls ) :
	EffectControlDialog( _controls )
{
	auto l = new QHBoxLayout(this);

	auto widthKnob = new Knob(KnobType::Bright26, tr("WIDTH"), this);
	widthKnob->setModel( &_controls->m_widthModel );
	widthKnob->setHintText( tr( "Width:" ) , " samples" );

	l->addWidget( widthKnob );

	this->setLayout(l);
}


} // namespace lmms::gui
