/* SaWaterfallView.h - declaration of SaWaterfallView class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
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
#ifndef SAWATERFALLVIEW_H
#define SAWATERFALLVIEW_H

#include <string>
#include <utility>
#include <vector>
#include <QPainter>
#include <QWidget>

#include "SaControls.h"
#include "SaProcessor.h"


// Widget that displays a spectrum waterfall (spectrogram) and time labels.
class SaWaterfallView : public QWidget
{
	Q_OBJECT
public:
	explicit SaWaterfallView(SaControls *controls, SaProcessor *processor, QWidget *_parent = 0);
	virtual ~SaWaterfallView() {}

	QSize sizeHint() const override {return QSize(400, 350);}

	// Check if waterfall should be displayed and adjust window size if needed.
	void updateVisibility();

protected:
	void paintEvent(QPaintEvent *event) override;

private slots:
	void periodicUpdate();

private:
	const SaControls *m_controls;
	SaProcessor *m_processor;
	const EffectControlDialog *m_controlDialog;

	// Methods and data used to make time labels
	float m_oldTimePerLine;
	float timeToYPixel(float time, int height);
	std::vector<std::pair<float, std::string>> makeTimeTics();
	std::vector<std::pair<float, std::string>> m_timeTics;	// 0..n (s)
};
#endif // SAWATERFALLVIEW_H
