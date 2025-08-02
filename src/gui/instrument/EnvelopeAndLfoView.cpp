/*
 * EnvelopeAndLfoView.cpp - widget which is m_used by envelope/lfo/filter-
 *                          tab of instrument track window
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

#include "EnvelopeAndLfoView.h"

#include <string_view>

#include <QSizePolicy>
#include <QVBoxLayout>

#include "EnvelopeGraph.h"
#include "LfoGraph.h"
#include "EnvelopeAndLfoParameters.h"
#include "SampleLoader.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "DataFile.h"
#include "PixmapButton.h"
#include "StringPairDrag.h"
#include "TempoSyncKnob.h"
#include "TextFloat.h"
#include "Track.h"

namespace lmms
{

namespace gui
{

EnvelopeAndLfoView::EnvelopeAndLfoView(QWidget * parent) :
	QWidget(parent),
	ModelView(nullptr, this),
	m_params(nullptr)
{
	// Helper lambdas for consistent repeated buiding of certain widgets
	auto buildKnob = [&](const QString& label, const QString& hintText)
	{
		auto knob = new Knob(KnobType::Bright26, label, this, Knob::LabelRendering::LegacyFixedFontSize);
		knob->setHintText(hintText, "");
		
		return knob;
	};

	auto buildPixmapButton = [&](std::string_view activePixmap, std::string_view inactivePixmap)
	{
		auto button = new PixmapButton(this, nullptr);
		button->setActiveGraphic(embed::getIconPixmap(activePixmap));
		button->setInactiveGraphic(embed::getIconPixmap(inactivePixmap));

		return button;
	};

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(5, 5, 5, 5);

	// Envelope

	QVBoxLayout* envelopeLayout = new QVBoxLayout();
	mainLayout->addLayout(envelopeLayout);

	QHBoxLayout* graphAndAmountLayout = new QHBoxLayout();
	envelopeLayout->addLayout(graphAndAmountLayout);

	m_envelopeGraph = new EnvelopeGraph(this);
	graphAndAmountLayout->addWidget(m_envelopeGraph);
	m_envelopeGraph->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	m_amountKnob = buildKnob(tr("AMT"), tr("Modulation amount:"));
	graphAndAmountLayout->addWidget(m_amountKnob, 0, Qt::AlignCenter);

	QHBoxLayout* envKnobsLayout = new QHBoxLayout();
	envelopeLayout->addLayout(envKnobsLayout);

	m_predelayKnob = buildKnob(tr("DEL"), tr("Pre-delay:"));
	envKnobsLayout->addWidget(m_predelayKnob);

	m_attackKnob = buildKnob(tr("ATT"), tr("Attack:"));
	envKnobsLayout->addWidget(m_attackKnob);

	m_holdKnob = buildKnob(tr("HOLD"), tr("Hold:"));
	envKnobsLayout->addWidget(m_holdKnob);

	m_decayKnob = buildKnob(tr("DEC"), tr("Decay:"));
	envKnobsLayout->addWidget(m_decayKnob);

	m_sustainKnob = buildKnob(tr("SUST"), tr("Sustain:"));
	envKnobsLayout->addWidget(m_sustainKnob);

	m_releaseKnob = buildKnob(tr("REL"), tr("Release:"));
	envKnobsLayout->addWidget(m_releaseKnob);


	// Add some space between the envelope and LFO section
	mainLayout->addSpacing(10);


	// LFO

	QHBoxLayout* lfoLayout = new QHBoxLayout();
	mainLayout->addLayout(lfoLayout);

	QVBoxLayout* graphAndTypesLayout = new QVBoxLayout();
	lfoLayout->addLayout(graphAndTypesLayout);

	m_lfoGraph = new LfoGraph(this);
	m_lfoGraph->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	graphAndTypesLayout->addWidget(m_lfoGraph);

	QHBoxLayout* typesLayout = new QHBoxLayout();
	graphAndTypesLayout->addLayout(typesLayout);
	typesLayout->setContentsMargins(0, 0, 0, 0);
	typesLayout->setSpacing(0);

	auto sin_lfo_btn = buildPixmapButton("sin_wave_active", "sin_wave_inactive");
	auto triangle_lfo_btn = buildPixmapButton("triangle_wave_active", "triangle_wave_inactive");
	auto saw_lfo_btn = buildPixmapButton("saw_wave_active", "saw_wave_inactive");
	auto sqr_lfo_btn = buildPixmapButton("square_wave_active","square_wave_inactive");
	auto random_lfo_btn = buildPixmapButton("random_wave_active", "random_wave_inactive");
	m_userLfoBtn = buildPixmapButton("usr_wave_active", "usr_wave_inactive");

	connect(m_userLfoBtn, SIGNAL(toggled(bool)), this, SLOT(lfoUserWaveChanged()));

	typesLayout->addWidget(sin_lfo_btn);
	typesLayout->addWidget(triangle_lfo_btn);
	typesLayout->addWidget(saw_lfo_btn);
	typesLayout->addWidget(sqr_lfo_btn);
	typesLayout->addWidget(random_lfo_btn);
	typesLayout->addWidget(m_userLfoBtn);

	m_lfoWaveBtnGrp = new AutomatableButtonGroup(this);
	m_lfoWaveBtnGrp->addButton(sin_lfo_btn);
	m_lfoWaveBtnGrp->addButton(triangle_lfo_btn);
	m_lfoWaveBtnGrp->addButton(saw_lfo_btn);
	m_lfoWaveBtnGrp->addButton(sqr_lfo_btn);
	m_lfoWaveBtnGrp->addButton(m_userLfoBtn);
	m_lfoWaveBtnGrp->addButton(random_lfo_btn);

	QVBoxLayout* knobsAndCheckBoxesLayout  = new QVBoxLayout();
	lfoLayout->addLayout(knobsAndCheckBoxesLayout);

	QHBoxLayout* lfoKnobsLayout = new QHBoxLayout();
	knobsAndCheckBoxesLayout->addLayout(lfoKnobsLayout);

	m_lfoPredelayKnob = buildKnob(tr("DEL"), tr("Pre-delay:"));
	lfoKnobsLayout->addWidget(m_lfoPredelayKnob);

	m_lfoAttackKnob = buildKnob(tr("ATT"), tr("Attack:"));
	lfoKnobsLayout->addWidget(m_lfoAttackKnob);

	m_lfoSpeedKnob = new TempoSyncKnob(KnobType::Bright26, tr("SPD"), this, Knob::LabelRendering::LegacyFixedFontSize);
	m_lfoSpeedKnob->setHintText(tr("Frequency:"), "");
	lfoKnobsLayout->addWidget(m_lfoSpeedKnob);

	m_lfoAmountKnob = buildKnob(tr("AMT"), tr("Modulation amount:"));
	lfoKnobsLayout->addWidget(m_lfoAmountKnob);

	QVBoxLayout* checkBoxesLayout = new QVBoxLayout();
	knobsAndCheckBoxesLayout->addLayout(checkBoxesLayout);

	m_x100Cb = new LedCheckBox(tr("FREQ x 100"), this);
	m_x100Cb->setToolTip(tr("Multiply LFO frequency by 100"));
	checkBoxesLayout->addWidget(m_x100Cb);

	m_controlEnvAmountCb = new LedCheckBox(tr("MOD ENV AMOUNT"), this);
	m_controlEnvAmountCb->setToolTip(tr("Control envelope amount by this LFO"));
	checkBoxesLayout->addWidget(m_controlEnvAmountCb);

	setAcceptDrops(true);
}




EnvelopeAndLfoView::~EnvelopeAndLfoView()
{
	delete m_lfoWaveBtnGrp;
}




void EnvelopeAndLfoView::modelChanged()
{
	m_params = castModel<EnvelopeAndLfoParameters>();
	m_envelopeGraph->setModel(m_params);
	m_predelayKnob->setModel( &m_params->m_predelayModel );
	m_attackKnob->setModel( &m_params->m_attackModel );
	m_holdKnob->setModel( &m_params->m_holdModel );
	m_decayKnob->setModel( &m_params->m_decayModel );
	m_sustainKnob->setModel( &m_params->m_sustainModel );
	m_releaseKnob->setModel( &m_params->m_releaseModel );
	m_amountKnob->setModel( &m_params->m_amountModel );

	m_lfoGraph->setModel(m_params);
	m_lfoPredelayKnob->setModel( &m_params->m_lfoPredelayModel );
	m_lfoAttackKnob->setModel( &m_params->m_lfoAttackModel );
	m_lfoSpeedKnob->setModel( &m_params->m_lfoSpeedModel );
	m_lfoAmountKnob->setModel( &m_params->m_lfoAmountModel );
	m_lfoWaveBtnGrp->setModel( &m_params->m_lfoWaveModel );
	m_x100Cb->setModel( &m_params->m_x100Model );
	m_controlEnvAmountCb->setModel( &m_params->m_controlEnvAmountModel );
}




void EnvelopeAndLfoView::dragEnterEvent( QDragEnterEvent * _dee )
{
	StringPairDrag::processDragEnterEvent( _dee,
					{"samplefile", QString("clip_%1").arg(static_cast<int>(Track::Type::Sample)) });
}




void EnvelopeAndLfoView::dropEvent( QDropEvent * _de )
{
	QString type = StringPairDrag::decodeKey( _de );
	QString value = StringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		m_params->m_userWave = SampleLoader::createBufferFromFile(value);
		m_userLfoBtn->model()->setValue( true );
		m_params->m_lfoWaveModel.setValue(static_cast<int>(EnvelopeAndLfoParameters::LfoShape::UserDefinedWave));
		_de->accept();
		update();
	}
	else if( type == QString( "clip_%1" ).arg( static_cast<int>(Track::Type::Sample) ) )
	{
		DataFile dataFile( value.toUtf8() );
		auto file = dataFile.content().
					firstChildElement().firstChildElement().
					firstChildElement().attribute("src");
		m_params->m_userWave = SampleLoader::createBufferFromFile(file);
		m_userLfoBtn->model()->setValue( true );
		m_params->m_lfoWaveModel.setValue(static_cast<int>(EnvelopeAndLfoParameters::LfoShape::UserDefinedWave));
		_de->accept();
		update();
	}
}




void EnvelopeAndLfoView::lfoUserWaveChanged()
{
	if( static_cast<EnvelopeAndLfoParameters::LfoShape>(m_params->m_lfoWaveModel.value()) ==
				EnvelopeAndLfoParameters::LfoShape::UserDefinedWave )
	{
		if (m_params->m_userWave->size() <= 1)
		{
			TextFloat::displayMessage( tr( "Hint" ),
				tr( "Drag and drop a sample into this window." ),
					embed::getIconPixmap( "hint" ), 3000 );
		}
	}
}

} // namespace gui

} // namespace lmms
