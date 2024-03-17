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
class QWidget;

namespace lmms
{

class ClapInstance;
class ClapControlBase;
class PluginPresets;

namespace gui
{

class PixmapButton;
class PresetSelector;

//! View for one processor, ClapViewBase contains 2 of those for mono plugins
class ClapViewParameters : public LinkedModelGroupView
{
public:
	//! @param colNum numbers of columns for the controls
	ClapViewParameters(QWidget* parent, ClapInstance* instance, int colNum);
	~ClapViewParameters() override = default;

private:
	ClapInstance* m_instance = nullptr;
};

//! View for one preset collection, ClapViewBase contains 2 of those for mono plugins
class ClapViewPresets : public LinkedModelGroupView
{
public:
	//! @param colNum numbers of columns for the controls
	ClapViewPresets(QWidget* parent, ClapInstance* instance, int colNum);
	~ClapViewPresets() override = default;

private:
	ClapInstance* m_instance = nullptr;
};


//! Base class for view for one CLAP plugin
class LMMS_EXPORT ClapViewBase : public LinkedModelGroupsView
{
protected:
	//! @param pluginWidget A child class which inherits QWidget
	ClapViewBase(QWidget* pluginWidget, ClapControlBase* ctrlBase);
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
		PresetRow,
		ButtonRow,
		ParametersRow,
		LinkChannelsRow
	};

	/*
	class Parameters : public LinkedModelGroupsView
	{
	public:
		Parameters(ClapViewParameters* view) : m_view{view} {}
		auto getGroupView() -> LinkedModelGroupView* override { return m_view; }
		operator QWidget*() const { return m_view; }
	private:
		ClapViewParameters* m_view = nullptr;
	} m_parametersView;

	class Presets : public LinkedModelGroupsView
	{
	public:
		Presets(ClapViewPresets* view) : m_view{view} {}
		auto getGroupView() -> LinkedModelGroupView* override { return m_view; }
		operator QWidget*() const { return m_view; }
	private:
		ClapViewPresets* m_view = nullptr;
	} m_presetsView;*/

	PresetSelector* m_presetSelector = nullptr;

	auto getGroupView() -> LinkedModelGroupView* override { return m_parametersView; }

	ClapViewParameters* m_parametersView = nullptr;

	//! Numbers of controls per row; must be multiple of 2 for mono effects
	static constexpr int s_colNum = 6;

	QMdiSubWindow* m_helpWindow = nullptr;
};


} // namespace gui

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_GUI_CLAP_VIEW_BASE_H
