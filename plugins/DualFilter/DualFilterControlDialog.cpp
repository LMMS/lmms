/*
 * DualFilterControlDialog.cpp - control dialog for dual filter effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QLayout>
#include <QtGui/QStyleOption>
#include <QtGui/QPainter>
#include <QtGui/QGroupBox>

#include "DualFilterControlDialog.h"
#include "DualFilterControls.h"
#include "LedCheckbox.h"
#include "ComboBox.h"
#include "ToolTip.h"
#include "gui_templates.h"

#define makeknob( name, model, label, hint, unit, volume, container )   \
	Knob * name = new Knob( knobBright_26, this); 			            \
	name ->setModel( &controls-> model );					            \
	name ->setLabel( label );							                \
	name ->setHintText( hint, unit );                                   \
    name ->setVolumeKnob( volume );                                     \
    container ->addWidget(name);


DualFilterControlDialog::DualFilterControlDialog(DualFilterControls *controls, QWidget *_parent) :
	EffectControlDialog( controls , _parent)
{
    // filter1
    QHBoxLayout *mainLayout = new QHBoxLayout;
    QGroupBox *filter1GroupBox = new QGroupBox( this );
    QVBoxLayout *filter1GroupBoxLayout = new QVBoxLayout;
    QHBoxLayout *filter1KnobsLayout = new QHBoxLayout;
    filter1GroupBoxLayout->addLayout(filter1KnobsLayout);
    filter1GroupBox->setLayout(filter1GroupBoxLayout);
    mainLayout->addWidget(filter1GroupBox);

    makeknob( mixKnob, m_mixModel, tr( "MIX" ), tr( "Mix" ), "", false, mainLayout)

    // filter 2
    QGroupBox *filter2GroupBox = new QGroupBox( this );
    QVBoxLayout *filter2GroupBoxLayout = new QVBoxLayout;
    QHBoxLayout *filter2KnobsLayout = new QHBoxLayout;
    filter2GroupBoxLayout->addLayout(filter2KnobsLayout);
    filter2GroupBox->setLayout(filter2GroupBoxLayout);
    mainLayout->addWidget(filter2GroupBox);

    this->setLayout(mainLayout);

    // filter 1 controls
    makeknob( cut1Knob, m_cut1Model, tr( "FREQ" ), tr( "Cutoff frequency:" ), tr( "Hz" ), false, filter1KnobsLayout)
    makeknob( res1Knob, m_res1Model, tr( "RESO" ), tr( "Resonance:" ), tr( "" ), false, filter1KnobsLayout)
    makeknob( gain1Knob, m_gain1Model, tr( "GAIN" ), tr( "Gain:" ), tr( "%" ), true, filter1KnobsLayout)

    ComboBox * m_filter1ComboBox = new ComboBox( this );
    m_filter1ComboBox->setModel( &controls->m_filter1Model );
    filter1GroupBoxLayout->addWidget(m_filter1ComboBox);

    LedCheckBox * filter1Toggle = new LedCheckBox( "", this, tr( "Filter 1 enabled" ), LedCheckBox::Green );
	filter1Toggle -> move( 11,11 );
    filter1Toggle -> setModel( &controls -> m_enabled1Model );
    ToolTip::add( filter1Toggle, tr( "Click to enable/disable Filter 1" ) );

    // filter 2 controls
    makeknob( cut2Knob, m_cut2Model, tr( "FREQ" ), tr( "Cutoff frequency:" ), tr( "Hz" ), false, filter2KnobsLayout)
    makeknob( res2Knob, m_res2Model, tr( "RESO" ), tr( "Resonance:" ), tr( "" ), false, filter2KnobsLayout)
    makeknob( gain2Knob, m_gain2Model, tr( "GAIN" ), tr( "Gain:" ), tr( "%" ), true, filter2KnobsLayout)

    ComboBox * m_filter2ComboBox = new ComboBox( this );
    m_filter2ComboBox->setModel( &controls->m_filter2Model );
    filter2GroupBoxLayout->addWidget(m_filter2ComboBox);

    LedCheckBox * filter2Toggle = new LedCheckBox( "", this, tr( "Filter 2 enabled" ), LedCheckBox::Green );
	filter2Toggle -> move( 228, 11 );
    filter2Toggle -> setModel( &controls -> m_enabled2Model );
    ToolTip::add( filter2Toggle, tr( "Click to enable/disable Filter 2" ) );
}

void DualFilterControlDialog::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
