/*
 * ModelVisitor.cpp - visitors for automatable models
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

#include "ModelVisitor.h"

#include "AutomatableModel.h"
#include "ComboBoxModel.h"
#include "TempoSyncKnobModel.h"

namespace lmms
{


void ModelVisitor::visit(BoolModel &m) { up(m); }
void ModelVisitor::visit(IntModel &m) { up(m); }
void ModelVisitor::visit(FloatModel &m) { up(m); }
void ModelVisitor::visit(ComboBoxModel &m) { up<IntModel>(m); }
void ModelVisitor::visit(TempoSyncKnobModel &m) { up<FloatModel>(m); }

void ConstModelVisitor::visit(const BoolModel &m) { up(m); }
void ConstModelVisitor::visit(const IntModel &m) { up(m); }
void ConstModelVisitor::visit(const FloatModel &m) { up(m); }
void ConstModelVisitor::visit(const ComboBoxModel &m) { up<IntModel>(m); }
void ConstModelVisitor::visit(const TempoSyncKnobModel &m) { up<FloatModel>(m); }

} // namespace lmms