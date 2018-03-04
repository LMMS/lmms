/*
 * RelativePathsTest.cpp
 *
 * Copyright (c) 2017 Tres Finocchiaro <tres/dot/finocchiaro/at/gmail.com>
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

#include "ConfigManager.h"
#include "SampleBuffer.h"

#include <QDir>

class RelativePathsTest : QTestSuite
{
	Q_OBJECT
private slots:
	void RelativePathComparisonTests()
	{
		QFileInfo fi(ConfigManager::inst()->factorySamplesDir() + "/drums/kick01.ogg");
		QVERIFY(fi.exists());

		QString absPath = fi.absoluteFilePath();
		QString relPath = "drums/kick01.ogg";
		QString fuzPath = absPath;
		fuzPath.replace(relPath, "drums/.///kick01.ogg");
		QCOMPARE(SampleBuffer::tryToMakeRelative(absPath), relPath);
		QCOMPARE(SampleBuffer::tryToMakeAbsolute(relPath), absPath);
		QCOMPARE(SampleBuffer::tryToMakeRelative(fuzPath), relPath);
	}
} RelativePathTests;

#include "RelativePathsTest.moc"
