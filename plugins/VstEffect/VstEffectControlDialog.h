/*
 * VstEffectControlDialog.h - dialog for displaying GUI of VST-effect-plugin
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _VST_EFFECT_CONTROL_DIALOG_H
#define _VST_EFFECT_CONTROL_DIALOG_H

#include "EffectControlDialog.h"
#include "VstPlugin.h"

#include <QObject>
#include <QPainter>
#include <QLabel>
#include <QSharedPointer>


class VstEffectControls;
class PixmapButton;
class QPixmap;
class QPushButton;
class PixmapButton;


class VstEffectControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	VstEffectControlDialog( VstEffectControls * _controls );
	virtual ~VstEffectControlDialog();

protected:
	void paintEvent( QPaintEvent * _pe ) override;
	void showEvent( QShowEvent* _se ) override;

private:
	QWidget * m_pluginWidget;

	QPushButton * m_togglePluginButton;
	PixmapButton * m_openPresetButton;
	PixmapButton * m_rolLPresetButton;
	PixmapButton * m_rolRPresetButton;
	PixmapButton * m_managePluginButton;
	PixmapButton * m_savePresetButton;

	QSharedPointer<VstPlugin> m_plugin;

	QLabel * tbLabel;

public slots:
	void togglePluginUI( bool checked );
} ;

#endif
