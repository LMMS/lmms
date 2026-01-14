/*
 * SfzSamplerView.cpp - Controls View for SfzSampler
 *
 * Copyright (c) 2023 Daniel Kauss Serna <daniel.kauss.serna@gmail.com>
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

#include "SfzSamplerView.h"

#include <QDropEvent>
#include <QPainter>
#include <QPushButton>

#include "Clipboard.h"
#include "ComboBox.h"
#include "DataFile.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "LcdSpinBox.h"
#include "PixmapButton.h"
#include "SfzSampler.h"
#include "StringPairDrag.h"
#include "Track.h"
#include "embed.h"
#include "ConfigManager.h"
#include "FileDialog.h"
#include "PathUtil.h"
#include "embed.h"

namespace lmms {

namespace gui {

SfzSamplerView::SfzSamplerView(SfzSampler* instrument, QWidget* parent)
	: InstrumentView(instrument, parent)
	, m_logo(PLUGIN_NAME::getIconPixmap("logo"))
	, m_instrument(instrument)
	, m_hBoxLayout(new QHBoxLayout)
	, m_vBoxLayout(new QVBoxLayout)
	, m_sidebarWidget(new SfzSidebarWidget(this))
	, m_controlsWidget(new SfzControlsWidget(this))
	, m_pianoWidget(new SfzPianoWidget(this))
	, m_controlsLayout(new QBoxLayout(QBoxLayout::TopToBottom, m_controlsWidget))
	, m_sidebarLayout(new QVBoxLayout(m_sidebarWidget))
{
	// window settings
	setAcceptDrops(true);
	setAutoFillBackground(true);

	setMaximumSize(QSize(10000, 10000));
	setMinimumSize(QSize(s_sidebarMinWidth + s_controlsAreaWidth, s_controlsAreaHeight + s_keyboardAreaMinHeight));

	setLayout(m_hBoxLayout);
	m_hBoxLayout->addWidget(m_sidebarWidget);
	m_hBoxLayout->addWidget(new SfzDividerLine(this, 3, 11, false));
	m_hBoxLayout->addLayout(m_vBoxLayout);
	m_vBoxLayout->addWidget(m_controlsWidget);
	m_vBoxLayout->addWidget(new SfzDividerLine(this, 2, 8));
	m_vBoxLayout->addWidget(m_pianoWidget);

	m_hBoxLayout->setContentsMargins(0, 0, 0, 0);
	m_hBoxLayout->setSpacing(0);
	m_vBoxLayout->setContentsMargins(0, 0, 0, 0);
	m_vBoxLayout->setSpacing(0);

	m_controlsWidget->setFixedSize(QSize(s_controlsAreaWidth, s_controlsAreaHeight));
	m_sidebarWidget->setMinimumWidth(s_sidebarMinWidth);
	m_pianoWidget->setMinimumHeight(s_keyboardAreaMinHeight);

	auto logoFrame = new SfzSidebarGroupBox(m_sidebarWidget);
	auto logoLabel = new QLabel(logoFrame);
	logoLabel->setPixmap(m_logo);
	logoLabel->setAlignment(Qt::AlignCenter);
	logoLabel->setFixedHeight(70);
	logoFrame->layout()->setContentsMargins(0, 0, 0, 0);
	logoFrame->layout()->addWidget(logoLabel);

	auto frame1 = new SfzSidebarGroupBox(m_sidebarWidget);
	auto label1 = new QLabel("Testing1", frame1);
	frame1->layout()->addWidget(label1);

	auto frame2 = new SfzSidebarGroupBox(m_sidebarWidget);
	auto label2 = new QLabel("Testing2\nvery long\nwow", frame2);
	auto label3 = new QLabel("another thing!", frame2);
	frame2->layout()->addWidget(label2);
	frame2->layout()->addWidget(label3);

	
	m_sidebarLayout->addWidget(logoFrame);
	m_sidebarLayout->addWidget(new SfzDividerLine(this, 2, 8));
	m_sidebarLayout->addWidget(frame1);
	m_sidebarLayout->addWidget(new SfzDividerLine(this, 2, 8));
	m_sidebarLayout->addWidget(frame2);
	m_sidebarLayout->addWidget(new SfzDividerLine(this, 2, 8));
	m_sidebarLayout->addStretch();


	m_emptyFileLabel = new QLabel("Drag and drop or click to load SFZ file", m_controlsWidget);
	m_emptyFileLabel->setAlignment(Qt::AlignCenter);
	m_emptyFileLabel->move(s_controlsAreaWidth / 2, s_controlsAreaHeight / 2);
	m_controlsLayout->addWidget(m_emptyFileLabel, 0, Qt::AlignCenter);


	auto openfilebutton = new QPushButton("Open SFZ File", m_pianoWidget);
	openfilebutton->setIcon(embed::getIconPixmap("folder"));
	openfilebutton->setToolTip(tr("Open SFZ File"));
	connect(openfilebutton, &PixmapButton::clicked, [this](){
		auto openFileDialog = FileDialog(nullptr, QObject::tr("Open SFZ File"));
		auto dir = ConfigManager::inst()->userSamplesDir();
		openFileDialog.setDirectory(dir);
		if (openFileDialog.exec() == QDialog::Accepted)
		{
			if (openFileDialog.selectedFiles().isEmpty()) { return; }
			m_instrument->loadFile(openFileDialog.selectedFiles()[0]);
		}
	});

	update();
}


void SfzSamplerView::dragEnterEvent(QDragEnterEvent* dee)
{
}

void SfzSamplerView::dropEvent(QDropEvent* de)
{
}

void SfzSamplerView::resizeEvent(QResizeEvent* re)
{
}

void SfzSamplerView::paintEvent(QPaintEvent* pe)
{
	QPainter brush(this);
	brush.fillRect(rect(), QColor(19, 19, 20));
}

void SfzSidebarWidget::paintEvent(QPaintEvent* pe)
{
	QPainter brush(this);
	brush.fillRect(rect(), QColor(19, 19, 20));
}

void SfzControlsWidget::paintEvent(QPaintEvent* pe)
{
	QPainter brush(this);
	brush.fillRect(rect(), QColor(19, 19, 20));
}

void SfzPianoWidget::paintEvent(QPaintEvent* pe)
{
	QPainter brush(this);
	brush.fillRect(rect(), QColor(72, 72, 75));
}

SfzSidebarGroupBox::SfzSidebarGroupBox(QWidget* parent)
	: QFrame(parent)
{
	setLayout(new QVBoxLayout(this));
}

void SfzSidebarGroupBox::paintEvent(QPaintEvent* pe)
{
	QPainter brush(this);
	brush.fillRect(rect(), QColor(34, 39, 39));
}


SfzDividerLine::SfzDividerLine(QWidget* parent, float lineWidth, float dividerWidth, bool horizontal)
	: QWidget(parent)
	, m_lineWidth(lineWidth)
	, m_dividerWidth(dividerWidth)
	, m_horizontal(horizontal)
{
	if (horizontal)
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		setFixedHeight(m_dividerWidth);
	}
	else
	{
		setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		setFixedWidth(m_dividerWidth);
	}
}

void SfzDividerLine::paintEvent(QPaintEvent* pe)
{
	QPainter brush(this);
	const float padding = 0.5f * (m_dividerWidth);
	const float x1 = padding, y1 = padding, x2 = width() - padding, y2 = height() - padding;

	// Glow background of line
	const int numPasses = 4;
	for (int i = 1; i <= numPasses; ++i)
	{
		const float ratio = static_cast<float>(i) / numPasses;
		brush.setPen(QPen(QColor(131, 149, 255), m_dividerWidth * (1.0f - ratio), Qt::SolidLine, Qt::RoundCap));
		brush.setOpacity(ratio);
		brush.drawLine(x1, y1, x2, y2);
	}
	// Actual line
	brush.setPen(QPen(QColor(145, 232, 233), m_lineWidth, Qt::SolidLine, Qt::RoundCap));
	brush.setOpacity(1.0f);
	brush.drawLine(x1, y1, x2, y2);
}




} // namespace gui
} // namespace lmms
