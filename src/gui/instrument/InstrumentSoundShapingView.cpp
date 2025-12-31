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

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "EnvelopeAndLfoParameters.h"
#include "EnvelopeAndLfoView.h"
#include "ComboBox.h"
#include "GroupBox.h"
#include "FontHelper.h"
#include "InstrumentSoundShaping.h"
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

	m_volumeView = new EnvelopeAndLfoView(m_targetsTabWidget);
	m_cutoffView = new EnvelopeAndLfoView(m_targetsTabWidget);
	m_resonanceView = new EnvelopeAndLfoView(m_targetsTabWidget);

	m_targetsTabWidget->addTab(m_volumeView, tr("VOLUME"), nullptr);
	m_targetsTabWidget->addTab(m_cutoffView, tr("CUTOFF"), nullptr);
	m_targetsTabWidget->addTab(m_resonanceView, tr("RESO"), nullptr);

	mainLayout->addWidget(m_targetsTabWidget, 1);


	m_filterGroupBox = new GroupBox(tr("FILTER"), this);
	QHBoxLayout* filterLayout = new QHBoxLayout(m_filterGroupBox);
	QMargins filterMargins = filterLayout->contentsMargins();
	filterMargins.setTop(18);
	filterLayout->setContentsMargins(filterMargins);

	m_filterComboBox = new ComboBox(m_filterGroupBox);
	filterLayout->addWidget(m_filterComboBox);

	m_filterCutKnob = new Knob(KnobType::Bright26, tr("FREQ"), m_filterGroupBox, Knob::LabelRendering::LegacyFixedFontSize);
	m_filterCutKnob->setHintText(tr("Cutoff frequency:"), " " + tr("Hz"));
	filterLayout->addWidget(m_filterCutKnob);

	m_filterResKnob = new Knob(KnobType::Bright26, tr("Q/RESO"), m_filterGroupBox, Knob::LabelRendering::LegacyFixedFontSize);
	m_filterResKnob->setHintText(tr("Q/Resonance:"), "");
	filterLayout->addWidget(m_filterResKnob);

	mainLayout->addWidget(m_filterGroupBox);


	m_singleStreamInfoLabel = new QLabel(tr("Envelopes, LFOs and filters are not supported by the current instrument."), this);
	m_singleStreamInfoLabel->setWordWrap(true);
	// TODO Could also be rendered in system font size...
	m_singleStreamInfoLabel->setFont(adjustedToPixelSize(m_singleStreamInfoLabel->font(), DEFAULT_FONT_SIZE));
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
	m_filterGroupBox->setModel(&m_ss->getFilterEnabledModel());
	m_filterComboBox->setModel(&m_ss->getFilterModel());
	m_filterCutKnob->setModel(&m_ss->getFilterCutModel());
	m_filterResKnob->setModel(&m_ss->getFilterResModel());

	m_volumeView->setModel(&m_ss->getVolumeParameters());
	m_cutoffView->setModel(&m_ss->getCutoffParameters());
	m_resonanceView->setModel(&m_ss->getResonanceParameters());
}





} // namespace lmms::gui
