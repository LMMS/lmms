/*
 * VstEffectControlDialog.h - dialog for displaying GUI of VST-effect-plugin
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _VST_EFFECT_CONTROL_DIALOG_H
#define _VST_EFFECT_CONTROL_DIALOG_H

#include "EffectControlDialog.h"
#include "VstPlugin.h"

#include <QObject>
#include <QPainter>
#include <QtGui/QLabel>


class VstEffectControls;
class pixmapButton;
class QPixmap;
class QPushButton;
class pixmapButton;


class VstEffectControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	VstEffectControlDialog( VstEffectControls * _controls );
	virtual ~VstEffectControlDialog();

protected:
	virtual void paintEvent( QPaintEvent * _pe );

private:
	QWidget * m_pluginWidget;

	pixmapButton * m_openPresetButton;
	pixmapButton * m_rolLPresetButton;
	pixmapButton * m_rolRPresetButton;
	pixmapButton * m_managePluginButton;
	pixmapButton * m_savePresetButton;

	VstPlugin * m_plugin;

	QLabel * tbLabel;
} ;

#endif
