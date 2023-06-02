/*
 * Model.cpp - implementation of Model base class
 *
 * Copyright (c) 2007-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "Model.h"

namespace lmms
{

Model::Model(Model* parent, QString displayName, bool defaultConstructed) :
	QObject(parent),
	m_displayName(displayName),
	m_defaultConstructed(defaultConstructed)
{
}

bool Model::isDefaultConstructed() const
{
	return m_defaultConstructed;
}

Model* Model::parentModel() const
{
	return dynamic_cast<Model*>(parent());
}

QString Model::displayName() const
{
	return m_displayName;
}

void Model::setDisplayName(const QString& displayName)
{
	m_displayName = displayName;
}

QString Model::fullDisplayName() const
{
	const QString n = displayName();

	if (parentModel())
	{
		const QString p = parentModel()->fullDisplayName();

		if (!p.isEmpty())
		{
			return p + ">" + n;
		}
	}

	return n;
}



} // namespace lmms

