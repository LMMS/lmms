/*
 * DataFileTest.cpp
 *
 * Copyright (c) 2024 Jonah Janzen
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

#include "DataFile.h"
typedef lmms::DataFile::Type Type;

#include <QtTest/QtTest>
#include <map>
#include <set>

// DataFile handles XML-based serialization / deserialization of LMMS objects.
// There are 9 possible types of DataFile. Most have specific file extensions associated with them.
// Possible future work item: Add file-based tests that actually read from and write to real-world project files.
class DataFileTest : public QObject
{
	Q_OBJECT
private slots:
	// The correct extensions should be associated with the correct file types.
	void validateTest()
	{
		// The valid extensions for each type of DataFile.
		const std::map<Type, std::set<std::string>> allowedExtensionMap = {
			{Type::SongProject, {"mmp", "mmpz"}},
			{Type::SongProjectTemplate, {"mpt"}},
			{Type::InstrumentTrackSettings, {"xpf", "xml"}},
			{Type::MidiClip, {"xpt", "xptz"}},
			{Type::Unknown, {"wav", "ogg", "ds", "mp3", "txt", "impossibleTestExtension", "!&&&**.test"}},
			// These next four are never associated with files on disk, so they have no valid extensions.
			{Type::ClipboardData, {}},
			{Type::DragNDropData, {}},
			{Type::EffectSettings, {}},
			{Type::JournalData, {}},
		};

		// All the extensions that could conceivably appear when dealing with files in LMMS, plus some extras thrown in
		// just to be on the safe side.
		const char* possibleExtensions[]
			= {"mmp", "mmpz", "mpt", "xpf", "xml", "xpt", "xpt", "xptz", "wav", "ogg", "ds", "mp3", "sf2", "sf3", "so",
				"dll", "pat", "mid", "lv2", "txt", "impossibleTestExtension", "!&&&**.test"};

		for (std::pair<Type, std::set<std::string>> typeExtensionsPair : allowedExtensionMap)
		{
			Type type = typeExtensionsPair.first;
			std::set<std::string> allowedExtensions = typeExtensionsPair.second;

			lmms::DataFile file(type);
			for (std::string ext : possibleExtensions)
			{
				if (allowedExtensions.find(ext) != allowedExtensions.end())
				{
					// If the extension is allowed for this DataFile type, it should pass validation.
					QVERIFY2(file.validate(QString::fromStdString(ext)),
						qPrintable(QString("The extension %1 is valid for a DataFile of type %2, but was incorrectly "
										   "reported to be invalid.")
									   .arg(QString::fromStdString(ext), QString::number((int)type))));
				}
				else
				{
					// If the extension is not allowed for this DataFile type, it should not pass validation.
					QVERIFY2(!file.validate(QString::fromStdString(ext)),
						qPrintable(QString("The extension %1 is invalid for a DataFile of type %2, but was incorrectly "
										   "reported to be valid.")
									   .arg(QString::fromStdString(ext), QString::number((int)type))));
				}
			}
		}
	}

	// DataFiles should be given the correct file extension when saving.
	void nameWithExtensionTest()
	{
		lmms::DataFile songProjectFile(Type::SongProject);
		QCOMPARE(songProjectFile.nameWithExtension("testProject1"), "testProject1.mmpz");
		QCOMPARE(songProjectFile.nameWithExtension("testProject1.mmpz"), "testProject1.mmpz");
		QCOMPARE(songProjectFile.nameWithExtension("123foobar!.mmp"), "123foobar!.mmp");

		lmms::DataFile songTemplateFile(Type::SongProjectTemplate);
		QCOMPARE(songTemplateFile.nameWithExtension("myFunTemplate13"), "myFunTemplate13.mpt");
		QCOMPARE(songTemplateFile.nameWithExtension("sample_template.mpt"), "sample_template.mpt");

		lmms::DataFile instrumentTrackFile(Type::InstrumentTrackSettings);
		QCOMPARE(instrumentTrackFile.nameWithExtension("*********"), "*********.xpf");
		QCOMPARE(instrumentTrackFile.nameWithExtension("tracksettings.xpf"), "tracksettings.xpf");

		lmms::DataFile otherFile(Type::Unknown);
		QCOMPARE(otherFile.nameWithExtension("examplesample.wav"), "examplesample.wav");
	}

	// Only benign local files of predetermined types should be loaded to minimize security risks.
	// Currently, LMMS only allows sample clips and audio file processors to include local paths.
	void loadLocalResourceTest()
	{
		lmms::DataFile legalFile(Type::SongProject);
		QDomElement sampleClipElement = legalFile.createElement("sampleclip");
		sampleClipElement.setAttribute("src", "local:mysample.wav");
		legalFile.documentElement().appendChild(sampleClipElement);
		QVERIFY2(!legalFile.hasLocalPlugins(),
			"A project containing a local sample should be allowed to load, but was not.");

		lmms::DataFile audioFileProcessorFile(Type::SongProject);
		QDomElement audioFileProcessorElement = audioFileProcessorFile.createElement("audiofileprocessor");
		audioFileProcessorElement.setAttribute("src", "local:an/audio/file.processor");
		audioFileProcessorFile.documentElement().appendChild(audioFileProcessorElement);
		QVERIFY2(!audioFileProcessorFile.hasLocalPlugins(),
			"A project containing a local audio file processor should be allowed to load, but was not.");

		lmms::DataFile illegalFile(Type::SongProject);
		QDomElement illegalElement = illegalFile.createElement("instrument");
		illegalElement.setAttribute("src", "local:maliciousplugin.lv2");
		illegalFile.documentElement().appendChild(illegalElement); // Make sure it works on nested elements.
		QVERIFY2(illegalFile.hasLocalPlugins(),
			"A project containing a local plugin was allowed to load when it should not have been.");
	}

	// A DataFile should be properly serialized to and reloaded from a text stream.
	void writeTextStreamTest()
	{
		QString output;
		QTextStream outputStream(&output);

		lmms::DataFile originalFile(Type::MidiClip);
		originalFile.write(outputStream);
		lmms::DataFile reloadedFile(output.toUtf8());
		QString reloadedOutput;
		QTextStream reloadedOutputStream(&reloadedOutput);
		reloadedFile.write(reloadedOutputStream);

		// Comparisons are string-based because QDomElement does not define an equality operator.
		// Additionally, serialization somehow consistently produces ' in the reloaded version and \" in the original
		// string.
		QCOMPARE(reloadedOutput.replace("'", "\""), output);
	}

	// A DataFile should be properly serialized to and reloaded from a file on disk.
	void writeFileTest()
	{
		char* tmpnam = std::tmpnam(nullptr);
		lmms::DataFile originalFile(Type::MidiClip);
		originalFile.writeFile(tmpnam);
		lmms::DataFile reloadedFile(QString::fromUtf8(tmpnam));
		std::remove(tmpnam);

		QString original;
		QTextStream originalStream(&original);
		originalFile.write(originalStream);
		QString reloaded;
		QTextStream reloadedStream(&reloaded);
		reloadedFile.write(reloadedStream);

		// Same comparison details as in above method.
		QCOMPARE(reloaded.replace("'", "\""), original);
	}

	// Should correctly detect a position in upgrade version history based on the file version of an old LMMS project.
	// Closely tied to ProjectVersionTest.
	void legacyFileVersionTest()
	{
		lmms::DataFile first(Type::Unknown);
		first.documentElement().setAttribute("creatorversion", "0.2.1-20070501");
		QCOMPARE(first.legacyFileVersion(), 1);

		lmms::DataFile zeroPointFour(Type::Unknown);
		zeroPointFour.documentElement().setAttribute("creatorversion", "0.4.5");
		QCOMPARE(zeroPointFour.legacyFileVersion(), 12);

		lmms::DataFile onePointZero(Type::Unknown);
		onePointZero.documentElement().setAttribute("creatorversion", "1.0.0");
		QCOMPARE(onePointZero.legacyFileVersion(), 12);

		lmms::DataFile onePointThree(Type::Unknown);
		onePointThree.documentElement().setAttribute("creatorversion", "1.3.0");
		QCOMPARE(onePointThree.legacyFileVersion(), 17);
	}

	// A DataFile should copy referenced resources to the local directory.
	// Because the tests do not currently operate on actual projects, we are not able to test whether a DataFile
	// referencing real resources can copy those resources.
	void copyResourcesTest()
	{
		lmms::DataFile blankFile(Type::Unknown);
		QVERIFY2(blankFile.copyResources(QString::fromUtf8(std::tmpnam(nullptr)) + "/"),
			"A file without resources should be able to copy its resources successfully.");

		lmms::DataFile songWithSample(Type::SongProject);
		QDomNode sampleClipNode;
		sampleClipNode.toElement().setTagName("sampleclip");
		sampleClipNode.toElement().setAttribute("src", "local:nonexistentfile.mp3");
		songWithSample.appendChild(sampleClipNode);
		QVERIFY2(!songWithSample.copyResources(std::tmpnam(nullptr)) + "/",
			"A file referencing a nonexistent sample should not be able to copy its resources.");
	}
};

QTEST_GUILESS_MAIN(DataFileTest)
#include "DataFileTest.moc"