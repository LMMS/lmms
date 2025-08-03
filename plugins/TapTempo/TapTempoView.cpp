/*
 * TapTempoView.cpp - Plugin to count beats per minute
 *
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
 *
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
#include "TapTempoView.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "Engine.h"
#include "FontHelper.h"
#include "SamplePlayHandle.h"
#include "Song.h"
#include "TapTempo.h"

namespace lmms::gui {
TapTempoView::TapTempoView(TapTempo* plugin)
	: ToolPluginView(plugin)
	, m_plugin(plugin)
{
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	auto font = QFont();

	m_tapButton = new QPushButton();
	m_tapButton->setFixedSize(200, 200);
	m_tapButton->setFont(adjustedToPixelSize(font, 32));
	m_tapButton->setText(tr("0"));

	auto precisionCheckBox = new QCheckBox(tr("Precision"));
	precisionCheckBox->setFocusPolicy(Qt::NoFocus);
	precisionCheckBox->setToolTip(tr("Display in high precision"));
	precisionCheckBox->setText(tr("Precision"));

	auto muteCheckBox = new QCheckBox(tr("0.0 ms"));
	muteCheckBox->setFocusPolicy(Qt::NoFocus);
	muteCheckBox->setToolTip(tr("Mute metronome"));
	muteCheckBox->setText(tr("Mute"));

	m_msLabel = new QLabel();
	m_msLabel->setFocusPolicy(Qt::NoFocus);
	m_msLabel->setToolTip(tr("BPM in milliseconds"));
	m_msLabel->setText(tr("0 ms"));

	m_hzLabel = new QLabel();
	m_hzLabel->setFocusPolicy(Qt::NoFocus);
	m_hzLabel->setToolTip(tr("Frequency of BPM"));
	m_hzLabel->setText(tr("0.0000 hz"));

	auto resetButton = new QPushButton(tr("Reset"));
	resetButton->setFocusPolicy(Qt::NoFocus);
	resetButton->setToolTip(tr("Reset counter and sidebar information"));

	auto syncButton = new QPushButton(tr("Sync"));
	syncButton->setFocusPolicy(Qt::NoFocus);
	syncButton->setToolTip(tr("Sync with project tempo"));

	auto optionLayout = new QVBoxLayout();
	optionLayout->addWidget(precisionCheckBox);
	optionLayout->addWidget(muteCheckBox);

	auto bpmInfoLayout = new QVBoxLayout();
	bpmInfoLayout->addWidget(m_msLabel, 0, Qt::AlignHCenter);
	bpmInfoLayout->addWidget(m_hzLabel, 0, Qt::AlignHCenter);

	auto sidebarLayout = new QHBoxLayout();
	sidebarLayout->addLayout(optionLayout);
	sidebarLayout->addLayout(bpmInfoLayout);

	auto buttonsLayout = new QHBoxLayout();
	buttonsLayout->addWidget(resetButton, 0, Qt::AlignCenter);
	buttonsLayout->addWidget(syncButton, 0, Qt::AlignCenter);

	auto mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(m_tapButton, 0, Qt::AlignCenter);
	mainLayout->addLayout(buttonsLayout);
	mainLayout->addLayout(sidebarLayout);

	connect(m_tapButton, &QPushButton::pressed, this, [this, muteCheckBox]() {
		if (!muteCheckBox->isChecked())
		{
			const auto timeSigNumerator = Engine::getSong()->getTimeSigModel().getNumerator();
			Engine::audioEngine()->addPlayHandle(new SamplePlayHandle(
				m_plugin->m_numTaps % timeSigNumerator == 0 ? "misc/metronome02.ogg" : "misc/metronome01.ogg"));
		}

		m_plugin->onBpmClick();
		updateLabels();
	});

	connect(resetButton, &QPushButton::pressed, this, [this]() { closeEvent(nullptr); });

	connect(precisionCheckBox, &QCheckBox::toggled, [this](bool checked) {
		m_plugin->m_showDecimal = checked;
		updateLabels();
	});

	connect(syncButton, &QPushButton::clicked, this, [this]() {
		const auto& tempoModel = Engine::getSong()->tempoModel();
		if (m_plugin->m_bpm < tempoModel.minValue() || m_plugin->m_bpm > tempoModel.maxValue()) { return; }
		Engine::getSong()->setTempo(std::round(m_plugin->m_bpm));
	});

	hide();
	if (parentWidget())
	{
		parentWidget()->hide();
		parentWidget()->layout()->setSizeConstraint(QLayout::SetFixedSize);

		Qt::WindowFlags flags = parentWidget()->windowFlags();
		flags |= Qt::MSWindowsFixedSizeDialogHint;
		flags &= ~Qt::WindowMaximizeButtonHint;
		parentWidget()->setWindowFlags(flags);
	}
}

void TapTempoView::updateLabels()
{
	const double bpm = m_plugin->m_showDecimal ? m_plugin->m_bpm : std::round(m_plugin->m_bpm);
	const double hz = bpm / 60;
	const double ms = bpm > 0 ? 1 / hz * 1000 : 0;

	m_tapButton->setText(QString::number(bpm, 'f', m_plugin->m_showDecimal ? 1 : 0));
	m_msLabel->setText(tr("%1 ms").arg(ms, 0, 'f', m_plugin->m_showDecimal ? 1 : 0));
	m_hzLabel->setText(tr("%1 hz").arg(hz, 0, 'f', 4));
}

void TapTempoView::keyPressEvent(QKeyEvent* event)
{
	QWidget::keyPressEvent(event);
	if (!event->isAutoRepeat()) { m_plugin->onBpmClick(); }
}

void TapTempoView::closeEvent(QCloseEvent*)
{
	m_plugin->m_numTaps = 0;
	m_plugin->m_bpm = 0;
	updateLabels();
}

} // namespace lmms::gui
