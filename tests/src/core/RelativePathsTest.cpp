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

#include <QDir>
#include <QObject>
#include <QtTest/QtTest>

#include "ConfigManager.h"
#include "PathUtil.h"
#include "SampleBuffer.h"

class RelativePathsTest : public QObject
{
	Q_OBJECT
private slots:
	void PathUtilComparisonTests()
	{
		using namespace lmms;

		QFileInfo fi(ConfigManager::inst()->factorySamplesDir() + "/drums/kick01.ogg");
		QVERIFY(fi.exists());

		QString absPath = fi.absoluteFilePath();
		QString oldRelPath = "drums/kick01.ogg";
		QString relPath = PathUtil::basePrefixQString(PathUtil::Base::FactorySample) + "drums/kick01.ogg";
		QString fuzPath = absPath;
		fuzPath.replace(relPath, "drums/.///kick01.ogg");

		//Test nicely formatted paths
		QCOMPARE(PathUtil::toShortestRelative(absPath), relPath);
		QCOMPARE(PathUtil::toAbsolute(relPath), absPath);

		//Test upgrading old paths
		QCOMPARE(PathUtil::toShortestRelative(oldRelPath), relPath);
		QCOMPARE(PathUtil::toAbsolute(oldRelPath), absPath);

		//Test weird but valid paths
		QCOMPARE(PathUtil::toShortestRelative(fuzPath), relPath);
		QCOMPARE(PathUtil::toAbsolute(fuzPath), absPath);

		//Empty paths should stay empty
		const QString empty = QString("");
		QCOMPARE(PathUtil::stripPrefix(empty), empty);
		QCOMPARE(PathUtil::cleanName(empty), empty);
		QCOMPARE(PathUtil::toAbsolute(empty), empty);
		QCOMPARE(PathUtil::toShortestRelative(empty), empty);
	}

	void PathUtilComparisonTestsUtf8()
	{
		using namespace lmms;

		QFileInfo fi(ConfigManager::inst()->factorySamplesDir() + "/drums/kick01.ogg");
		QVERIFY(fi.exists());

		std::string absPath = fi.absoluteFilePath().toStdString();
		std::string oldRelPath = "drums/kick01.ogg";
		std::string relPath = std::string{PathUtil::basePrefix(PathUtil::Base::FactorySample)} + "drums/kick01.ogg";

		auto replace = [](std::string& str, std::string_view from, std::string_view to) {
			const auto startPos = str.find(from);
			if (startPos == std::string::npos) { return; }
			str.replace(startPos, from.length(), to);
		};

		std::string fuzPath = absPath;
		replace(fuzPath, relPath, "drums/.///kick01.ogg");

		//Test nicely formatted paths
		QCOMPARE(PathUtil::toShortestRelative(absPath), relPath);
		QCOMPARE(PathUtil::toAbsolute(relPath).value(), absPath);

		//Test upgrading old paths
		QCOMPARE(PathUtil::toShortestRelative(oldRelPath), relPath);
		QCOMPARE(PathUtil::toAbsolute(oldRelPath).value(), absPath);

		//Test weird but valid paths
		QCOMPARE(PathUtil::toShortestRelative(fuzPath), relPath);
		QCOMPARE(PathUtil::toAbsolute(fuzPath).value(), absPath);

		//Empty paths should stay empty
		const auto empty = std::string_view{""};
		QCOMPARE(PathUtil::stripPrefix(empty), empty);
		QCOMPARE(PathUtil::cleanName(empty), empty);
		QCOMPARE(PathUtil::toAbsolute(empty).value(), empty);
		QCOMPARE(PathUtil::toShortestRelative(empty), empty);
	}
};

QTEST_GUILESS_MAIN(RelativePathsTest)
#include "RelativePathsTest.moc"
