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
#include <spa/audio.h>

#include "SpaInstrument.h"

struct SpaOscModelFactoryVisitor : public spa::audio::visitor
{
	class SpaInstrument *m_instRef;
	const QString *m_dest;
	AutomatableModel *m_res = nullptr;

	template <class ModelType, class T> void make(T min, T max, T val)
	{
		m_res = new ModelType(m_instRef, *m_dest, min, max, val);
	}

	template <class T> using CtlIn = spa::audio::control_in<T>;
	virtual void visit(CtlIn<float> &in)
	{
		make<FloatOscModel>(in.min, in.max, in.def);
	}
	virtual void visit(CtlIn<double> &in)
	{
		make<FloatOscModel>(in.min, in.max, in.def);
	}
	virtual void visit(CtlIn<int> &in)
	{
		make<IntOscModel>(in.min, in.max, in.def);
	}
	virtual void visit(CtlIn<bool> &in)
	{
		m_res = new BoolOscModel(m_instRef, *m_dest, in.def);
	}
};

SpaOscModelFactory::SpaOscModelFactory(
	SpaInstrument *instRef, const QString &dest)
{
	SpaOscModelFactoryVisitor vis;
	vis.m_instRef = instRef;
	vis.m_dest = &dest;
	spa::port_ref_base &base = instRef->m_plugin->port(dest.toUtf8().data());
	base.accept(vis);
	m_res = vis.m_res;
}

void BoolOscModel::sendOsc()
{
	m_instRef->writeOsc(m_dest.data(), value() ? "T" : "F");
}
void IntOscModel::sendOsc()
{
	m_instRef->writeOsc(m_dest.data(), "i", value());
}
void FloatOscModel::sendOsc()
{
	m_instRef->writeOsc(m_dest.data(), "f", value());
}

BoolOscModel::BoolOscModel(SpaInstrument *instRef, const QString dest, bool val) :
	SpaOscModel<BoolModel>(val, nullptr, dest, false)
{
	qDebug() << "LMMS: receiving bool model: val = " << val;
	init(instRef, dest);
	QObject::connect(this, SIGNAL(dataChanged()), this, SLOT(sendOsc()));
}

IntOscModel::IntOscModel(SpaInstrument *instRef, const QString dest, int min,
	int max, int val) :
	SpaOscModel<IntModel>(val, min, max, nullptr, dest, false)
{
	qDebug() << "LMMS: receiving int model: (val, min, max) = (" << val
		 << ", " << min << ", " << max << ")";
	init(instRef, dest);
	QObject::connect(this, SIGNAL(dataChanged()), this, SLOT(sendOsc()));
}

FloatOscModel::FloatOscModel(SpaInstrument *instRef, const QString dest,
	float min, float max, float val) :
	SpaOscModel<FloatModel>(val, min, max, 0.1f, nullptr, dest, false)
/* TODO: get step from plugin (we currently need a plugin where this
	can be tested) */
{
	qDebug() << "LMMS: receiving float model: (val, min, max) = (" << val
		 << ", " << min << ", " << max << ")";
	init(instRef, dest);
	QObject::connect(this, SIGNAL(dataChanged()), this, SLOT(sendOsc()));
}
