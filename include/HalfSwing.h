/*
 * HalfSwing.h - A groove that uses just the latter half of the Hydrogen Swing algorithm
 *
 * Copyright (c) 2005-2008 teknopaul <teknopaul/at/users.sourceforge.net>
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
 **/

#ifndef HALFSWING_H
#define HALFSWING_H

#include <QObject>

#include "AutomatableSlider.h"
#include "Groove.h"
#include "lmms_basics.h"
#include "TimePos.h"
#include "Note.h"
#include "MidiClip.h"

namespace lmms
{

/**
 * A groove that uses just the latter half of the Hydrogen Swing algorithm
 */
class HalfSwing : public Groove
{
	Q_OBJECT
public:
	HalfSwing(QObject* parent = nullptr);
	~HalfSwing() override;

	void init();

	int isInTick(TimePos* curStart, const fpp_t frames, const f_cnt_t offset, Note* n, MidiClip* c);

	QString nodeName() const override
	{
		return "half";
	}

	QWidget* instantiateView(QWidget* parent);

public slots:
	// valid values are from 0 - 127
	void update();

private:
	int m_framesPerTick;
} ;


namespace gui
{

class HalfSwingView : public QWidget
{
	Q_OBJECT
public:
	HalfSwingView(HalfSwing* halfSwing, QWidget* parent = nullptr);
	~HalfSwingView() override;

public slots:
	void valueChanged();
	void modelChanged();

private:
	HalfSwing* m_swing;
	IntModel* m_sliderModel;
	AutomatableSlider* m_slider;
};

} // namespace gui

} // namespace lmms

#endif // HALFSWING_H
