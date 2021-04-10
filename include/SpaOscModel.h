/*
 * SpaOscModel.h - AutomatableModel which forwards OSC events
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

#ifndef SPA_OSC_MODEL_H
#define SPA_OSC_MODEL_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SPA

#include "AutomatableModel.h"

template <class Base> class SpaOscModel : public Base
{
protected:
	class SpaProc *m_plugRef;
	QByteArray m_dest;

	using Base::Base;
	void init(class SpaProc *plugRef, const QString dest)
	{
		m_plugRef = plugRef;
		m_dest = dest.toUtf8();
	}
};

class BoolOscModel : public SpaOscModel<BoolModel>
{
	Q_OBJECT
private slots:
	void sendOsc();

public:
	BoolOscModel(SpaProc *plugRef, const QString dest, bool val);
};

class IntOscModel : public SpaOscModel<IntModel>
{
	Q_OBJECT
private slots:
	void sendOsc();

public:
	IntOscModel(class SpaProc *plugRef, const QString dest, int min,
		int max, int val);
};

class FloatOscModel : public SpaOscModel<FloatModel>
{
	Q_OBJECT
private slots:
	void sendOsc();

public:
	FloatOscModel(class SpaProc *plugRef, const QString dest,
		float min, float max, float step, float val);
};

#endif // LMMS_HAVE_SPA

#endif // SPA_OSC_MODEL_H
