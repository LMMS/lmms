/*
 * SpaOscModel.cpp - AutomatableModel which forwards OSC events
 *
 * Copyright (c) 2018-2018 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "SpaOscModel.h"

#include <QDebug>

#include "SpaPluginBase.h"

void BoolOscModel::sendOsc()
{
	m_plugRef->writeOsc(m_dest.data(), value() ? "T" : "F");
}
void IntOscModel::sendOsc()
{
	m_plugRef->writeOsc(m_dest.data(), "i", value());
}
void FloatOscModel::sendOsc()
{
	m_plugRef->writeOsc(m_dest.data(), "f", value());
}

BoolOscModel::BoolOscModel(SpaPluginBase *plugRef, const QString dest, bool val) :
	SpaOscModel<BoolModel>(val, nullptr, dest, false)
{
	qDebug() << "LMMS: receiving bool model: val = " << val;
	init(plugRef, dest);
	QObject::connect(this, SIGNAL(dataChanged()), this, SLOT(sendOsc()));
}

IntOscModel::IntOscModel(SpaPluginBase *plugRef, const QString dest, int min,
	int max, int val) :
	SpaOscModel<IntModel>(val, min, max, nullptr, dest, false)
{
	qDebug() << "LMMS: receiving int model: (val, min, max) = (" << val
		 << ", " << min << ", " << max << ")";
	init(plugRef, dest);
	QObject::connect(this, SIGNAL(dataChanged()), this, SLOT(sendOsc()));
}

FloatOscModel::FloatOscModel(SpaPluginBase *plugRef, const QString dest,
	float min, float max, float val) :
	SpaOscModel<FloatModel>(val, min, max, 0.1f, nullptr, dest, false)
/* TODO: get step from plugin (we currently need a plugin where this
	can be tested) */
{
	qDebug() << "LMMS: receiving float model: (val, min, max) = (" << val
		 << ", " << min << ", " << max << ")";
	init(plugRef, dest);
	QObject::connect(this, SIGNAL(dataChanged()), this, SLOT(sendOsc()));
}
