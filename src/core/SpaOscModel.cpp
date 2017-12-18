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

#include <QDebug>
#include <spa/audio.h>
#include "SpaOscModel.h"
#include "spa/SpaInstrument.h"

struct SpaOscModelFactoryVisitor : public spa::audio::visitor
{
	class SpaInstrument* inst_ref;
	const QString* dest;
	AutomatableModel* res = nullptr;

	template<class ModelType, class T>
	void make(T min, T max, T val) {
		res = new ModelType(inst_ref, *dest, min, max, val);
	}

	template<class T> using c_in = spa::audio::control_in<T>;
	virtual void visit(c_in<float>& in) {
		make<FloatOscModel>(in.min, in.max, in.def); }
	virtual void visit(c_in<double>& in) {
		make<FloatOscModel>(in.min, in.max, in.def); }
	virtual void visit(c_in<int>& in) {
		make<IntOscModel>(in.min, in.max, in.def); }
	virtual void visit(c_in<bool>& in) {
		res = new BoolOscModel(inst_ref, *dest, in.def); }
};

SpaOscModelFactory::SpaOscModelFactory(SpaInstrument *inst_ref,
	const QString& dest)
{
	SpaOscModelFactoryVisitor vis;
	vis.inst_ref = inst_ref;
	vis.dest = &dest;
	spa::port_ref_base& base = inst_ref->plugin->port(dest.toUtf8().data());
	base.accept(vis);
	res = vis.res;
}

void BoolOscModel::sendOsc() { inst_ref->writeOsc(dest.data(),
				value() ? "T" : "F"); }
void IntOscModel::sendOsc() { inst_ref->writeOsc(dest.data(), "i", value()); }
void FloatOscModel::sendOsc() { inst_ref->writeOsc(dest.data(), "f", value()); }

BoolOscModel::BoolOscModel(SpaInstrument *inst_ref, const QString dest,
	bool val)
	: SpaOscModel<BoolModel>(val, nullptr, dest, false)
{
	qDebug() << "LMMS: receiving bool model: val = " << val;
	init(inst_ref, dest);
	QObject::connect(this, SIGNAL(dataChanged()), this, SLOT(sendOsc()));
}

IntOscModel::IntOscModel(SpaInstrument *inst_ref, const QString dest,
	int min, int max, int val)
	: SpaOscModel<IntModel>(val, min, max, nullptr, dest, false)
{
	qDebug() << "LMMS: receiving int model: (val, min, max) = ("
		<< val << ", " << min << ", " << max << ")";
	init(inst_ref, dest);
	QObject::connect(this, SIGNAL(dataChanged()), this, SLOT(sendOsc()));
}

FloatOscModel::FloatOscModel(SpaInstrument *inst_ref, const QString dest,
	float min, float max, float val)
	: SpaOscModel<FloatModel>(val, min, max, 0.1f, nullptr, dest, false)
	/* TODO: get step from plugin (we currently need a plugin where this
		can be tested) */
{
	qDebug() << "LMMS: receiving float model: (val, min, max) = ("
		<< val << ", " << min << ", " << max << ")";
	init(inst_ref, dest);
	QObject::connect(this, SIGNAL(dataChanged()), this, SLOT(sendOsc()));
}
