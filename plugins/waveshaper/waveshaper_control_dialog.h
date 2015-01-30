/*
 * waveshaper_control_dialog.h - control-dialog for waveshaper-effect
 *
 * * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _WAVESHAPER_CONTROL_DIALOG_H
#define _WAVESHAPER_CONTROL_DIALOG_H

#include "EffectControlDialog.h"


class waveShaperControls;


class waveShaperControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	waveShaperControlDialog( waveShaperControls * _controls );
	virtual ~waveShaperControlDialog()
	{
	}


private:

} ;

#endif
