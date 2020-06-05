/*
 * Lv2ViewBase.h - base class for Lv2 plugin views
 *
 * Copyright (c) 2018-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#ifndef LV2VIEWBASE_H
#define LV2VIEWBASE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2


#include <QString>
#include <QVector>

#include "LinkedModelGroupViews.h"
#include "Lv2Basics.h"

class Lv2Proc;
class Lv2ControlBase;


//! View for one processor, Lv2ViewBase contains 2 of those for mono plugins
class Lv2ViewProc : public LinkedModelGroupView
{
public:
	//! @param colNum numbers of columns for the controls
	Lv2ViewProc(QWidget *parent, Lv2Proc *ctrlBase, int colNum);
	~Lv2ViewProc();

private:
	static AutoLilvNode uri(const char *uriStr);
};


//! Base class for view for one Lv2 plugin
class Lv2ViewBase : public LinkedModelGroupsView
{
protected:
	//! @param pluginWidget A child class which inherits QWidget
	Lv2ViewBase(class QWidget *pluginWidget, Lv2ControlBase *ctrlBase);
	~Lv2ViewBase();

	// these widgets must be connected by child widgets
	class QPushButton *m_reloadPluginButton = nullptr;
	class QPushButton *m_toggleUIButton = nullptr;
	class QPushButton *m_helpButton = nullptr;

	void toggleUI();
	void toggleHelp(bool visible);

	// to be called by child virtuals
	//! Reconnect models if model changed
	void modelChanged(Lv2ControlBase* ctrlBase);

private:
	enum Rows
	{
		ButtonRow,
		ProcRow,
		LinkChannelsRow
	};

	static AutoLilvNode uri(const char *uriStr);
	LinkedModelGroupView* getGroupView() override { return m_procView; }

	Lv2ViewProc* m_procView;

	//! Numbers of controls per row; must be multiple of 2 for mono effects
	const int m_colNum = 6;
	class QMdiSubWindow* m_helpWindow = nullptr;
	class LedCheckBox *m_multiChannelLink;
};


#endif // LMMS_HAVE_LV2
#endif // LV2VIEWBASE_H
