/*
 * LinkedModelGroups.h - base classes for groups of linked models
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

#ifndef LMMS_LINKED_MODEL_GROUPS_H
#define LMMS_LINKED_MODEL_GROUPS_H

#include <cstddef>

#include "Model.h"

class QDomDocument;
class QDomElement;

namespace lmms
{

/**
	@file LinkedModelGroups.h
	See Lv2ControlBase.h and Lv2Proc.h for example usage
*/


/**
	Base class for a group of linked models

	See the LinkedModelGroup class for explanations

	Features:
	* Models are stored by their QObject::objectName
	* Models are linked automatically
*/
class LinkedModelGroup : public Model
{
	Q_OBJECT
public:
	/*
		Initialization
	*/
	//! @param parent model of the LinkedModelGroups class
	LinkedModelGroup(Model* parent) : Model(parent) {}

	/*
		Linking (initially only)
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

	// TODO: refactor those 2
	template<class Functor>
	void foreach_model(const Functor& ftor)
	{
		for (auto itr = m_models.begin(); itr != m_models.end(); ++itr)
		{
			ftor(itr->first, itr->second);
		}
	}

	template<class Functor>
	void foreach_model(const Functor& ftor) const
	{
		for (auto itr = m_models.cbegin(); itr != m_models.cend(); ++itr)
		{
			ftor(itr->first, itr->second);
		}
	}

	std::size_t modelNum() const { return m_models.size(); }
	bool containsModel(const QString& name) const;
	void removeControl(AutomatableModel *);

	/*
		Load/Save
	*/
	void saveValues(class QDomDocument& doc, class QDomElement& that);
	void loadValues(const class QDomElement& that);

signals:
	// NOTE: when separating core from UI, this will need to be removed
	// (who would know if the client is Qt, i.e. it may not have slots at all)
	// In this case you'd e.g. send the UI something like
	// "/added <model meta info>"
	void modelAdded(lmms::AutomatableModel* added);
	void modelRemoved(lmms::AutomatableModel* removed);

public:
	AutomatableModel* getModel(const std::string& s)
	{
		auto itr = m_models.find(s);
		return (itr == m_models.end()) ? nullptr : itr->second.m_model;
	}

	//! Register a further model
	void addModel(class AutomatableModel* model, const QString& name);
	//! Unregister a model, return true if a model was erased
	bool eraseModel(const QString& name);

	//! Remove all models
	void clearModels();

private:
	//! models for the controls
	//! @note The AutomatableModels behind the ModelInfo are not owned,
	//!   but referenced after `addModel` is being called.
	std::map<std::string, ModelInfo> m_models;
};


/**
	Container for a group of linked models

	Each group contains the same models and model types. The models are
	numbered, and equal numbered models are associated and always linked.

	A typical application are two mono plugins making a stereo plugin.

	@note Though this class can contain multiple model groups, a corresponding
	view ("LinkedModelGroupViews") will only display one group, as they all have
	the same values

	@note Though called "container", this class does not contain, but only
	know the single groups. The inheriting classes are responsible for storage.
*/
class LinkedModelGroups
{
public:
	virtual ~LinkedModelGroups() = default;

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


} // namespace lmms

#endif // LMMS_LINKED_MODEL_GROUPS_H
