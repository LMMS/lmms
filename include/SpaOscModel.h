/*
 * SpaOscModel.h - AutomatableModel which forwards OSC events
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

#ifndef SPAOSCMODEL_H
#define SPAOSCMODEL_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SPA

#include "AutomatableModel.h"

template <class Base> class SpaOscModel : public Base
{
protected:
	class SpaInstrument *m_instRef;
	QByteArray m_dest;

	using Base::Base;
	void init(class SpaInstrument *instRef, const QString dest)
	{
		m_instRef = instRef;
		m_dest = dest.toUtf8();
	}
};

class BoolOscModel : public SpaOscModel<BoolModel>
{
	Q_OBJECT
private slots:
	void sendOsc();

public:
	BoolOscModel(
		class SpaInstrument *instRef, const QString dest, bool val);
};

class IntOscModel : public SpaOscModel<IntModel>
{
	Q_OBJECT
private slots:
	void sendOsc();

public:
	IntOscModel(class SpaInstrument *instRef, const QString dest, int min,
		int max, int val);
};

class FloatOscModel : public SpaOscModel<FloatModel>
{
	Q_OBJECT
private slots:
	void sendOsc();

public:
	FloatOscModel(class SpaInstrument *instRef, const QString dest,
		float min, float max, float val);
};

struct SpaOscModelFactory
{
	AutomatableModel *m_res;
	SpaOscModelFactory(class SpaInstrument *instRef, const QString &dest);
};

#endif // LMMS_HAVE_SPA

#endif // SPAOSCMODEL_H
