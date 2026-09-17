/*
 * PatmanTest.cpp
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <QApplication>
#include <QDir>
#include <QDomDocument>
#include <QTemporaryFile>
#include <QtTest>

#include "Engine.h"
#include "InstrumentTrack.h"
#include "Patman.h"
#include "PathUtil.h"
#include "ProjectJournal.h"
#include "Song.h"

namespace
{

QByteArray validPatchHeaderWithNoSamples()
{
	QByteArray header(239, '\0');
	const char signature[] = "GF1PATCH110\0ID#000002";
	memcpy(header.data(), signature, 22);
	header[82] = 1;
	header[151] = 1;
	header[198] = 0;
	return header;
}

QString createTempPatchFile( const QByteArray& bytes )
{
	QTemporaryFile file( QDir::tempPath() + "/patman-test-XXXXXX.pat" );
	file.setAutoRemove( false );
	if (!file.open())
	{
		return {};
	}

	file.write( bytes );
	file.flush();
	const QString filePath = file.fileName();
	file.close();
	return filePath;
}

QString patchFileFromInstrument( lmms::PatmanInstrument& instrument )
{
	QDomDocument doc;
	QDomElement element = doc.createElement( "instrument" );
	instrument.saveSettings( doc, element );
	return element.attribute( "src" );
}

}

class PatmanTest : public QObject
{
	Q_OBJECT

	void ensureJournalReady()
	{
		using namespace lmms;

		if (Engine::projectJournal() == nullptr)
		{
			Engine::destroy();
			Engine::init( true );
		}

		QVERIFY2( Engine::projectJournal() != nullptr,
			"ProjectJournal must be initialized before constructing InstrumentTrack" );
		Engine::projectJournal()->setJournalling( false );
	}

private slots:
	void initTestCase()
	{
		using namespace lmms;
		Engine::init( true );
		ensureJournalReady();
	}

	void cleanupTestCase()
	{
		using namespace lmms;
		Engine::destroy();
	}

	void missingPatchClearsFile()
	{
		using namespace lmms;
		ensureJournalReady();

		InstrumentTrack track( Engine::getSong() );
		track.setName( "Manual Name" );
		PatmanInstrument instrument( &track );

		instrument.setFile( "/tmp/does-not-exist-patman-test.pat" );

		QCOMPARE( patchFileFromInstrument( instrument ), QString() );
		QCOMPARE( track.name(), QString( "Manual Name" ) );
	}

	void invalidPatchClearsFile()
	{
		using namespace lmms;
		ensureJournalReady();

		const QString invalidPatchPath = createTempPatchFile( "not-a-valid-gus-patch" );
		QVERIFY( !invalidPatchPath.isEmpty() );

		InstrumentTrack track( Engine::getSong() );
		track.setName( "Manual Name" );
		PatmanInstrument instrument( &track );

		instrument.setFile( invalidPatchPath );

		QCOMPARE( patchFileFromInstrument( instrument ), QString() );
		QCOMPARE( track.name(), QString( "Manual Name" ) );

		QFile::remove( invalidPatchPath );
	}

	void playNoteWithNoLoadedSamplesDoesNotCrash()
	{
		using namespace lmms;
		ensureJournalReady();

		const QString validZeroSamplePatchPath = createTempPatchFile( validPatchHeaderWithNoSamples() );
		QVERIFY( !validZeroSamplePatchPath.isEmpty() );

		InstrumentTrack track( Engine::getSong() );
		PatmanInstrument instrument( &track );

		instrument.setFile( validZeroSamplePatchPath );
		QVERIFY( !patchFileFromInstrument( instrument ).isEmpty() );

		QTest::ignoreMessage(
			QtWarningMsg,
			QRegularExpression( "Patman: skipping note playback because patch has no loaded samples:.*" ) );
		instrument.playNote( nullptr, nullptr );

		QFile::remove( validZeroSamplePatchPath );
	}

	void manualTrackNameIsPreservedOnSuccessfulLoad()
	{
		using namespace lmms;
		ensureJournalReady();

		const QString validZeroSamplePatchPath = createTempPatchFile( validPatchHeaderWithNoSamples() );
		QVERIFY( !validZeroSamplePatchPath.isEmpty() );

		InstrumentTrack track( Engine::getSong() );
		track.setName( "Custom Manual Name" );
		PatmanInstrument instrument( &track );

		instrument.setFile( validZeroSamplePatchPath, true );

		QCOMPARE( track.name(), QString( "Custom Manual Name" ) );

		QFile::remove( validZeroSamplePatchPath );
	}

	void trackRenamesWhenPreviousPatchWasEmpty()
	{
		using namespace lmms;
		ensureJournalReady();

		const QString validZeroSamplePatchPath = createTempPatchFile( validPatchHeaderWithNoSamples() );
		QVERIFY( !validZeroSamplePatchPath.isEmpty() );

		InstrumentTrack track( Engine::getSong() );
		track.setName( "" );
		PatmanInstrument instrument( &track );

		instrument.setFile( validZeroSamplePatchPath, true );

		QCOMPARE( track.name(), PathUtil::cleanName( validZeroSamplePatchPath ) );

		QFile::remove( validZeroSamplePatchPath );
	}
};

int main( int argc, char** argv )
{
	qputenv( "QT_QPA_PLATFORM", QByteArray( "offscreen" ) );
	QApplication app( argc, argv );
	PatmanTest tc;
	return QTest::qExec( &tc, argc, argv );
}

#include "PatmanTest.moc"
