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

#include <QDebug>
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

		auto fi = QFileInfo{ConfigManager::inst()->factorySamplesDir() + "/drums/kick01.ogg"};
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
		qDebug() << "fuzPath:" << QString::fromUtf8(fuzPath.c_str(), fuzPath.size());

		const QString relPathQ = QString::fromStdString(relPath);
		const QString absPathQ = QString::fromStdString(absPath);

		//Test nicely formatted paths
		QCOMPARE(QString::fromStdString(PathUtil::toShortestRelative(absPath)), relPathQ);
		QCOMPARE(QString::fromStdString(PathUtil::toAbsolute(relPath).value()), absPathQ);

		//Test upgrading old paths
		QCOMPARE(QString::fromStdString(PathUtil::toShortestRelative(oldRelPath)), relPathQ);
		QCOMPARE(QString::fromStdString(PathUtil::toAbsolute(oldRelPath).value()), absPathQ);

		//Test weird but valid paths
		QCOMPARE(QString::fromStdString(PathUtil::toShortestRelative(fuzPath)), relPathQ);
		QCOMPARE(QString::fromStdString(PathUtil::toAbsolute(fuzPath).value()), absPathQ);

		//Empty paths should stay empty
		const auto empty = std::string_view{""};
		const auto [base, prefixStripped] = PathUtil::parsePath(empty);
		QCOMPARE(base, PathUtil::Base::Absolute);
		QCOMPARE(QString::fromUtf8(prefixStripped.data(), prefixStripped.size()), "");
		QCOMPARE(QString::fromStdString(PathUtil::cleanName(empty)), "");
		QCOMPARE(QString::fromStdString(PathUtil::toAbsolute(empty).value()), "");
		QCOMPARE(QString::fromStdString(PathUtil::toShortestRelative(empty)), "");
	}

	void PathUtilComparisonTestsQString()
	{
		using namespace lmms;

		auto fi = QFileInfo{ConfigManager::inst()->factorySamplesDir() + "/drums/kick01.ogg"};
		QVERIFY(fi.exists());

		QString absPath = fi.absoluteFilePath();
		QString oldRelPath = "drums/kick01.ogg";
		const std::string_view basePrefix = PathUtil::basePrefix(PathUtil::Base::FactorySample);
		QString relPath = QString::fromUtf8(basePrefix.data(), basePrefix.size()) + "drums/kick01.ogg";
		QString fuzPath = absPath;
		fuzPath.replace(relPath, "drums/.///kick01.ogg");
		qDebug() << "fuzPath:" << fuzPath;

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
		QCOMPARE(PathUtil::cleanName(empty), empty);
		QCOMPARE(PathUtil::toAbsolute(empty), empty);
		QCOMPARE(PathUtil::toShortestRelative(empty), empty);
	}

	void cleanNameTests()
	{
		using namespace lmms;

		QString kickQ = ConfigManager::inst()->factorySamplesDir() + "/drums/kick01.ogg";
		auto kick = kickQ.toStdString();

		QCOMPARE(PathUtil::cleanName(kickQ), "kick01");
		QCOMPARE(QString::fromStdString(PathUtil::cleanName(kick)), "kick01");

		auto testQ = QString{"usersample:foo/success.abc.def.ghi"};
		auto test = testQ.toStdString();

		QCOMPARE(PathUtil::cleanName(testQ), "success.abc.def");
		QCOMPARE(QString::fromStdString(PathUtil::cleanName(test)), "success.abc.def");

		auto noNameQ = QString{"foo/bar/.extension"};
		auto noName = noNameQ.toStdString();

		QCOMPARE(PathUtil::cleanName(noNameQ), "");
		QCOMPARE(QString::fromStdString(PathUtil::cleanName(noName)), "");

		auto noExtensionQ = QString{"../../meow"};
		auto noExtension = noExtensionQ.toStdString();

		QCOMPARE(PathUtil::cleanName(noExtensionQ), "meow");
		QCOMPARE(QString::fromStdString(PathUtil::cleanName(noExtension)), "meow");
	}
};

QTEST_GUILESS_MAIN(RelativePathsTest)
#include "RelativePathsTest.moc"
