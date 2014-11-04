/*
 * LadspaControlDialog.h - dialog for displaying and editing control port
 *                         values for LADSPA plugins
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LADSPA_CONTROL_DIALOG_H
#define LADSPA_CONTROL_DIALOG_H

#include "EffectControlDialog.h"


class QHBoxLayout;
class LadspaControls;
class ledCheckBox;


class LadspaControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	LadspaControlDialog( LadspaControls * _ctl );
	~LadspaControlDialog();


private slots:
	void updateEffectView( LadspaControls * _ctl );


private:
	QHBoxLayout * m_effectLayout;
	ledCheckBox * m_stereoLink;

} ;

#endif
