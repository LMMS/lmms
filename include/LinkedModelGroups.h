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
public:
	/*
		Initialization
	*/
	//! @param parent model of the LinkedModelGroups class
	//! @param curProc number of this processor, counted from 0
	//! @param nProc total number of processors
	LinkedModelGroup(Model* parent, std::size_t curProc) :
		Model(parent), m_curProc(curProc) {}

	/*
		Linking
	*/
	void linkControls(LinkedModelGroup *other);

	/*
		Models
	*/
	struct ModelInfo
	{
		QString m_name;
		class AutomatableModel* m_model;
		ModelInfo() { /* hopefully no one will use this */ } // TODO: remove?
		ModelInfo(const QString& name, AutomatableModel* model)
			: m_name(name), m_model(model) {}
	};

	template<class Functor>
	void foreach_model(const Functor& ftor)
	{
		for(auto itr = d.m_models.begin(); itr != d.m_models.end(); ++itr)
		{
			ftor(itr->first, itr->second);
		}
	}

	template<class Functor>
	void foreach_model(const Functor& ftor) const
	{
		for(auto itr = d.m_models.cbegin(); itr != d.m_models.cend(); ++itr)
		{
			ftor(itr->first, itr->second);
		}
	}

	std::size_t modelNum() const { return models().size(); }

	// this is bad style (redirecting into the sub-class), but this class
	// will be married with the sub-classes (Lv2Proc, SpaProc) anyways,
	// so let's do the dirty trick for now...
	virtual void removeControl(AutomatableModel *) {}

	/*
		Load/Save
	*/
	//! @param lmg0 The linking model group with index 0
	void saveValues(class QDomDocument& doc, class QDomElement& that);
	//! @param lmg0 The linking model group with index 0
	void loadValues(const class QDomElement& that);

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
	// TODO: remove
	std::map<std::string, ModelInfo>& models() { return d.m_models; }
	const std::map<std::string, ModelInfo>& models() const { return d.m_models; }

	struct
	{
		//! models for the controls; the vector defines indices for the controls
		std::map<std::string, ModelInfo> m_models;
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

	void linkAllModels();

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
};


#endif // LINKEDMODELGROUPS_H
