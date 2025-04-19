/*
 * DetuningHelper.h - detuning automation helper
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_DETUNING_HELPER_H
#define LMMS_DETUNING_HELPER_H

#include "InlineAutomation.h"

namespace lmms
{

class DetuningHelper : public InlineAutomation
{
	Q_OBJECT
public:
	DetuningHelper() :
		InlineAutomation()
	{
	}
	DetuningHelper(const DetuningHelper& _copy) :
		InlineAutomation(_copy)
	{
	}

	~DetuningHelper() override = default;

	float defaultValue() const override
	{
		return 0;
	}

	QString displayName() const override
	{
		return tr( "Note detuning" );
	}

	inline QString nodeName() const override
	{
		return "detuning";
	}

} ;


} // namespace lmms

#endif // LMMS_DETUNING_HELPER_H
