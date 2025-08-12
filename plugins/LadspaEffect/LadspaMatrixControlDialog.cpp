/*
 * LadspaMatrixControlDialog.h - Dialog for displaying and editing control port
 *                               values for LADSPA plugins in a matrix display
 *
 * Copyright (c) 2015 Michael Gregorius <michaelgregorius/at/web[dot]de>
 *
 * This file is part of LMMS - http://lmms.io
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


#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpacerItem>
#include <QVBoxLayout>


#include "LadspaControl.h"
#include "LadspaControls.h"
#include "LadspaMatrixControlDialog.h"
#include "LadspaWidgetFactory.h"
#include "LedCheckBox.h"


namespace lmms::gui
{

LadspaMatrixControlDialog::LadspaMatrixControlDialog(LadspaControls * ladspaControls) :
	EffectControlDialog(ladspaControls),
	m_scrollArea(nullptr),
	m_stereoLink(nullptr)
{
	QVBoxLayout * mainLayout = new QVBoxLayout(this);

	m_scrollArea = new QScrollArea(this);
	m_scrollArea->setWidgetResizable(true);
	m_scrollArea->setFrameShape(QFrame::NoFrame);
	// Set to always on so that the elements do not move around when the
	// scroll bar is hidden or shown.
	m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	// Add a scroll area that grows
	mainLayout->addWidget(m_scrollArea, 1);

	// Populate the parameter matrix and put it into the scroll area
	updateEffectView(ladspaControls);

	// Add button to link all channels if there's more than one channel
	if (getChannelCount() > 1)
	{
		mainLayout->addSpacing(3);

		m_stereoLink = new LedCheckBox(tr("Link Channels"), this, QString(), LedCheckBox::LedColor::Green, false);
		m_stereoLink->setModel(&ladspaControls->m_stereoLinkModel);
		mainLayout->addWidget(m_stereoLink, 0, Qt::AlignCenter);
	}
}

bool LadspaMatrixControlDialog::isResizable() const
{
	return true;
}

bool LadspaMatrixControlDialog::needsLinkColumn() const
{
	LadspaControls * ladspaControls = getLadspaControls();

	ch_cnt_t const channelCount = getChannelCount();
	for (ch_cnt_t i = 0; i < channelCount; ++i)
	{
		// Create a const reference so that the C++11 based for loop does not detach the Qt container
		auto const & currentControls = ladspaControls->m_controls[i];
		for (auto ladspaControl : currentControls)
		{
			if (ladspaControl->m_link)
			{
				return true;
			}
		}
	}

	return false;
}

void LadspaMatrixControlDialog::arrangeControls(QWidget * parent, QGridLayout* gridLayout)
{
	LadspaControls * ladspaControls = getLadspaControls();

	int const headerRow = 0;
	int const linkColumn = 0;

	bool const linkColumnNeeded = needsLinkColumn();
	if (linkColumnNeeded)
	{
		gridLayout->addWidget(new QLabel("<b>" + tr("Link") + "</b>", parent), headerRow, linkColumn, Qt::AlignHCenter);

		// If there's a link column then it should not stretch
		gridLayout->setColumnStretch(linkColumn, 0);
	}

	int const channelStartColumn = linkColumnNeeded ? 1 : 0;

	// The header row should not grow vertically
	gridLayout->setRowStretch(0, 0);

	// Records the maximum row with parameters so that we can add a vertical spacer after that row
	int maxRow = 0;

	// Iterate the channels and add widgets for each control
	// Note: the code assumes that all channels have the same structure, i.e. that all channels
	//       have the same number of parameters which are in the same order.
	ch_cnt_t const numberOfChannels = getChannelCount();
	for (ch_cnt_t i = 0; i < numberOfChannels; ++i)
	{
		int currentChannelColumn = channelStartColumn + i;
		gridLayout->setColumnStretch(currentChannelColumn, 1);

		// First add the channel header with the channel number
		gridLayout->addWidget(new QLabel("<b>" + tr("Channel %1").arg(QString::number(i + 1)) + "</b>", parent), headerRow, currentChannelColumn, Qt::AlignHCenter);

		int currentRow = 1;

		if (i == 0)
		{
			// Configure the current parameter row to not stretch.
			// Only do this once, i.e. when working with the first channel.
			gridLayout->setRowStretch(currentRow, 0);
		}

		// Create a const reference so that the C++11 based for loop does not detach the Qt container
		auto const & currentControls = ladspaControls->m_controls[i];
		for (auto ladspaControl : currentControls)
		{
			// Only use the first channel to determine if we need to add link controls
			if (i == 0 && ladspaControl->m_link)
			{
				LedCheckBox * linkCheckBox = new LedCheckBox("", parent, "", LedCheckBox::LedColor::Green);
				linkCheckBox->setModel(&ladspaControl->m_linkEnabledModel);
				linkCheckBox->setToolTip(tr("Link channels"));
				gridLayout->addWidget(linkCheckBox, currentRow, linkColumn, Qt::AlignHCenter);
			}

			QWidget * controlWidget = LadspaWidgetFactory::createWidget(ladspaControl, this);
			if (controlWidget)
			{
				gridLayout->addWidget(controlWidget, currentRow, currentChannelColumn);
			}

			// Record the maximum row so that we add a vertical spacer after that row
			maxRow = std::max(maxRow, currentRow);

			++currentRow;
		}
	}

	// Add a spacer item after the maximum row
	QSpacerItem * spacer = new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	gridLayout->addItem(spacer, maxRow + 1, 0);
}

QWidget * LadspaMatrixControlDialog::createMatrixWidget()
{
	QWidget *widget = new QWidget(this);
	QGridLayout *gridLayout = new QGridLayout(widget);
	gridLayout->setMargin(0);
	widget->setLayout(gridLayout);

	arrangeControls(widget, gridLayout);

	return widget;
}

void LadspaMatrixControlDialog::updateEffectView(LadspaControls * ladspaControls)
{
	m_effectControls = ladspaControls;

	// No need to delete the existing widget as it's deleted
	// by the scroll view when we replace it.
	QWidget * matrixWidget = createMatrixWidget();
	m_scrollArea->setWidget(matrixWidget);

	// Make sure that the horizontal scroll bar does not show
	// From: https://forum.qt.io/topic/13374/solved-qscrollarea-vertical-scroll-only/4
	m_scrollArea->setMinimumWidth(matrixWidget->minimumSizeHint().width() + m_scrollArea->verticalScrollBar()->width());

	if (getChannelCount() > 1 && m_stereoLink != nullptr)
	{
		m_stereoLink->setModel(&ladspaControls->m_stereoLinkModel);
	}

	connect(ladspaControls, &LadspaControls::effectModelChanged,
				this, &LadspaMatrixControlDialog::updateEffectView,
				Qt::DirectConnection);
}

LadspaControls * LadspaMatrixControlDialog::getLadspaControls() const
{
	return dynamic_cast<LadspaControls *>(m_effectControls);
}

ch_cnt_t LadspaMatrixControlDialog::getChannelCount() const
{
	return getLadspaControls()->m_processors;
}

} // namespace lmms::gui
