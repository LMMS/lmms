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
#include "SampleLoader.h"
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
	, m_instrument(instrument)
{
	// window settings
	setAcceptDrops(true);
	setAutoFillBackground(true);

	setMaximumSize(QSize(10000, 10000));
	setMinimumSize(QSize(516, 400));

	auto openfilebutton = new QPushButton("Open SFZ File", this);
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

void SfzSamplerView::paintEvent(QPaintEvent* pe)
{
	QPainter brush(this);
}

void SfzSamplerView::resizeEvent(QResizeEvent* re)
{
}

} // namespace gui
} // namespace lmms
