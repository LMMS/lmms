/*
 * ClapViewBase.h - Base class for CLAP plugin views
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_GUI_CLAP_VIEW_BASE_H
#define LMMS_GUI_CLAP_VIEW_BASE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include "LinkedModelGroupViews.h"
#include "lmms_export.h"

class QPushButton;
class QMdiSubWindow;

namespace lmms
{

class ClapInstance;
class ClapControlBase;

namespace gui
{

//! View for one processor, ClapViewBase contains 2 of those for mono plugins
class ClapViewInstance : public LinkedModelGroupView
{
public:
	//! @param colNum numbers of columns for the controls
	ClapViewInstance(QWidget* parent, ClapInstance* ctrlBase, int colNum);
	~ClapViewInstance() override = default;

private:
	ClapInstance* m_instance = nullptr;
};


//! Base class for view for one CLAP plugin
class LMMS_EXPORT ClapViewBase : public LinkedModelGroupsView
{
protected:
	//! @param pluginWidget A child class which inherits QWidget
	ClapViewBase(class QWidget* pluginWidget, ClapControlBase* ctrlBase);
	~ClapViewBase();

	// these widgets must be connected by child widgets
	QPushButton* m_reloadPluginButton = nullptr;
	QPushButton* m_toggleUIButton = nullptr;
	QPushButton* m_helpButton = nullptr;

	void toggleUI();
	void toggleHelp(bool visible);

	// to be called by child virtuals
	//! Reconnect models if model changed
	void modelChanged(ClapControlBase* ctrlBase);

private:
	enum class Rows
	{
		ButtonRow,
		ProcRow,
		LinkChannelsRow
	};

	auto getGroupView() -> LinkedModelGroupView* override { return m_procView; }

	ClapViewInstance* m_procView = nullptr;

	//! Numbers of controls per row; must be multiple of 2 for mono effects
	static constexpr int s_colNum = 6;

	QMdiSubWindow* m_helpWindow = nullptr;
};


} // namespace gui

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_GUI_CLAP_VIEW_BASE_H
