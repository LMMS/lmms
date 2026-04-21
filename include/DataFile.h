/*
 * DataFile.h - class for reading and writing LMMS data files
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2012-2013 Paul Giblock <p/at/pgiblock.net>
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

#ifndef LMMS_DATA_FILE_H
#define LMMS_DATA_FILE_H

#include <map>
#include <QDomDocument>
#include <vector>

#include "lmms_export.h"

class QTextStream;

namespace lmms
{

class ProjectVersion;


class LMMS_EXPORT DataFile : public QDomDocument
{

	using UpgradeMethod = void(DataFile::*)();

public:
	enum class Type
	{
		Unknown,
		SongProject,
		SongProjectTemplate,
		InstrumentTrackSettings,
		DragNDropData,
		ClipboardData,
		JournalData,
		EffectSettings,
		MidiClip
	} ;

	DataFile( const QString& fileName );
	DataFile( const QByteArray& data );
	DataFile( Type type );

	virtual ~DataFile() = default;

	//! @brief Performs basic validation, compared to file extension
	bool validate( QString extension );

	QString nameWithExtension( const QString& fn ) const;

	void write( QTextStream& strm );
	bool writeFile(const QString& fn, bool withResources = false);

	//! @brief Copies resources to @p resourcesDir and changes the
	//! DataFile to use local paths to them
	bool copyResources(const QString& resourcesDir);

	//! @brief Checks whether the XML tree of a @ref DataFile has (potentially unsafe) local paths
	//!
	//! This recursive method will go through all XML nodes of the DataFile and check whether any of them have local paths.
	//! If they are not on our list of elements that can have local paths we return true, indicating that we potentially
	//! have plugins with local paths that would be a security issue. The Song class can then abort loading this project.
	//! @param parent The parent node being iterated. When called without arguments, this will be an empty element that
	//! will be ignored (since the second parameter will be true).
	//! @param firstCall Defaults to true, and indicates to this recursive method whether this is the first call. If it is
	//! it will use the root element as the parent.
	bool hasLocalPlugins(QDomElement parent = QDomElement(), bool firstCall = true) const;

	QDomElement& content()
	{
		return m_content;
	}

	QDomElement& head()
	{
		return m_head;
	}

	Type type() const
	{
		return m_type;
	}

	unsigned int legacyFileVersion();

private:
	static Type type( const QString& typeName );
	static QString typeName( Type type );

	void cleanMetaNodes( QDomElement de );

	void mapSrcAttributeInElementsWithResources(const QMap<QString, QString>& map);

	// helper upgrade routines
	void upgrade_0_2_1_20070501();
	void upgrade_0_2_1_20070508();
	void upgrade_0_3_0_rc2();
	void upgrade_0_3_0();
	void upgrade_0_4_0_20080104();
	void upgrade_0_4_0_20080118();
	void upgrade_0_4_0_20080129();
	void upgrade_0_4_0_20080409();
	void upgrade_0_4_0_20080607();
	void upgrade_0_4_0_20080622();
	void upgrade_0_4_0_beta1();
	void upgrade_0_4_0_rc2();
	void upgrade_1_0_99();
	void upgrade_1_1_0();
	void upgrade_1_1_91();
	void upgrade_1_2_0_rc3();
	void upgrade_1_3_0();
	void upgrade_noHiddenClipNames();
	void upgrade_automationNodes();

	//! @brief Note range has been extended to match MIDI specification
	//!
	//! The non-standard note range previously affected all MIDI-based instruments
	//! except OpulenZ, and made them sound an octave lower than they should (#1857).
	void upgrade_extendedNoteRange();

	//! @brief TripleOscillator switched to using high-quality, alias-free oscillators by default
	//!
	//! Older projects were made without this feature and would sound differently if loaded
	//! with the new default setting. This upgrade routine preserves their old behavior.
	void upgrade_defaultTripleOscillatorHQ();

	void upgrade_mixerRename();
	void upgrade_bbTcoRename();
	void upgrade_sampleAndHold();

	//! Update MIDI CC indexes, so that they are counted from 0. Older releases of LMMS count the CCs from 1.
	void upgrade_midiCCIndexing();

	void upgrade_loopsRename();
	void upgrade_noteTypes();
	void upgrade_fixCMTDelays();
	void upgrade_fixBassLoopsTypo();
	void findProblematicLadspaPlugins();
	void upgrade_noHiddenAutomationTracks();

	// List of all upgrade methods
	static const std::vector<UpgradeMethod> UPGRADE_METHODS;
	// List of ProjectVersions for the legacyFileVersion method
	static const std::vector<ProjectVersion> UPGRADE_VERSIONS;

	// Map with DOM elements that access resources (for making bundles)
	using ResourcesMap = std::map<QString, std::vector<QString>>;
	static const ResourcesMap ELEMENTS_WITH_RESOURCES;

	void upgrade();

	void loadData( const QByteArray & _data, const QString & _sourceFile );

	QString m_fileName; //!< The origin file name or "" if this DataFile didn't originate from a file
	QDomElement m_content;
	QDomElement m_head;
	Type m_type;
	unsigned int m_fileVersion;
} ;


} // namespace lmms

#endif // LMMS_DATA_FILE_H
