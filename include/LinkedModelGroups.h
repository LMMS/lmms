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


#include <cstddef>
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
	void linkStateChanged(std::size_t id, bool value);

public:
	/*
		Initialization
	*/
	//! @param parent model of the LinkedModelGroups class
	//! @param curProc number of this processor, counted from 0
	//! @param nProc total number of processors
	LinkedModelGroup(Model* parent, std::size_t curProc) :
		Model(parent), m_curProc(curProc) {}
	//! After all models have been added, make this processor the one which
	//! will contain link models associated with its controls
	void makeLinkingProc();

	/*
		Linking
	*/
	//! Set all per-control link-enabled models to @p state, which will
	//! also link or unlink them (via `LinkedModelGroups::linkModel()`)
	void linkAllModels(bool state);
	//! Link specified port with the associated port of @p other
	//! @param id id of the port, conforming to m_models
	void linkControls(LinkedModelGroup* other, std::size_t id);
	//! @see linkControls
	void unlinkControls(LinkedModelGroup *other, std::size_t id);
	//! Return whether this is the first of more than one processors
	bool isLinking() const { return d.m_linkEnabled.size(); }

	/*
		Models
	*/
	struct ModelInfo
	{
		QString m_name;
		class AutomatableModel* m_model;
		ModelInfo(const QString& name, AutomatableModel* model)
			: m_name(name), m_model(model) {}
	};

	class BoolModel* linkEnabledModel(std::size_t id)
	{
		return d.m_linkEnabled[id];
	}
	const class BoolModel* linkEnabledModel(std::size_t id) const
	{
		return d.m_linkEnabled[id];
	}
	std::vector<ModelInfo>& models() { return d.m_models; }
	const std::vector<ModelInfo>& models() const { return d.m_models; }

	/*
		Load/Save
	*/
	//! @param lmg0 The linking model group with index 0
	void saveValues(class QDomDocument& doc, class QDomElement& that,
					const LinkedModelGroup *lmg0);
	void saveLinksEnabled(QDomDocument &doc, QDomElement &that);
	//! @param lmg0 The linking model group with index 0
	void loadValues(const class QDomElement& that, const LinkedModelGroup *lmg0);
	void loadLinksEnabled(const class QDomElement &that);

	/*
		General
	*/
	std::size_t curProc() const { return m_curProc; }

protected:
	//! Register a further model
	void addModel(class AutomatableModel* model, const QString& name);
	//! Remove all models and all link-enabled models
	void clearModels();

private:
	struct
	{
		//! models for the per-control link-enabled models
		std::vector<class BoolModel*> m_linkEnabled;
		//! models for the controls; the vector defines indices for the controls
		std::vector<ModelInfo> m_models;
	} d;

	std::size_t m_curProc;
};


/**
	Container for multiple equal groups of linked models

	Each group contains the same models and model types. The models are
	numbered, and equal numbered models are associated and can be linked.

	A typical application are two mono plugins making a stereo plugin.

	Inheriting classes need to do the following connections:

	\code
		if (multiChannelLinkModel())
		{
			connect(multiChannelLinkModel(), &BoolModel::dataChanged,
				this, [this](){updateLinkStatesFromGlobal();},
				Qt::DirectConnection);
			connect(getGroup(0), &LinkedModelGroup::linkStateChanged,
				this, [this](std::size_t id, bool value){
				linkModel(id, value);}, Qt::DirectConnection);
		}
	\endcode

	@note Though called "container", this class does not contain, but only
	know the single groups. The inheriting classes are responsible for storage.
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
	//! Take a specified model from the first LinkedModelGroup
	//! and link or unlink it to/from the associated model
	//! of every other LinkedModelGroup
	//! @param model number conforming to getGroup()
	//! @param state True iff it should be linked
	void linkModel(std::size_t model, bool state);
	//! Callback for the global linking LED
	void updateLinkStatesFromGlobal();

	/*
		Load/Save
	*/
	void saveSettings(class QDomDocument& doc, class QDomElement& that);
	void loadSettings(const class QDomElement& that);

	/*
		General
	*/
	//! Derived classes must return the group with index @p idx,
	//! or nullptr if @p is out of range
	virtual LinkedModelGroup* getGroup(std::size_t idx) = 0;
	//! @see getGroup
	virtual const LinkedModelGroup* getGroup(std::size_t idx) const = 0;

private:
	//! Model for the "global" linking
	//! Only allocated if #processors > 1
	std::unique_ptr<class BoolModel> m_multiChannelLinkModel;

	//! Force updateLinkStatesFromGlobal() to not unlink any ports
	//! Implementation detail, see linkPort() implementation
	bool m_noLink = false;
};


#endif // LINKEDMODELGROUPS_H
