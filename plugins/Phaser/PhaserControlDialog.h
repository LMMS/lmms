/*
 * PhaserControlDialog.h
 *
 * Copyright (c) 2019 Lost Robot <r94231@gmail.com>
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

#ifndef PHASER_CONTROL_DIALOG_H
#define PHASER_CONTROL_DIALOG_H

#include "EffectControlDialog.h"
#include <QLabel>
#include "Fader.h"


class PhaserControls;


class PhaserControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	PhaserControlDialog( PhaserControls* controls );
	virtual ~PhaserControlDialog()
	{
	}
	PhaserControls* model()
	{
		return m_controls;
	}

	const PhaserControls* model() const
	{
		return m_controls;
	}

	QLabel * m_cutoffDotLeftLabel;
	QLabel * m_cutoffDotRightLabel;

private slots:
	void updateSliders();

private:
	PhaserControls * m_controls;
} ;

#endif
