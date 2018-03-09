/*
 * Groove.h - classes for addinng swing/funk/groove/slide (you can't name it but you can feel it)
 *  to midi which is not precise enough at 192 ticks per tact to make your arse move.
 *
 * In it simplest terms a groove is a subtle delay on some notes in a pattern.
 *
 * Copyright (c) 2005-2008 teknopaul <teknopaul/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
 **/
#ifndef GROOVEFACTORY_H
#define GROOVEFACTORY_H

#include "Groove.h"

class GrooveFactory
{

public:
	static Groove * create(QString type);

private:
	GrooveFactory();
};

#endif // GROOVEFACTORY_H
