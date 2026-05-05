/*
 * MeterModel.h - model for meter specification
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_METER_MODEL_H
#define LMMS_METER_MODEL_H

#include "AutomatableModel.h"

namespace lmms
{


class LMMS_EXPORT MeterModel : public Model
{
	Q_OBJECT
	mapPropertyFromModel(int,getNumerator,setNumerator,m_numeratorModel);
	mapPropertyFromModel(int,getDenominator,setDenominator,m_denominatorModel);
public:
	MeterModel( Model * _parent );
	~MeterModel() override = default;

	void saveSettings( QDomDocument & _doc, QDomElement & _this,
						const QString & _name );
	void loadSettings( const QDomElement & _this, const QString & _name );

	void reset();

	// Must have the sub-models exposed to programmatically connect
	// to automation or controllers
	IntModel & numeratorModel()
	{
		return m_numeratorModel;
	}

	IntModel & denominatorModel()
	{
		return m_denominatorModel;
	}


private:
	IntModel m_numeratorModel;
	IntModel m_denominatorModel;

} ;


} // namespace lmms

#endif // LMMS_METER_MODEL_H
