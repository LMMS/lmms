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
#include <QVariant>
#include <QWidget>

#include "Engine.h"
#include "SamplePlayHandle.h"
#include "Song.h"
#include "TapTempo.h"

namespace lmms::gui {
TapTempoView::TapTempoView(TapTempo* plugin)
	: ToolPluginView(plugin)
	, m_plugin(plugin)
{
	setFixedSize(250, 288);

	auto font = QFont();
	font.setPointSize(24);
	m_tapButton = new QPushButton();
	m_tapButton->setFixedSize(200, 200);
	m_tapButton->setFont(font);
	m_tapButton->setText(tr("0"));

	m_precisionCheckBox = new QCheckBox(tr("Precision"));
	m_precisionCheckBox->setToolTip(tr("Display in high precision"));
	m_precisionCheckBox->setText(tr("Precision"));

	m_muteCheckBox = new QCheckBox(tr("0.0 ms"));
	m_muteCheckBox->setToolTip(tr("Mute metronome"));
	m_muteCheckBox->setText(tr("Mute"));

	m_msLabel = new QLabel();
	m_msLabel->setToolTip(tr("BPM in milliseconds"));
	m_msLabel->setText(tr("0.0 ms"));

	m_hzLabel = new QLabel();
	m_hzLabel->setToolTip(tr("Frequency of BPM"));
	m_hzLabel->setText(tr("0.0 hz"));

	m_verticalLayout = new QVBoxLayout();
	m_verticalLayout->addWidget(m_precisionCheckBox);
	m_verticalLayout->addWidget(m_muteCheckBox);

	m_sidebarLayout = new QHBoxLayout();
	m_sidebarLayout->setSpacing(5);
	m_sidebarLayout->addWidget(m_msLabel, 0, Qt::AlignHCenter);
	m_sidebarLayout->addWidget(m_hzLabel, 0, Qt::AlignHCenter);
	m_sidebarLayout->addLayout(m_verticalLayout);

	m_mainLayout = new QVBoxLayout();
	m_mainLayout->setSpacing(5);
	m_mainLayout->setContentsMargins(2, 2, 2, 2);
	m_mainLayout->addWidget(m_tapButton, 0, Qt::AlignHCenter | Qt::AlignVCenter);
	m_mainLayout->addLayout(m_sidebarLayout);

	m_windowLayout = new QVBoxLayout(this);
	m_windowLayout->setSpacing(0);
	m_windowLayout->setContentsMargins(0, 0, 0, 0);
	m_windowLayout->addLayout(m_mainLayout);

	connect(m_tapButton, &QPushButton::pressed, this, [this]() {
		m_plugin->onBpmClick();
		updateLabels();

		if (!m_muteCheckBox->isChecked())
		{
			const auto timeSigNumerator = Engine::getSong()->getTimeSigModel().getNumerator();
			m_plugin->m_numTaps % timeSigNumerator == 0
				? Engine::audioEngine()->addPlayHandle(new SamplePlayHandle("misc/metronome02.ogg"))
				: Engine::audioEngine()->addPlayHandle(new SamplePlayHandle("misc/metronome01.ogg"));
		}
	});

	connect(m_precisionCheckBox, &QCheckBox::toggled, [this](bool checked) {
		m_plugin->m_showDecimal = checked;
		updateLabels();
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
	m_hzLabel->setText(tr("%1 Hz").arg(hz, 0, 'f', 4));
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
