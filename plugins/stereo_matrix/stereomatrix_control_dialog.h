/*
 * stereomatrix_control_dialog.h - control dialog for stereoMatrix-effect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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

#ifndef _STEREOMATRIX_CONTROL_DIALOG_H
#define _STEREOMATRIX_CONTROL_DIALOG_H

#include "EffectControlDialog.h"

class stereoMatrixControls;


class stereoMatrixControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	stereoMatrixControlDialog( stereoMatrixControls * _controls );
	virtual ~stereoMatrixControlDialog()
	{
	}

};


#endif
