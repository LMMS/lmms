/*
 * LinkedModelGroupViews.h - views for groups of linkable models
 *
 * Copyright (c) 2019-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#ifndef LINKEDMODELGROUPVIEWS_H
#define LINKEDMODELGROUPVIEWS_H


#include <memory>
#include <vector>
#include <QGroupBox>


/**
	@file LinkedModelGroupViews.h
	See Lv2ViewBase.h for example usage
*/


//! View for one processor, LinkedModelGroupsViewBase contains 2
//! of those for mono plugins
class LinkedModelGroupViewBase : public QGroupBox
{
public:
	/**
		@param colNum numbers of columns for the controls
			(link LEDs not counted)
		@param name Name for the group, like "Left" or "Group 1",
			automatically set if not given
	*/
	LinkedModelGroupViewBase(QWidget *parent, class LinkedModelGroup* model,
		int colNum, int nProc, const QString &name = QString());
	~LinkedModelGroupViewBase();

	//! Reconnect models if model changed
	void modelChanged(class LinkedModelGroup *linkedModelGroup);

protected:
	//! Add a control to this widget
	void addControl(class ControlBase *ctrl);

private:
	void makeAllGridCellsEqualSized();

	int m_colNum; //!< column number in surrounding grid in Lv2ViewBase
	bool m_isLinking;
	class QGridLayout* m_grid;
	std::vector<std::unique_ptr<class ControlBase>> m_controls;
	std::vector<std::unique_ptr<class LedCheckBox>> m_leds;
};


//! Base class for view for one plugin with linkable models.
//! Provides a global channel link LED.
class LinkedModelGroupsViewBase
{
protected:
	LinkedModelGroupsViewBase(class LinkedModelGroups *ctrlBase);
	~LinkedModelGroupsViewBase() = default;

	//! Reconnect models if model changed; to be called by child virtuals
	void modelChanged(class LinkedModelGroups* ctrlBase);

	//! Access to the global multi channel link LED
	LedCheckBox* globalLinkLed() { return m_multiChannelLink.get(); }

private:
	//! The base class must return the adressed group view
	virtual LinkedModelGroupViewBase* getGroupView(std::size_t idx) = 0;

	// Implement deletion in the CPP file:
	struct MultiChannelLinkDeleter { void operator()(LedCheckBox* l); };
	std::unique_ptr<class LedCheckBox, MultiChannelLinkDeleter>
		m_multiChannelLink = nullptr;
};


#endif // LINKEDMODELGROUPVIEWS_H
