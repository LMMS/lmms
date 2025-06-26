/*
* ProjectProperties.cpp - implementation of class ProjectProperties, for project-specific settings
*
* Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
* Copyright (c) 2025 regulus79
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


#include <QDomElement>

#include "ProjectProperties.h"

namespace lmms
{


ProjectProperties * ProjectProperties::s_instanceOfMe = nullptr;


ProjectProperties::ProjectProperties()
{
}


QString ProjectProperties::value(const QString& cls, const QString& attribute, const QString& defaultVal) const
{
	if (m_settings.find(cls) != m_settings.end())
	{
		for (const auto& setting : m_settings[cls])
		{
			if (setting.first == attribute)
			{
				return setting.second;
			}
		}
	}
	return defaultVal;
}




void ProjectProperties::setValue(const QString & cls,
				const QString & attribute,
				const QString & value)
{
	if(m_settings.contains(cls))
	{
		for(QPair<QString, QString>& pair : m_settings[cls])
		{
			if(pair.first == attribute)
			{
				if (pair.second != value)
				{
					pair.second = value;
					emit valueChanged(cls, attribute, value);
				}
				return;
			}
		}
	}
	// not in map yet, so we have to add it...
	m_settings[cls].push_back(qMakePair(attribute, value));
}


void ProjectProperties::deleteValue(const QString & cls, const QString & attribute)
{
	if(m_settings.contains(cls))
	{
		for(stringPairVector::iterator it = m_settings[cls].begin();
					it != m_settings[cls].end(); ++it)
		{
			if((*it).first == attribute)
			{
				m_settings[cls].erase(it);
				return;
			}
		}
	}
}

void ProjectProperties::reset()
{
	m_settings.clear();
}

void ProjectProperties::saveSettings(QDomDocument& doc, QDomElement& element)
{
	for (auto it = m_settings.begin(); it != m_settings.end(); ++it)
	{
		QDomElement n = doc.createElement(it.key());
		for (const auto& [first, second] : *it)
		{
			n.setAttribute(first, second);
		}
		element.appendChild(n);
	}
}


void ProjectProperties::loadSettings(const QDomElement& element)
{
	QDomNode node = element.firstChild();

	while(!node.isNull())
	{
		if(node.isElement() && node.toElement().hasAttributes())
		{
			stringPairVector attr;
			QDomNamedNodeMap node_attr = node.toElement().attributes();
			for(int i = 0; i < node_attr.count(); ++i)
			{
				QDomNode n = node_attr.item(i);
				if(n.isAttr())
				{
					attr.push_back(qMakePair(n.toAttr().name(), n.toAttr().value()));
				}
			}
			m_settings[node.nodeName()] = attr;
		}
		node = node.nextSibling();
	}
}


} // namespace lmms
