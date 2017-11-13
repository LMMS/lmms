/*
 * ProjectVersion.h - version compared in import upgrades
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2015 Tres Finocchiaro <tres.finocchiaro/at/gmail.com>
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


#ifndef PROJECT_VERSION_H
#define PROJECT_VERSION_H

#include <QtCore/QString>

/*! \brief Version number parsing and comparison
 *
 *  Parses and compares version information.  i.e. "1.0.3" < "1.0.10"
 */
class ProjectVersion
{
public:
	enum CompareType { Major, Minor, Release, Stage, Build };

	ProjectVersion(QString version, CompareType c = Build);
	ProjectVersion(const char * version, CompareType c = Build);

	int getMajor() const { return m_major; }
	int getMinor() const { return m_minor; }
	int getRelease() const { return m_release; }
	QString getStage() const { return m_stage; }
	int getBuild() const { return m_build; }
	CompareType getCompareType() const { return m_compareType; }
	ProjectVersion setCompareType(CompareType compareType) { m_compareType = compareType; return * this; }

	static int compare(const ProjectVersion& a, const ProjectVersion& b, CompareType c);
	static int compare(ProjectVersion v1, ProjectVersion v2);

private:
	QString m_version;
	int m_major;
	int m_minor;
	int m_release;
	QString m_stage;
	int m_build;
	CompareType m_compareType;
} ;

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
