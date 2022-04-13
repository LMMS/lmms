/*
 * TapTempo.cpp - plugin to count beats per minute
 *
 *
 * Copyright (c) 2022 sakertooth <sakertooth@gmail.com>
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

#include "TapTempo.h"

#include <cfloat>
#include <cmath>
#include <string>

#include <QFont>
#include <QLabel>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QPushButton>

#include "embed.h"
#include "LedCheckBox.h"
#include "plugin_export.h"


double asSeconds(std::chrono::duration<double> duration)
{
	return duration.count();
}


extern "C"
{
Plugin::Descriptor PLUGIN_EXPORT taptempo_plugin_descriptor =
{
		STRINGIFY( PLUGIN_NAME ),
		"Tap Tempo",
		QT_TRANSLATE_NOOP( "PluginBrowser",
							"Tap to the beat" ),
		"sakertooth <sakertooth@gmail.com>",
		0x0100,
		Plugin::Tool,
		new PluginPixmapLoader("logo"),
		nullptr,
		nullptr
} ;

// neccessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model *, void *)
{
	return new TapTempo;
}
}

TapTempo::TapTempo() : 
	ToolPlugin(&taptempo_plugin_descriptor, nullptr)
{
}

QString TapTempo::nodeName() const
{
	return taptempo_plugin_descriptor.name;
}

TapTempoView::TapTempoView(ToolPlugin * _tool) :
	ToolPluginView(_tool),
	m_numTaps(0)
{
	setFixedSize(200, 200);
	m_bpmButton = new QPushButton;
	m_bpmButton->setText("0");
	m_bpmButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	
	QFont font = m_bpmButton->font();
	font.setPointSize(25);
	m_bpmButton->setFont(font);

	QWidget* labelArea = new QWidget(this);
	m_msLabel = new QLabel;
	m_hzLabel = new QLabel;

	LedCheckBox* checkBox = new LedCheckBox(labelArea, tr("Display high precision"));
	checkBox->setToolTip(tr("Display high precision"));
	connect(checkBox, &LedCheckBox::toggled, [this](bool checked){ m_showDecimal = checked; updateLabels(); });
	
	QVBoxLayout * layout = new QVBoxLayout(this);
	layout->setAlignment(Qt::AlignCenter);
	layout->addWidget(m_bpmButton, Qt::AlignCenter);
	layout->addWidget(labelArea);
	QHBoxLayout* labelLayout = new QHBoxLayout(labelArea);
	labelLayout->addWidget(m_msLabel, 1);
	labelLayout->addWidget(m_hzLabel, 1);
	labelLayout->addWidget(checkBox, 0);

	connect(m_bpmButton, &QPushButton::pressed, this, &TapTempoView::onBpmClick);

	hide();
	if(parentWidget())
	{
		parentWidget()->hide();
		parentWidget()->layout()->setSizeConstraint(QLayout::SetFixedSize);

		Qt::WindowFlags flags = parentWidget()->windowFlags();
		flags |= Qt::MSWindowsFixedSizeDialogHint;
		flags &= ~Qt::WindowMaximizeButtonHint;
		parentWidget()->setWindowFlags(flags);
	}

	updateLabels();
}


void TapTempoView::onBpmClick()
{
	auto currentTime = std::chrono::steady_clock::now();

	if (asSeconds(currentTime - m_previousTime) > 2.0)
	{
		m_numTaps = 1;
		m_firstTime = currentTime;
		m_previousTime = currentTime;
		m_bpm = 0;
		updateLabels();
		return;
	}

	++m_numTaps;
	
	m_bpm = (m_numTaps - 1) / std::max(DBL_MIN, asSeconds(currentTime - m_firstTime)) * 60;
	updateLabels();

	m_previousTime = currentTime;
}


void TapTempoView::updateLabels()
{
	// Round the BPM before calculating Hz and ms
	double bpm = m_showDecimal ? m_bpm : std::round(m_bpm);
	double hz = bpm / 60;
	double ms = bpm > 0 ? 1 / hz * 1000 : 0;

	m_bpmButton->setText(QString::number(bpm, 'f', m_showDecimal ? 1 : 0));
	m_msLabel->setText(tr("%1 ms").arg(ms, 0, 'f', m_showDecimal ? 1 : 0));
	m_hzLabel->setText(tr("%1 Hz").arg(hz, 0, 'f', 4));
}


void TapTempoView::keyPressEvent(QKeyEvent* event) 
{
	QWidget::keyPressEvent(event);
	if (!event->isAutoRepeat()) 
	{
		onBpmClick();
	}
}

void TapTempoView::closeEvent(QCloseEvent* event)
{
	m_numTaps = 0;
	m_bpm = 0;
	updateLabels();
}
