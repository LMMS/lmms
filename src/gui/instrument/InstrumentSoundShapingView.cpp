/*
 * InstrumentSoundShapingView.cpp - view for InstrumentSoundShaping class
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "InstrumentSoundShapingView.h"

#include <QLabel>
#include <QBoxLayout>

#include "EnvelopeAndLfoParameters.h"
#include "EnvelopeAndLfoView.h"
#include "ComboBox.h"
#include "GroupBox.h"
#include "gui_templates.h"
#include "Knob.h"
#include "TabWidget.h"


namespace lmms::gui
{

InstrumentSoundShapingView::InstrumentSoundShapingView(QWidget* parent) :
	QWidget(parent),
	ModelView(nullptr, this)
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(5, 5, 5, 5);

	m_targetsTabWidget = new TabWidget(tr("TARGET"), this);

	for (auto i = std::size_t{0}; i < InstrumentSoundShaping::NumTargets; ++i)
	{
		m_envLfoViews[i] = new EnvelopeAndLfoView(m_targetsTabWidget);
		m_targetsTabWidget->addTab(m_envLfoViews[i],
			tr(InstrumentSoundShaping::targetNames[i][0]), nullptr);
	}

	mainLayout->addWidget(m_targetsTabWidget, 1);


	m_filterGroupBox = new GroupBox(tr("FILTER"), this);
	QHBoxLayout* filterLayout = new QHBoxLayout(m_filterGroupBox);
	QMargins filterMargins = filterLayout->contentsMargins();
	filterMargins.setTop(18);
	filterLayout->setContentsMargins(filterMargins);

	m_filterComboBox = new ComboBox(m_filterGroupBox);
	filterLayout->addWidget(m_filterComboBox);

	m_filterCutKnob = new Knob(KnobType::Bright26, m_filterGroupBox);
	m_filterCutKnob->setLabel(tr("FREQ"));
	m_filterCutKnob->setHintText(tr("Cutoff frequency:"), " " + tr("Hz"));
	filterLayout->addWidget(m_filterCutKnob);

	m_filterResKnob = new Knob(KnobType::Bright26, m_filterGroupBox);
	m_filterResKnob->setLabel(tr("Q/RESO"));
	m_filterResKnob->setHintText(tr("Q/Resonance:"), "");
	filterLayout->addWidget(m_filterResKnob);

	mainLayout->addWidget(m_filterGroupBox);


	m_singleStreamInfoLabel = new QLabel(tr("Envelopes, LFOs and filters are not supported by the current instrument."), this);
	m_singleStreamInfoLabel->setWordWrap(true);
	// TODO Could also be rendered in system font size...
	m_singleStreamInfoLabel->setFont(adjustedToPixelSize(m_singleStreamInfoLabel->font(), 10));
	m_singleStreamInfoLabel->setFixedWidth(242);

	mainLayout->addWidget(m_singleStreamInfoLabel, 0, Qt::AlignTop);
}




InstrumentSoundShapingView::~InstrumentSoundShapingView()
{
	delete m_targetsTabWidget;
}



void InstrumentSoundShapingView::setFunctionsHidden( bool hidden )
{
	m_targetsTabWidget->setHidden( hidden );
	m_filterGroupBox->setHidden( hidden );
	m_singleStreamInfoLabel->setHidden( !hidden );
}



void InstrumentSoundShapingView::modelChanged()
{
	m_ss = castModel<InstrumentSoundShaping>();
	m_filterGroupBox->setModel( &m_ss->m_filterEnabledModel );
	m_filterComboBox->setModel( &m_ss->m_filterModel );
	m_filterCutKnob->setModel( &m_ss->m_filterCutModel );
	m_filterResKnob->setModel( &m_ss->m_filterResModel );
	for (auto i = std::size_t{0}; i < InstrumentSoundShaping::NumTargets; ++i)
	{
		m_envLfoViews[i]->setModel( m_ss->m_envLfoParameters[i] );
	}
}





} // namespace lmms::gui
