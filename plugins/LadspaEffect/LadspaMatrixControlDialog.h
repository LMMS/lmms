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

#ifndef LADSPA_MATRIX_CONTROL_DIALOG_H
#define LADSPA_MATRIX_CONTROL_DIALOG_H

#include "EffectControlDialog.h"


class QGridLayout;

namespace lmms
{

class LadspaControls;

namespace gui
{

class LedCheckBox;


class LadspaMatrixControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	LadspaMatrixControlDialog( LadspaControls * _ctl );
	~LadspaMatrixControlDialog();

	virtual bool isResizable() const { return true; }

private slots:
	void updateEffectView( LadspaControls * _ctl );


private:
	QGridLayout * m_effectGridLayout;
	LedCheckBox * m_stereoLink;

} ;

} // namespace gui

} // namespace lmms

#endif
