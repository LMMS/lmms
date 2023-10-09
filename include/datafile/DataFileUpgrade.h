/*
 * DataFileUpgrade.h - base functor for DataFile upgrade routines
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

#pragma once

#ifndef LMMS_DATAFILEUPGRADE_H
#define LMMS_DATAFILEUPGRADE_H

#include "DataFile.h"

#include "lmms_basics.h"

namespace lmms
{


// Base functor with some helper methods
class DataFileUpgrade
{
public:
	DataFileUpgrade(DataFile& document) : m_document(document) {}

	virtual void upgrade() {}

	void operator()() { upgrade(); }

	// wrappers
	QDomNodeList elementsByTagName( const QString& tagName );
	QDomElement createElement(const QString& tagName);
	QDomElement documentElement() const;
	QDomElement firstChildElement(const QString& tagName) const;
	QDomElement firstChildElement() const;

	// helper functions
	void renameElements(QDomNodeList& elements, const QString& newTagName);
	void renameAttribute(QDomNodeList& elements, const QString& oldName, const QString& newName);

protected:
	DataFile& m_document;
};


void findIds(const QDomElement& elem, QList<jo_id_t>& idList);


} // namespace lmms

#endif // LMMS_DATAFILEUPGRADE_H
