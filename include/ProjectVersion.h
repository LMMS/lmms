/*
 * ProjectVersion.h - version compared in import upgrades
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2015 Tres Finocchiaro <tres.finocchiaro/at/gmail.com>
 *
 * This file is part of LMMS - http://lmms.io
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


#ifndef PROJECT_VERSION_H
#define PROJECT_VERSION_H

#include <QtCore/QString>

enum CompareType { Major, Minor, Release, Build };

/*! \brief Version number parsing and comparison 
 *
 *  Parses and compares version information.  i.e. "1.0.3" < "1.0.10"
 */
class ProjectVersion
{
public:
	ProjectVersion(QString version, CompareType c = CompareType::Build) : 
		m_version(version), 
		m_major(version.section( '.', 0, 0 ).toInt()) ,
		m_minor(version.section( '.', 1, 1 ).toInt()) ,
		m_release(version.section( '.', 2 ).section( '-', 0, 0 ).toInt()) ,
		m_build(version.section( '.', 2 ).section( '-', 1 )),
		m_compareType(c)
	{
	}

	static int compare(ProjectVersion v1, ProjectVersion v2);

	int getMajor() { return m_major; }
	int getMinor() { return m_minor; }
	int getRelease() { return m_release; }
	QString getBuild() { return m_build; }
	CompareType getCompareType() { return m_compareType; }
	void setCompareType(CompareType compareType) { m_compareType = compareType; }
	

private:
	QString m_version;
	int m_major;
	int m_minor;
	int m_release;
	QString m_build;
	CompareType m_compareType;
} ;




inline int compare(ProjectVersion v1, QString v2)
{
	return ProjectVersion::compare(v1, ProjectVersion(v2));
}




/*
 * ProjectVersion v. char[]
 */
inline bool operator<(ProjectVersion v1, QString v2) { return compare(v1, v2) < 0; }
inline bool operator>(ProjectVersion v1, QString v2) { return compare(v1, v2) > 0; }
inline bool operator<=(ProjectVersion v1, QString v2) { return compare(v1, v2) <= 0; }
inline bool operator>=(ProjectVersion v1, QString v2) { return compare(v1, v2) >= 0; }
inline bool operator==(ProjectVersion v1, QString v2) { return compare(v1, v2) == 0; }
inline bool operator!=(ProjectVersion v1, QString v2) { return compare(v1, v2) != 0; }

inline bool operator<(QString v1, ProjectVersion v2) { return 0 < compare(v2, v1); }
inline bool operator>(QString v1, ProjectVersion v2) { return 0 > compare(v2, v1); }
inline bool operator<=(QString v1, ProjectVersion v2) { return 0 <= compare(v2, v1); }
inline bool operator>=(QString v1, ProjectVersion v2) { return 0 >= compare(v2, v1); }
inline bool operator==(QString v1, ProjectVersion v2) { return 0 == compare(v2, v1); }
inline bool operator!=(QString v1, ProjectVersion v2) { return 0 != compare(v2, v1); }




/*
 * ProjectVersion v. ProjectVersion
 */
inline bool operator<(ProjectVersion & v1, ProjectVersion & v2) { return ProjectVersion::compare(v1, v2) < 0; }
inline bool operator>(ProjectVersion & v1, ProjectVersion & v2) { return ProjectVersion::compare(v1, v2) > 0; }
inline bool operator<=(ProjectVersion & v1, ProjectVersion & v2) { return ProjectVersion::compare(v1, v2) <= 0; }
inline bool operator>=(ProjectVersion & v1, ProjectVersion & v2) { return ProjectVersion::compare(v1, v2) >= 0; }
inline bool operator==(ProjectVersion & v1, ProjectVersion & v2) { return ProjectVersion::compare(v1, v2) == 0; }
inline bool operator!=(ProjectVersion & v1, ProjectVersion & v2) { return ProjectVersion::compare(v1, v2) != 0; }


#endif
