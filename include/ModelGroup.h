/*
 * ModelGroup.h - base classes for groups of models
 *
 * Copyright (c) 2019-2024 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#ifndef LMMS_MODEL_GROUPS_H
#define LMMS_MODEL_GROUPS_H

#include <cstddef>

#include "Model.h"

class QDomDocument;
class QDomElement;

namespace lmms {

/**
	@file ModelGroup.h
	See Lv2Proc.h for example usage
*/

class AutomatableModel;

/**
	Base class for a group of models

	Features:
	* Models are stored by their QObject::objectName
	* Load/Save routines
	* Add/Remove routines
*/
class LMMS_EXPORT ModelGroup
{
public:
	// models
	struct ModelInfo
	{
		QString m_name;
		AutomatableModel* m_model;
		ModelInfo() { /* hopefully no one will use this */ } // TODO: remove?
		ModelInfo(const QString& name, AutomatableModel* model)
			: m_name(name)
			, m_model(model)
		{
		}
	};

	// TODO: refactor those 2
	template <class Functor> void foreach_model(const Functor& ftor)
	{
		for (auto& [name, info] : m_models)
		{
			ftor(name, info);
		}
	}

	template <class Functor> void foreach_model(const Functor& ftor) const
	{
		for (const auto& [name, info] : m_models)
		{
			ftor(name, info);
		}
	}

protected:
	ModelGroup(Model* parent)
		: m_parent(parent)
	{
	}

	void saveSettings(class QDomDocument& doc, class QDomElement& that);
	void loadSettings(const class QDomElement& that);

	std::size_t size() const { return m_models.size(); }

	//! Register a further model
	void addModel(AutomatableModel* model, const QString& name);
	//! Remove a model
	void removeModel(AutomatableModel*);

private:
	//! models for the controls
	//! @note The AutomatableModels behind the ModelInfo are not owned,
	//!   but referenced after `addModel` is being called.
	std::map<std::string, ModelInfo> m_models;

	Model* m_parent;
};

} // namespace lmms

#endif // LMMS_MODEL_GROUPS_H
