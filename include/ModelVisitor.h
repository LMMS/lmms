/*
 * ModelVisitor.h - visitors for automatable models
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

#ifndef MODELVISITOR_H
#define MODELVISITOR_H

class AutomatableModel;
class BoolModel;
class IntModel;
class FloatModel;
class ComboBoxModel;
class TempoSyncKnobModel;

class ModelVisitor
{
	template<class ParentType = AutomatableModel, class ModelType>
	void up(ModelType& m) { visit(static_cast<ParentType&>(m)); }
public:
	virtual void visit(AutomatableModel& ) {}
	virtual void visit(BoolModel& m);
	virtual void visit(IntModel& m);
	virtual void visit(FloatModel& m);
	virtual void visit(ComboBoxModel& m);
	virtual void visit(TempoSyncKnobModel& m);
	virtual ~ModelVisitor();
};

class ConstModelVisitor
{
	template<class ParentType = AutomatableModel, class ModelType>
	void up(const ModelType& m) {
		visit(static_cast<const ParentType&>(m)); }
public:
	virtual void visit(const AutomatableModel& ) {}
	virtual void visit(const BoolModel& m);
	virtual void visit(const IntModel& m);
	virtual void visit(const FloatModel& m);
	virtual void visit(const ComboBoxModel& m);
	virtual void visit(const TempoSyncKnobModel& m);
	virtual ~ConstModelVisitor();
};

#endif // MODELVISITOR_H
