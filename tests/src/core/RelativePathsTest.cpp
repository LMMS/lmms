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

#include <QObject>
#include <QtTest>

#include "ConfigManager.h"
#include "PathUtil.h"

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
		QString relPath = PathUtil::basePrefix(PathUtil::Base::FactorySample) + "drums/kick01.ogg";
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
		QString empty = QString("");
		QCOMPARE(PathUtil::stripPrefix(""), empty);
		QCOMPARE(PathUtil::cleanName(""), empty);
		QCOMPARE(PathUtil::toAbsolute(""), empty);
		QCOMPARE(PathUtil::toShortestRelative(""), empty);
	}

	//! Test UTF-8 encoded std::string --> std::filesystem:path conversion
	void StringToPathTest()
	{
		using namespace lmms;

		const auto qPath = ConfigManager::inst()->factorySamplesDir() + QString::fromUtf16(u"/fake/音楽.wav");
		const std::filesystem::path expected = qPath.toStdU16String();

		const std::filesystem::path actual = PathUtil::stringToPath(qPath.toStdString());

		const auto actualComparable = QString::fromStdU16String(actual.u16string());
		const auto expectedComparable = QString::fromStdU16String(expected.u16string());
		QCOMPARE(actualComparable, expectedComparable);
	}

	//! Test QString --> std::filesystem:path conversion
	void QStringToPathTest()
	{
		using namespace lmms;

		const auto qPath = ConfigManager::inst()->factorySamplesDir() + QString::fromUtf16(u"/fake/音楽.wav");
		const std::filesystem::path expected = qPath.toStdU16String();

		const std::filesystem::path actual = PathUtil::stringToPath(qPath);

		const auto actualComparable = QString::fromStdU16String(actual.u16string());
		const auto expectedComparable = QString::fromStdU16String(expected.u16string());
		QCOMPARE(actualComparable, expectedComparable);
	}

	//! Test std::filesystem:path --> UTF-8 encoded std::string conversion
	void PathToStringTest()
	{
		using namespace lmms;

		const auto expected = ConfigManager::inst()->factorySamplesDir() + QString::fromUtf16(u"/fake/音楽.wav");
		const std::filesystem::path filePath = expected.toStdU16String();

		const std::string actual = PathUtil::pathToString(filePath);

		const auto actualComparable = QString::fromStdString(actual);
		QCOMPARE(actualComparable, expected);
	}
};

QTEST_GUILESS_MAIN(RelativePathsTest)
#include "RelativePathsTest.moc"
