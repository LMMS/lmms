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
class ProjectVersion : public QString
{
public:
	ProjectVersion(const QString & s, const CompareType c = CompareType::Build) : 
		QString(s), 
		m_major(s.section( '.', 0, 0 ).toInt()) ,
		m_minor(s.section( '.', 1, 1 ).toInt()) ,
		m_release(s.section( '.', 2 ).section( '-', 0, 0 ).toInt()) ,
		m_build(s.section( '.', 2 ).section( '-', 1 )),
		m_compareType(c)
	{
	}

	static int compare(const ProjectVersion & v1, const ProjectVersion & v2);

	const int getMajor() const { return m_major; }
	const int getMinor() const { return m_minor; }
	const int getRelease() const { return m_release; }
	const QString getBuild() const { return m_build; }
	const CompareType getCompareType() const { return m_compareType; }
	

private:
	static int compStr(const ProjectVersion & v1, const char * v2);
	static int compStr(const ProjectVersion & v1, const QString & v2);

	const int m_major;
	const int m_minor;
	const int m_release;
	const QString m_build;
	const CompareType m_compareType;
} ;




inline int compStr(const ProjectVersion & v1, const char * v2)
{
	return ProjectVersion::compare(v1, ProjectVersion(v2));
}



inline int compStr(const ProjectVersion & v1, const QString & v2)
{
	return ProjectVersion::compare(v1, ProjectVersion(v2));
}



inline int compStr(const char * v1, const ProjectVersion & v2)
{
	return ProjectVersion::compare(ProjectVersion(v1), v2);
}



inline int compStr(const QString & v1, const ProjectVersion & v2)
{
	return ProjectVersion::compare(ProjectVersion(v1), v2);
}



/*
 * ProjectVersion v. char[]
 */
inline bool operator<(const ProjectVersion & v1, const char * v2) { return compStr(v1, v2) < 0; }
inline bool operator>(const ProjectVersion & v1, const char * v2) { return compStr(v1, v2) > 0; }
inline bool operator<=(const ProjectVersion & v1, const char * v2) { return compStr(v1, v2) <= 0; }
inline bool operator>=(const ProjectVersion & v1, const char * v2) { return compStr(v1, v2) >= 0; }
inline bool operator==(const ProjectVersion & v1, const char * v2) { return compStr(v1, v2) == 0; }
inline bool operator!=(const ProjectVersion & v1, const char * v2) { return compStr(v1, v2) != 0; }

inline bool operator<(const char * v1, const ProjectVersion & v2) { return compStr(v1, v2) < 0; }
inline bool operator>(const char * v1, const ProjectVersion & v2) { return compStr(v1, v2) > 0; }
inline bool operator<=(const char * v1, const ProjectVersion & v2) { return compStr(v1, v2) <= 0; }
inline bool operator>=(const char * v1, const ProjectVersion & v2) { return compStr(v1, v2) >= 0; }
inline bool operator==(const char * v1, const ProjectVersion & v2) { return compStr(v1, v2) == 0; }
inline bool operator!=(const char * v1, const ProjectVersion & v2) { return compStr(v1, v2) != 0; }

/*
 * ProjectVersion v. QString
 */
inline bool operator<(const ProjectVersion & v1, const QString & v2) { return compStr(v1, v2) < 0; }
inline bool operator>(const ProjectVersion & v1, const QString & v2) { return compStr(v1, v2) > 0; }
inline bool operator<=(const ProjectVersion & v1, const QString & v2) { return compStr(v1, v2) <= 0; }
inline bool operator>=(const ProjectVersion & v1, const QString & v2) { return compStr(v1, v2) >= 0; }
inline bool operator==(const ProjectVersion & v1, const QString & v2) { return compStr(v1, v2) == 0; }
inline bool operator!=(const ProjectVersion & v1, const QString & v2) { return compStr(v1, v2) != 0; }

inline bool operator<(const QString & v1, const ProjectVersion & v2) { return compStr(v1, v2) < 0; }
inline bool operator>(const QString & v1, const ProjectVersion & v2) { return compStr(v1, v2) > 0; }
inline bool operator<=(const QString & v1, const ProjectVersion & v2) { return compStr(v1, v2) <= 0; }
inline bool operator>=(const QString & v1, const ProjectVersion & v2) { return compStr(v1, v2) >= 0; }
inline bool operator==(const QString & v1, const ProjectVersion & v2) { return compStr(v1, v2) == 0; }
inline bool operator!=(const QString & v1, const ProjectVersion & v2) { return compStr(v1, v2) != 0; }

/*
 * ProjectVersion v. ProjectVersion
 */
inline bool operator<(const ProjectVersion & v1, const ProjectVersion & v2) { return ProjectVersion::compare(v1, v2) < 0; }
inline bool operator>(const ProjectVersion & v1, const ProjectVersion & v2) { return ProjectVersion::compare(v1, v2) > 0; }
inline bool operator<=(const ProjectVersion & v1, const ProjectVersion & v2) { return ProjectVersion::compare(v1, v2) <= 0; }
inline bool operator>=(const ProjectVersion & v1, const ProjectVersion & v2) { return ProjectVersion::compare(v1, v2) >= 0; }
inline bool operator==(const ProjectVersion & v1, const ProjectVersion & v2) { return ProjectVersion::compare(v1, v2) == 0; }
inline bool operator!=(const ProjectVersion & v1, const ProjectVersion & v2) { return ProjectVersion::compare(v1, v2) != 0; }


#endif
