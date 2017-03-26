/*
 * ProjectVersionTest.cpp
 *
 * Copyright (c) 2015 Lukas W <lukaswhl/at/gmail.com>
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

#include "QTestSuite.h"

#include "ProjectVersion.h"

class ProjectVersionTest : QTestSuite
{
	Q_OBJECT
private slots:
	void ProjectVersionComparisonTests()
	{
		QVERIFY(ProjectVersion("1.1.0", ProjectVersion::Minor) > "1.0.3");
		QVERIFY(ProjectVersion("1.1.0", ProjectVersion::Major) < "2.1.0");
		QVERIFY(ProjectVersion("1.1.0", ProjectVersion::Release) > "0.2.1");
		QVERIFY(ProjectVersion("1.1.4", ProjectVersion::Release) < "1.1.10");
		QVERIFY(ProjectVersion("1.1.0", ProjectVersion::Minor) == "1.1.5");
		QVERIFY( ! ( ProjectVersion("3.1.0", ProjectVersion::Minor) < "2.2.5" ) );
		QVERIFY( ! ( ProjectVersion("2.5.0", ProjectVersion::Release) < "2.2.5" ) );
		QVERIFY(ProjectVersion("1.1.0") > "1.1.0-alpha");
		QVERIFY(ProjectVersion("1.1.0-alpha") < "1.1.0-beta");
		QVERIFY(ProjectVersion("1.2.0-rc1") < "1.2.0-rc2");
	}
} ProjectVersionTests;

#include "ProjectVersionTest.moc"
