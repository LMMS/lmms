/*
 * VstEffectControls.h - controls for VST effect plugins
 *
 * Copyright (c) 2008-2011 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _VST_EFFECT_CONTROLS_H
#define _VST_EFFECT_CONTROLS_H

#include "EffectControls.h"


#include <QObject>

class QGridLayout;
class QPaintEvent;
class QPushButton;
class QMdiSubWindow;
class QScrollArea;

namespace lmms
{


class VstEffect;

namespace gui
{
class CustomTextKnob;
class ManageVSTEffectView;
class VstEffectControlDialog;
}


class VstEffectControls : public EffectControls
{
	Q_OBJECT
public:
	VstEffectControls( VstEffect * _eff );
	~VstEffectControls() override;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return "vsteffectcontrols";
	}

	int controlCount() override;

	gui::EffectControlDialog* createView() override;


protected slots:
	void updateMenu();
	void managePlugin();
	void openPreset();
	void savePreset();
	void rollPreset();
	void rolrPreset();
	void selPreset();
	void setParameter( lmms::Model * action );

protected:
	virtual void paintEvent( QPaintEvent * _pe );

private:
	VstEffect * m_effect;

	QPushButton * m_selPresetButton;

	QMdiSubWindow * m_subWindow;
	QScrollArea * m_scrollArea;
	std::vector<FloatModel*> knobFModel;
	int paramCount;

	QObject * ctrHandle;

	int lastPosInMenu;
//	QLabel * m_presetLabel;

	friend class gui::VstEffectControlDialog;
	friend class gui::ManageVSTEffectView;

	bool m_vstGuiVisible;
} ;


namespace gui
{


class ManageVSTEffectView : public QObject
{
	Q_OBJECT
public:
	ManageVSTEffectView( VstEffect * _eff, VstEffectControls * m_vi );
	~ManageVSTEffectView() override;


protected slots:
	void syncPlugin();
	void displayAutomatedOnly();
	void setParameter( lmms::Model * action );
	void syncParameterText();
	void closeWindow();

private:

//	static QPixmap * s_artwork;

	VstEffectControls * m_vi2;


	VstEffect * m_effect;


	QWidget *widget;
	QGridLayout * l;

	QPushButton * m_syncButton;
	QPushButton * m_displayAutomatedOnly;
	QPushButton * m_closeButton;
	CustomTextKnob ** vstKnobs;

} ;


} // namespace gui

} // namespace lmms

#endif
