/*
 * DataFileUpgrade.cpp - base functor for DataFile upgrade routines
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

#include "datafile/DataFileUpgrade.h"

#include <QDomElement>
#include <QList>

#include <set>
#include <cassert>


namespace lmms
{


// wrappers
QDomNodeList DataFileUpgrade::elementsByTagName( const QString& tagName ) {
	return m_document.elementsByTagName(tagName);
}
QDomElement DataFileUpgrade::createElement(const QString& tagName) {
	return m_document.createElement(tagName);
}
QDomElement DataFileUpgrade::documentElement() const {
	return m_document.documentElement();
}
QDomElement DataFileUpgrade::firstChildElement(const QString& tagName) const {
	return m_document.firstChildElement(tagName);
}
QDomElement DataFileUpgrade::firstChildElement() const {
	return m_document.firstChildElement();
}


// helpers
void DataFileUpgrade::renameElements(QDomNodeList& elements, const QString& newTagName)
{
	for (int i = 0; i < elements.length(); ++i)
	{
		auto item = elements.item(i).toElement();
		if (item.isNull()) { continue; }
		item.setTagName(newTagName);
	}
}


void DataFileUpgrade::renameAttribute(QDomNodeList& elements, const QString& oldName, const QString& newName)
{
	for (int i = 0; i < elements.length(); ++i)
	{
		auto item = elements.item(i).toElement();
		if (item.isNull() || !item.hasAttribute(oldName)) { continue; }
		// make before break
		item.setAttribute(newName, item.attribute(oldName));
		item.removeAttribute(oldName);
	}
}


void findIds(const QDomElement& elem, QList<jo_id_t>& idList)
{
	if(elem.hasAttribute("id"))
	{
		idList.append(elem.attribute("id").toInt());
	}
	QDomElement child = elem.firstChildElement();
	while(!child.isNull())
	{
		findIds(child, idList);
		child = child.nextSiblingElement();
	}
}


} // namespace lmms
