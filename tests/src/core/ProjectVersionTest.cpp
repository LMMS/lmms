/*
 * ProjectVersionTest.cpp
 *
 * Copyright (c) 2015 Lukas W <lukaswhl/at/gmail.com>
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

#include "QTestSuite.h"

#include "ProjectVersion.h"

class ProjectVersionTest : QTestSuite
{
	Q_OBJECT
private slots:
        void ProjectVersionComparaisonTests()
        {
                QVERIFY(ProjectVersion("1.1.0", CompareType::Minor) > "1.0.3");
                QVERIFY(ProjectVersion("1.1.0", CompareType::Major) < "2.1.0");
                QVERIFY(ProjectVersion("1.1.0", CompareType::Release) > "0.2.1");
                QVERIFY(ProjectVersion("1.1.4", CompareType::Release) < "1.1.10");
                QVERIFY(ProjectVersion("1.1.0", CompareType::Minor) == "1.1.5");
                QVERIFY( ! ( ProjectVersion("3.1.0", CompareType::Minor) < "2.2.5" ) );
                QVERIFY( ! ( ProjectVersion("2.5.0", CompareType::Release) < "2.2.5" ) );
				QVERIFY(ProjectVersion("1.1.0") > "1.1.0-alpha");
				QVERIFY(ProjectVersion("1.1.0-alpha") < "1.1.0-beta");
        }
} ProjectVersionTests;

#include "ProjectVersionTest.moc"
