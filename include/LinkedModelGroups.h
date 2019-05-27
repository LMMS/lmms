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
	//! @param curProc number of this processor, counted from 0
	//! @param nProc total number of processors
	LinkedModelGroup(Model* parent, int curProc) :
		Model(parent), m_curProc(curProc) {}
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
	bool isLinking() const { return m_linkEnabled.size(); }

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
		return m_linkEnabled[id];
	}
	const class BoolModel* linkEnabledModel(std::size_t id) const
	{
		return m_linkEnabled[id];
	}
	std::vector<ModelInfo>& models() { return m_models; }
	const std::vector<ModelInfo>& models() const { return m_models; }

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
	int curProc() const { return m_curProc; }

protected:
	//! Register a further model
	void addModel(class AutomatableModel* model, const QString& name);

private slots:
	//! Callback called after any of the per-control link-enabled models switch
	void linkStateChangedSlot();

private:
	//! models for the per-control link-enabled models
	std::vector<class BoolModel*> m_linkEnabled;
	//! models for the controls; the vector defines indices for the controls
	std::vector<ModelInfo> m_models;

	int m_curProc;
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
		if (multiChannelLinkModel())
		{
			connect(multiChannelLinkModel(), SIGNAL(dataChanged()),
				this, SLOT(updateLinkStatesFromGlobal()));
			connect(getGroup(0), SIGNAL(linkStateChanged(int, bool)),
					this, SLOT(linkPort(int, bool)));
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
	//! Take a specified port from the first Lv2Proc and link or unlink it
	//! from the associated port of every other Lv2Proc
	//! @param port number conforming to Lv2Proc::m_modelVector
	//! @param state True iff it should be linked
	void linkPort(int port, bool state);
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
