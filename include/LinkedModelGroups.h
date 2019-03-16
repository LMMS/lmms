/*
 * LinkedModelGroups.h - base classes for groups of linkable models
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

#ifndef LINKEDMODELGROUPS_H
#define LINKEDMODELGROUPS_H


#include <memory>
#include <vector>

#include "Model.h"


/**
	@file LinkedModelGroups.h
	See Lv2ControlBase.h and Lv2Proc.h for example usage
*/


/**
	Base class for a group of linkable models

	See the LinkedModelGroup class for explenations
*/
class LinkedModelGroup : public Model
{
	Q_OBJECT
signals:
	//! Signal emitted after any of the per-control link-enabled models switch
	void linkStateChanged(int id, bool value);

public:
	/*
		Initialization
	*/
	//! @param parent model of the LinkedModelGroups class
	LinkedModelGroup(Model* parent) : Model(parent) {}
	//! After all models have been added, make this processor the one which
	//! will contain link models associated with its controls
	void makeLinkingProc();

	/*
		Linking
	*/
	//! Set all per-control link-enabled models to @p state, which will
	//! also link or unlink them (via `Lv2ControlBase::linkPort()`)
	void linkAllModels(bool state);
	//! Link specified port with the associated port of @p other
	//! @param id id of the port, conforming to m_models
	void linkControls(LinkedModelGroup* other, int id);
	//! @see linkControls
	void unlinkControls(LinkedModelGroup *other, int id);
	//! Return whether this is the first of more than one processors
	bool isLinking() { return m_linkEnabled.size(); }

	/*
		Models
	*/
	class BoolModel* linkEnabledModel(std::size_t id) {
		return m_linkEnabled[id]; }
	std::vector<class AutomatableModel*> models() { return m_models; }

protected:
	//! Register a further model
	void addModel(class AutomatableModel* model);

private slots:
	//! Callback called after any of the per-control link-enabled models switch
	void linkStateChangedSlot();

private:
	//! models for the per-control link-enabled models
	std::vector<class BoolModel*> m_linkEnabled;
	//! models for the controls; the vector defines indices for the controls
	std::vector<class AutomatableModel*> m_models;
};


/**
	Container for multiple equal groups of linked models

	Each group contains the same models and model types. The models are
	numbered, and equal numbered models are associated and can be linked.

	A typical application are two mono plugins making a stereo plugin.

	Inheriting classes need to do the following connections, where the slots
	must be defined by those classes and call the equal named functions of this
	class:

	\code
		if(multiChannelLinkModel()) {
			connect(multiChannelLinkModel(), SIGNAL(dataChanged()),
				this, SLOT(updateLinkStatesFromGlobal()));
			connect(getGroup(0), SIGNAL(linkStateChanged(int, bool)),
					this, SLOT(linkPort(int, bool)));
		}
	\endcode
*/
class LinkedModelGroups
{
public:
	virtual ~LinkedModelGroups();

	//! Return the model for multi channel linking
	BoolModel* multiChannelLinkModel() { return m_multiChannelLinkModel.get(); }
	//! Create the model for multi channel linking
	void createMultiChannelLinkModel();

	/*
		to be called by slots
	*/
	//! Take a specified port from the first Lv2Proc and link or unlink it
	//! from the associated port of every other Lv2Proc
	//! @param port number conforming to Lv2Proc::m_modelVector
	//! @param state True iff it should be linked
	void linkPort(int port, bool state);
	//! Callback for the global linking LED
	void updateLinkStatesFromGlobal();

	//! Derived classes must return the group with index @p idx,
	//! or nullptr if @p is out of range
	virtual LinkedModelGroup* getGroup(std::size_t idx) = 0;

private:
	//! Model for the "global" linking
	//! Only allocated if #processors > 1
	std::unique_ptr<class BoolModel> m_multiChannelLinkModel;

	//! Force updateLinkStatesFromGlobal() to not unlink any ports
	//! Implementation detail, see linkPort() implementation
	bool m_noLink = false;
};


#endif // LINKEDMODELGROUPS_H
