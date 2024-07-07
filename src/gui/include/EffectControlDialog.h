/*
 * EffectControlDialog.h - base-class for effect-dialogs for displaying and
 *                         editing control port values
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_EFFECT_CONTROL_DIALOG_H
#define LMMS_GUI_EFFECT_CONTROL_DIALOG_H

#include <QWidget>

#include "ModelView.h"

namespace lmms
{

class EffectControls;


namespace gui
{

class LMMS_EXPORT EffectControlDialog : public QWidget, public ModelView
{
	Q_OBJECT
public:
	EffectControlDialog( EffectControls * _controls );
	~EffectControlDialog() override = default;

	virtual bool isResizable() const {return false;}


signals:
	void closed();


protected:
	void closeEvent( QCloseEvent * _ce ) override;

	EffectControls * m_effectControls;

} ;

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_EFFECT_CONTROL_DIALOG_H
