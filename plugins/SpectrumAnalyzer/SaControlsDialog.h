/*
 * SaControlsDialog.h - declatation of SaControlsDialog class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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

#ifndef SACONTROLSDIALOG_H
#define SACONTROLSDIALOG_H

#include <QLabel>
#include <QPushButton>

#include "EffectControlDialog.h"
#include "SaSpectrumView.h"


class BoolModel;
class SaControls;

class SaControlsDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	SaControlsDialog(SaControls *controls, SaProcessor *processor);
	virtual ~SaControlsDialog() {}

	virtual bool isResizable() const {return true;}

private:
	SaControls * m_controls;
};

#endif // SACONTROLSDIALOG_H
