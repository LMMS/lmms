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

#include <QVBoxLayout>
#include <QLabel>
#include <QFont>

#include "TapTempo.h"
#include "plugin_export.h"
#include "embed.h"

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
	ToolPluginView(_tool), m_bpmAverage(0), m_numTaps(0), m_keyDown(false)
{

	setFixedSize(250, 250);
	m_bpmButton = new QPushButton;
	m_bpmButton->setText("0");
	m_bpmButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QFont font = m_bpmButton->font();
	font.setPointSize(25);
	m_bpmButton->setFont(font);
	
	QVBoxLayout * layout = new QVBoxLayout(this);
	layout->setAlignment(Qt::AlignCenter);
	layout->addWidget(m_bpmButton, Qt::AlignCenter);
	
	connect(m_bpmButton, &QPushButton::clicked, this, &TapTempoView::onBpmClick);

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

}

void TapTempoView::onBpmClick()
{
	++m_numTaps;
	if (m_numTaps == 1)
	{
		m_previousTime = QTime::currentTime();
	}
	else if (m_numTaps >= 2)
	{
		QTime currentTime = QTime::currentTime();
		double period = m_previousTime.msecsTo(currentTime) / 1000.0;

		// Reset after 2 seconds
		if (period > 2.0) 
		{
			m_bpmAverage = 0;
			m_numTaps = 0;
			m_bpmButton->setText("0");
			return;
		}

		double recentBpmAverage = 60.0/period;
		m_bpmAverage += (recentBpmAverage - m_bpmAverage)/(m_numTaps - 1);
		m_bpmButton->setText(QString::number(m_bpmAverage));
		m_previousTime = currentTime;
	}
}

void TapTempoView::keyPressEvent(QKeyEvent * event) 
{
	QWidget::keyPressEvent(event);
	if (!m_keyDown) {
		onBpmClick();
		m_keyDown = true;
	}
}

void TapTempoView::keyReleaseEvent(QKeyEvent * event) 
{
	QWidget::keyReleaseEvent(event);
	m_keyDown = false;
}