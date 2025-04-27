/*
* ProjectProperties.h - class ProjectProperties, a class for managing project-specific settings
*
* Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_PROJECT_PROPERTIES_H
#define LMMS_PROJECT_PROPERTIES_H

#include "JournallingObject.h"

#include <QMap>
#include <QObject>
#include <QPair>
#include <QStringList>

#include <vector>
#include "lmms_export.h"
#include "lmmsconfig.h"


namespace lmms
{


class LMMS_EXPORT ProjectProperties : public QObject, public JournallingObject
{
	Q_OBJECT
public:
	static inline ProjectProperties * inst()
	{
		if(s_instanceOfMe == nullptr )
		{
			s_instanceOfMe = new ProjectProperties();
		}
		return s_instanceOfMe;
	}

	QString value(const QString& cls, const QString& attribute, const QString& defaultVal = "") const;

	void setValue(const QString & cls, const QString & attribute, const QString & value);

	void deleteValue(const QString & cls, const QString & attribute);

	void reset();


	QString nodeName() const override
	{
		return "projectproperties";
	}

	void saveSettings(QDomDocument& doc, QDomElement& element) override;
	void loadSettings(const QDomElement& element) override;

signals:
	void valueChanged( QString cls, QString attribute, QString value );

private:
	static ProjectProperties * s_instanceOfMe;

	ProjectProperties();
	ProjectProperties(const ProjectProperties & _c);

	using stringPairVector = std::vector<QPair<QString, QString>>;
	using settingsMap = QMap<QString, stringPairVector>;
	settingsMap m_settings;
};


} // namespace lmms

#endif // LMMS_PROJECT_PROPERTIES_H
