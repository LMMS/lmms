/*
 * SpaOscModel.cpp - AutomatableModel which forwards OSC events
 *
 * Copyright (c) 2018-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#ifdef LMMS_HAVE_SPA

#include <QDebug>

#include "SpaProc.h"

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

BoolOscModel::BoolOscModel(SpaProc *plugRef, const QString dest, bool val) :
	SpaOscModel<BoolModel>(val, nullptr, dest, false)
{
	qDebug() << "LMMS: receiving bool model: val = " << val;
	init(plugRef, dest);
	QObject::connect(this, SIGNAL(dataChanged()), this, SLOT(sendOsc()));
}

IntOscModel::IntOscModel(SpaProc *plugRef, const QString dest, int min,
	int max, int val) :
	SpaOscModel<IntModel>(val, min, max, nullptr, dest, false)
{
	qDebug() << "LMMS: receiving int model: (val, min, max) = (" << val
		 << ", " << min << ", " << max << ")";
	init(plugRef, dest);
	QObject::connect(this, SIGNAL(dataChanged()), this, SLOT(sendOsc()));
}

FloatOscModel::FloatOscModel(SpaProc *plugRef, const QString dest,
	float min, float max, float step, float val) :
	// Ctor for FloatModel (see using clause in SpaOscModel)
	SpaOscModel<FloatModel>(val, min, max, step, nullptr, dest, false)
{
	qDebug() << "LMMS: receiving float model: (val, min, max) = (" << val
		 << ", " << min << ", " << max << ")";
	init(plugRef, dest);
	QObject::connect(this, SIGNAL(dataChanged()), this, SLOT(sendOsc()));
}

#endif // LMMS_HAVE_SPA
