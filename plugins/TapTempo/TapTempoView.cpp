/*
 * TapTempoView.cpp - Plugin to count beats per minute
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
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
#include <cmath>

#include "FontHelper.h"
#include "TapTempo.h"

namespace lmms::gui {
TapTempoView::TapTempoView(TapTempo* plugin)
	: ToolPluginView(plugin)
	, m_tapButton(new QPushButton())
	, m_resetButton(new QPushButton())
	, m_syncButton(new QPushButton())
	, m_precisionCheckBox(new QCheckBox())
	, m_muteCheckBox(new QCheckBox())
	, m_msLabel(new QLabel())
	, m_hzLabel(new QLabel())
	, m_plugin(plugin)
{
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	auto font = QFont();

	m_tapButton->setFixedSize(200, 200);
	m_tapButton->setFont(adjustedToPixelSize(font, 32));
	m_tapButton->setText(tr("0"));

	m_precisionCheckBox->setFocusPolicy(Qt::NoFocus);
	m_precisionCheckBox->setToolTip(tr("Display in high precision"));
	m_precisionCheckBox->setText(tr("Precision"));

	m_muteCheckBox->setFocusPolicy(Qt::NoFocus);
	m_muteCheckBox->setToolTip(tr("Mute metronome"));
	m_muteCheckBox->setText(tr("Mute"));

	m_msLabel->setFocusPolicy(Qt::NoFocus);
	m_msLabel->setToolTip(tr("BPM in milliseconds"));
	m_msLabel->setText(tr("0 ms"));

	m_hzLabel->setFocusPolicy(Qt::NoFocus);
	m_hzLabel->setToolTip(tr("Frequency of BPM"));
	m_hzLabel->setText(tr("0.0000 hz"));

	m_resetButton->setFocusPolicy(Qt::NoFocus);
	m_resetButton->setToolTip(tr("Reset counter and sidebar information"));
	m_resetButton->setText(tr("Reset"));

	m_syncButton->setFocusPolicy(Qt::NoFocus);
	m_syncButton->setToolTip(tr("Sync with project tempo"));
	m_syncButton->setText(tr("Sync"));

	auto optionLayout = new QVBoxLayout();
	optionLayout->addWidget(m_precisionCheckBox);
	optionLayout->addWidget(m_muteCheckBox);

	auto bpmInfoLayout = new QVBoxLayout();
	bpmInfoLayout->addWidget(m_msLabel, 0, Qt::AlignHCenter);
	bpmInfoLayout->addWidget(m_hzLabel, 0, Qt::AlignHCenter);

	auto sidebarLayout = new QHBoxLayout();
	sidebarLayout->addLayout(optionLayout);
	sidebarLayout->addLayout(bpmInfoLayout);

	auto buttonsLayout = new QHBoxLayout();
	buttonsLayout->addWidget(m_resetButton, 0, Qt::AlignCenter);
	buttonsLayout->addWidget(m_syncButton, 0, Qt::AlignCenter);

	auto mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(m_tapButton, 0, Qt::AlignCenter);
	mainLayout->addLayout(buttonsLayout);
	mainLayout->addLayout(sidebarLayout);

	connect(m_tapButton, &QPushButton::pressed, this, [this] {
		m_plugin->tap(!m_muteCheckBox->isChecked());
		update();
	});

	connect(m_resetButton, &QPushButton::pressed, this, [this] {
		m_plugin->reset();
		update();
	});

	connect(m_precisionCheckBox, &QCheckBox::toggled, this, &TapTempoView::update);
	connect(m_syncButton, &QPushButton::clicked, m_plugin, &TapTempo::sync);

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

void TapTempoView::update()
{
	const double bpm = m_precisionCheckBox->isChecked() ? m_plugin->m_bpm : std::round(m_plugin->m_bpm);
	const double hz = bpm / 60;
	const double ms = bpm > 0 ? 1 / hz * 1000 : 0;

	m_tapButton->setText(QString::number(bpm, 'f', m_precisionCheckBox->isChecked() ? 1 : 0));
	m_msLabel->setText(tr("%1 ms").arg(ms, 0, 'f', m_precisionCheckBox->isChecked() ? 1 : 0));
	m_hzLabel->setText(tr("%1 hz").arg(hz, 0, 'f', 4));
}

void TapTempoView::closeEvent(QCloseEvent*)
{
	m_plugin->reset();
	update();
}

} // namespace lmms::gui
