/*
 * DataFile.cpp - implementation of class DataFile
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2012-2013 Paul Giblock    <p/at/pgiblock.net>
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

#include <algorithm>
#include <cmath>
#include <map>

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

#include "base64.h"
#include "ConfigManager.h"
#include "Effect.h"
#include "embed.h"
#include "GuiApplication.h"
#include "LocaleHelper.h"
#include "PluginFactory.h"
#include "ProjectVersion.h"
#include "SongEditor.h"
#include "TextFloat.h"
#include "Track.h"
#include "PathUtil.h"

#include "lmmsversion.h"

// upgrade functors
#include "datafile/UpgradeTo02.h"
#include "datafile/UpgradeTo03.h"
#include "datafile/UpgradeTo04.h"
#include "datafile/UpgradeTo1Pre3.h"
#include "datafile/UpgradeTo1_3_0.h"
#include "datafile/UpgradeExtendedNoteRange.h"
#include "datafile/UpgradeMixerRename.h"
#include "datafile/UpgradeRenameBBTCO.h"
#include "datafile/UpgradeSampleAndHold.h"

namespace lmms
{


// QMap with the DOM elements that access file resources
const DataFile::ResourcesMap DataFile::ELEMENTS_WITH_RESOURCES = {
{ "sampleclip", {"src"} },
{ "audiofileprocessor", {"src"} },
};

// Vector with all the upgrade methods
const std::vector<DataFile::UpgradeMethod> DataFile::UPGRADE_METHODS = {
	//&DataFile::upgrade_0_2_1_20070501   ,   &DataFile::upgrade_0_2_1_20070508,
	//&DataFile::upgrade_0_3_0_rc2        ,   &DataFile::upgrade_0_3_0,
	/*
	&DataFile::upgrade_0_4_0_20080104   ,   &DataFile::upgrade_0_4_0_20080118,
	&DataFile::upgrade_0_4_0_20080129   ,   &DataFile::upgrade_0_4_0_20080409,
	&DataFile::upgrade_0_4_0_20080607   ,   &DataFile::upgrade_0_4_0_20080622,
	&DataFile::upgrade_0_4_0_beta1      ,   &DataFile::upgrade_0_4_0_rc2,
	*/
	/*
	&DataFile::upgrade_1_0_99           ,   &DataFile::upgrade_1_1_0,
	&DataFile::upgrade_1_1_91           ,   &DataFile::upgrade_1_2_0_rc3,
	*/
	//&DataFile::upgrade_1_3_0            ,
	&DataFile::upgrade_noHiddenClipNames,
	&DataFile::upgrade_automationNodes  ,//   &DataFile::upgrade_extendedNoteRange,
	&DataFile::upgrade_defaultTripleOscillatorHQ,
	//&DataFile::upgrade_mixerRename      ,   &DataFile::upgrade_bbTcoRename,
	//&DataFile::upgrade_sampleAndHold    ,
};

// Vector of all versions that have upgrade routines.
const std::vector<ProjectVersion> DataFile::UPGRADE_VERSIONS = {
	"0.2.1-20070501"   ,   "0.2.1-20070508"   ,   "0.3.0-rc2",
	"0.3.0"            ,   "0.4.0-20080104"   ,   "0.4.0-20080118",
	"0.4.0-20080129"   ,   "0.4.0-20080409"   ,   "0.4.0-20080607",
	"0.4.0-20080622"   ,   "0.4.0-beta1"      ,   "0.4.0-rc2",
	"1.0.99-0"         ,   "1.1.0-0"          ,   "1.1.91-0",
	"1.2.0-rc3"        ,   "1.3.0"
};

namespace
{
	struct TypeDescStruct
	{
		DataFile::Type m_type;
		QString m_name;
	};

	const auto s_types = std::array{
		TypeDescStruct{ DataFile::Type::Unknown, "unknown" },
		TypeDescStruct{ DataFile::Type::SongProject, "song" },
		TypeDescStruct{ DataFile::Type::SongProjectTemplate, "songtemplate" },
		TypeDescStruct{ DataFile::Type::InstrumentTrackSettings, "instrumenttracksettings" },
		TypeDescStruct{ DataFile::Type::DragNDropData, "dnddata" },
		TypeDescStruct{ DataFile::Type::ClipboardData, "clipboard-data" },
		TypeDescStruct{ DataFile::Type::JournalData, "journaldata" },
		TypeDescStruct{ DataFile::Type::EffectSettings, "effectsettings" },
		TypeDescStruct{ DataFile::Type::MidiClip, "midiclip" }
	};
}




DataFile::DataFile( Type type ) :
	QDomDocument( "lmms-project" ),
	m_fileName(""),
	m_content(),
	m_head(),
	m_type( type ),
	m_fileVersion( DATAFILE_VERSION )
{
	appendChild( createProcessingInstruction("xml", "version=\"1.0\""));
	QDomElement root = createElement( "lmms-project" );
	root.setAttribute( "version", m_fileVersion );
	root.setAttribute( "type", typeName( type ) );
	root.setAttribute( "creator", "LMMS" );
	root.setAttribute( "creatorversion", LMMS_VERSION );
	appendChild( root );

	m_head = createElement( "head" );
	root.appendChild( m_head );

	m_content = createElement( typeName( type ) );
	root.appendChild( m_content );

}




DataFile::DataFile( const QString & _fileName ) :
	QDomDocument(),
	m_fileName(_fileName),
	m_content(),
	m_head(),
	m_fileVersion( UPGRADE_METHODS.size() )
{
	QFile inFile( _fileName );
	if( !inFile.open( QIODevice::ReadOnly ) )
	{
		if (gui::getGUI() != nullptr)
		{
			QMessageBox::critical( nullptr,
				gui::SongEditor::tr( "Could not open file" ),
				gui::SongEditor::tr( "Could not open file %1. You probably "
						"have no permissions to read this "
						"file.\n Please make sure to have at "
						"least read permissions to the file "
						"and try again." ).arg( _fileName ) );
		}

		return;
	}

	loadData( inFile.readAll(), _fileName );
}




DataFile::DataFile( const QByteArray & _data ) :
	QDomDocument(),
	m_fileName(""),
	m_content(),
	m_head(),
	m_fileVersion( UPGRADE_METHODS.size() )
{
	loadData( _data, "<internal data>" );
}





bool DataFile::validate( QString extension )
{
	switch( m_type )
	{
	case Type::SongProject:
		if( extension == "mmp" || extension == "mmpz" )
		{
			return true;
		}
		break;
	case Type::SongProjectTemplate:
		if(  extension == "mpt" )
		{
			return true;
		}
		break;
	case Type::InstrumentTrackSettings:
		if ( extension == "xpf" || extension == "xml" )
		{
			return true;
		}
		break;
	case Type::MidiClip:
		if (extension == "xpt" || extension == "xptz")
		{
			return true;
		}
		break;
	case Type::Unknown:
		if (! ( extension == "mmp" || extension == "mpt" || extension == "mmpz" ||
				extension == "xpf" || extension == "xml" ||
				( extension == "xiz" && ! getPluginFactory()->pluginSupportingExtension(extension).isNull()) ||
				extension == "sf2" || extension == "sf3" || extension == "pat" || extension == "mid" ||
				extension == "dll"
#ifdef LMMS_BUILD_LINUX
				|| extension == "so"
#endif
#ifdef LMMS_HAVE_LV2
				|| extension == "lv2"
#endif
				) )
		{
			return true;
		}
		if( extension == "wav" || extension == "ogg" ||
				extension == "ds" )
		{
			return true;
		}
		break;
	default:
		return false;
	}
	return false;
}




QString DataFile::nameWithExtension( const QString & _fn ) const
{
	const QString extension = _fn.section( '.', -1 );

	switch( type() )
	{
		case Type::SongProject:
			if( extension != "mmp" &&
					extension != "mpt" &&
					extension != "mmpz" )
			{
				if( ConfigManager::inst()->value( "app",
						"nommpz" ).toInt() == 0 )
				{
					return _fn + ".mmpz";
				}
				return _fn + ".mmp";
			}
			break;
		case Type::SongProjectTemplate:
			if( extension != "mpt" )
			{
				return _fn + ".mpt";
			}
			break;
		case Type::InstrumentTrackSettings:
			if( extension != "xpf" )
			{
				return _fn + ".xpf";
			}
			break;
		default: ;
	}
	return _fn;
}




void DataFile::write( QTextStream & _strm )
{
	if( type() == Type::SongProject || type() == Type::SongProjectTemplate
					|| type() == Type::InstrumentTrackSettings )
	{
		cleanMetaNodes( documentElement() );
	}

	save(_strm, 2);
}




bool DataFile::writeFile(const QString& filename, bool withResources)
{
	// Small lambda function for displaying errors
	auto showError = [this](QString title, QString body){
		if (gui::getGUI() != nullptr)
		{
			QMessageBox mb;
			mb.setWindowTitle(title);
			mb.setText(body);
			mb.setIcon(QMessageBox::Warning);
			mb.setStandardButtons(QMessageBox::Ok);
			mb.exec();
		}
		else
		{
			qWarning() << body;
		}
	};

	// If we are saving without resources, filename is just the file we are
	// saving to. If we are saving with resources (project bundle), filename
	// will be used (discarding extensions) to create a folder where the
	// bundle will be saved in

	QFileInfo fInfo(filename);

	const QString bundleDir = fInfo.path() + "/" + fInfo.fileName().section('.', 0, 0);
	const QString resourcesDir = bundleDir + "/resources";
	const QString fullName = withResources
		? nameWithExtension(bundleDir + "/" + fInfo.fileName())
		: nameWithExtension(filename);
	const QString fullNameTemp = fullName + ".new";
	const QString fullNameBak = fullName + ".bak";

	using gui::SongEditor;

	// If we are saving with resources, setup the bundle folder first
	if (withResources)
	{
		// First check if there's a bundle folder with the same name in
		// the path already. If so, warns user that we can't overwrite a
		// project bundle.
		if (QDir(bundleDir).exists())
		{
			showError(SongEditor::tr("Operation denied"),
				SongEditor::tr("A bundle folder with that name already eists on the "
				"selected path. Can't overwrite a project bundle. Please select a different "
				"name."));

			return false;
		}

		// Create bundle folder
		if (!QDir().mkdir(bundleDir))
		{
			showError(SongEditor::tr("Error"),
				SongEditor::tr("Couldn't create bundle folder."));
			return false;
		}

		// Create resources folder
		if (!QDir().mkdir(resourcesDir))
		{
			showError(SongEditor::tr("Error"),
				SongEditor::tr("Couldn't create resources folder."));
			return false;
		}

		// Copy resources to folder and update paths
		if (!copyResources(resourcesDir))
		{
			showError(SongEditor::tr("Error"),
				SongEditor::tr("Failed to copy resources."));
			return false;
		}
	}

	QFile outfile (fullNameTemp);

	if (!outfile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		showError(SongEditor::tr("Could not write file"),
			SongEditor::tr("Could not open %1 for writing. You probably are not permitted to"
				"write to this file. Please make sure you have write-access to "
				"the file and try again.").arg(fullName));

		return false;
	}

	const QString extension = fullName.section('.', -1);
	if (extension == "mmpz" || extension == "xptz")
	{
		QString xml;
		QTextStream ts( &xml );
		write( ts );
		outfile.write( qCompress( xml.toUtf8() ) );
	}
	else
	{
		QTextStream ts( &outfile );
		write( ts );
	}

	outfile.close();

	// make sure the file has been written correctly
	if( QFileInfo( outfile.fileName() ).size() > 0 )
	{
		if( ConfigManager::inst()->value( "app", "disablebackup" ).toInt() )
		{
			// remove current file
			QFile::remove( fullName );
		}
		else
		{
			// remove old backup file
			QFile::remove( fullNameBak );
			// move current file to backup file
			QFile::rename( fullName, fullNameBak );
		}
		// move temporary file to current file
		QFile::rename( fullNameTemp, fullName );

		return true;
	}

	return false;
}




bool DataFile::copyResources(const QString& resourcesDir)
{
	// List of filenames used so we can append a counter to any
	// repeating filenames
	std::list<QString> namesList;

	auto it = ELEMENTS_WITH_RESOURCES.begin();

	// Copy resources and manipulate the DataFile to have local paths to them
	while (it != ELEMENTS_WITH_RESOURCES.end())
	{
		QDomNodeList list = elementsByTagName(it->first);

		// Go through all elements with the tagname from our map
		for (int i = 0; !list.item(i).isNull(); ++i)
		{
			QDomElement el = list.item(i).toElement();

			auto res = it->second.begin();

			// Search for attributes that point to resources
			while (res != it->second.end())
			{
				// If the element has that attribute
				if (el.hasAttribute(*res))
				{
					// Get absolute path to resource
					bool error;
					QString resPath = PathUtil::toAbsolute(el.attribute(*res), &error);
					// If we are running without the project loaded (from CLI), "local:" base
					// prefixes aren't converted, so we need to convert it ourselves
					if (error)
					{
						resPath = QFileInfo(m_fileName).path() + "/" + resPath.remove(0,
							PathUtil::basePrefix(PathUtil::Base::LocalDir).length());
					}

					// Check if we need to add a counter to the filename
					QString finalFileName = QFileInfo(resPath).fileName();
					QString extension = resPath.section('.', -1);
					int repeatedNames = 0;
					for (QString name : namesList)
					{
						if (finalFileName == name)
						{
							++repeatedNames;
						}
					}
					// Add the name to the list before modifying it
					namesList.push_back(finalFileName);
					if (repeatedNames)
					{
						// Remove the extension, add the counter and add the
						// extension again to get the final file name
						finalFileName.truncate(finalFileName.lastIndexOf('.'));
						finalFileName = finalFileName + "-" + QString::number(repeatedNames) + "." + extension;
					}

					// Final path is our resources dir + the new file name
					QString finalPath = resourcesDir + "/" + finalFileName;

					// Copy resource file to the resources folder
					if(!QFile::copy(resPath, finalPath))
					{
						qWarning("ERROR: Failed to copy resource");
						return false;
					}

					// Update attribute path to point to the bundle file
					QString newAtt = PathUtil::basePrefix(PathUtil::Base::LocalDir) + "resources/" + finalFileName;
					el.setAttribute(*res, newAtt);
				}
				++res;
			}
		}
		++it;
	}

	return true;
}




/**
 * @brief This recursive method will go through all XML nodes of the DataFile
 *        and check whether any of them have local paths. If they are not on
 *        our list of elements that can have local paths we return true,
 *        indicating that we potentially have plugins with local paths that
 *        would be a security issue. The Song class can then abort loading
 *        this project.
 * @param parent The parent node being iterated. When called
 *        without arguments, this will be an empty element that will be
 *        ignored (since the second parameter will be true).
 * @param firstCall Defaults to true, and indicates to this recursive
 *        method whether this is the first call. If it is it will use the
 *        root element as the parent.
 */
bool DataFile::hasLocalPlugins(QDomElement parent /* = QDomElement()*/, bool firstCall /* = true*/) const
{
	// If this is the first iteration of the recursion we use the root element
	if (firstCall) { parent = documentElement(); }

	auto children = parent.childNodes();
	for (int i = 0; i < children.size(); ++i)
	{
		QDomNode child = children.at(i);
		QDomElement childElement = child.toElement();

		bool skipNode = false;
		// Skip the nodes allowed to have "local:" attributes, but
		// still check its children
		for (const auto& element : ELEMENTS_WITH_RESOURCES)
		{
			if (childElement.tagName() == element.first)
			{
				skipNode = true;
				break;
			}
		}

		// Check if they have "local:" attribute (unless they are allowed to
		// and skipNode is true)
		if (!skipNode)
		{
			auto attributes = childElement.attributes();
			for (int i = 0; i < attributes.size(); ++i)
			{
				QDomNode attribute = attributes.item(i);
				QDomAttr attr = attribute.toAttr();
				if (attr.value().startsWith(PathUtil::basePrefix(PathUtil::Base::LocalDir),
					Qt::CaseInsensitive))
				{
					return true;
				}
			}
		}

		// Now we check the children of this node (recursively)
		// and if any return true we return true.
		if (hasLocalPlugins(childElement, false))
		{
			return true;
		}
	}

	// If we got here none of the nodes had the "local:" path.
	return false;
}




DataFile::Type DataFile::type( const QString& typeName )
{
	const auto it = std::find_if(s_types.begin(), s_types.end(),
		[&typeName](const TypeDescStruct& type) { return type.m_name == typeName; });
	if (it != s_types.end()) { return it->m_type; }

	// compat code
	if( typeName == "channelsettings" )
	{
		return Type::InstrumentTrackSettings;
	}

	return Type::Unknown;
}


QString DataFile::typeName( Type type )
{
	return s_types[static_cast<std::size_t>(type)].m_name;
}


void DataFile::cleanMetaNodes( QDomElement _de )
{
	QDomNode node = _de.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.toElement().attribute( "metadata" ).toInt() )
			{
				QDomNode ns = node.nextSibling();
				_de.removeChild( node );
				node = ns;
				continue;
			}
			if( node.hasChildNodes() )
			{
				cleanMetaNodes( node.toElement() );
			}
		}
		node = node.nextSibling();
	}
}


void DataFile::upgrade_noHiddenClipNames()
{
	QDomNodeList tracks = elementsByTagName("track");

	auto clearDefaultNames = [](QDomNodeList clips, QString trackName)
	{
		for (int j = 0; j < clips.size(); ++j)
		{
			QDomElement clip = clips.item(j).toElement();
			QString clipName = clip.attribute("name", "");
			if (clipName == trackName) { clip.setAttribute("name", ""); }
		}
	};

	for (int i = 0; i < tracks.size(); ++i)
	{
		QDomElement track = tracks.item(i).toElement();
		QString trackName = track.attribute("name", "");

		QDomNodeList instClips = track.elementsByTagName("pattern");
		QDomNodeList autoClips = track.elementsByTagName("automationpattern");
		QDomNodeList bbClips = track.elementsByTagName("bbtco");

		clearDefaultNames(instClips, trackName);
		clearDefaultNames(autoClips, trackName);
		clearDefaultNames(bbClips, trackName);
	}
}

void DataFile::upgrade_automationNodes()
{
	QDomNodeList autoPatterns = elementsByTagName("automationpattern");

	// Go through all automation patterns
	for (int i = 0; i < autoPatterns.size(); ++i)
	{
		QDomElement autoPattern = autoPatterns.item(i).toElement();

		// On each automation pattern, get all <time> elements
		QDomNodeList times = autoPattern.elementsByTagName("time");

		// Loop through all <time> elements and change what we need
		for (int j=0; j < times.size(); ++j)
		{
			QDomElement el = times.item(j).toElement();

			float value = LocaleHelper::toFloat(el.attribute("value"));

			// inValue will be equal to "value" and outValue will
			// be set to the same
			el.setAttribute("outValue", value);
		}
	}
}


/** \brief Note range has been extended to match MIDI specification
 *
 * The non-standard note range previously affected all MIDI-based instruments
 * except OpulenZ, and made them sound an octave lower than they should (#1857).
 */
void DataFile::upgrade_extendedNoteRange()
{
	UpgradeExtendedNoteRange upgrader(*this);

	upgrader.upgrade();
}


/** \brief TripleOscillator switched to using high-quality, alias-free oscillators by default
 *
 * Older projects were made without this feature and would sound differently if loaded
 * with the new default setting. This upgrade routine preserves their old behavior.
 */
void DataFile::upgrade_defaultTripleOscillatorHQ()
{
	QDomNodeList tripleoscillators = elementsByTagName("tripleoscillator");
	for (int i = 0; !tripleoscillators.item(i).isNull(); i++)
	{
		for (int j = 1; j <= 3; j++)
		{
			// Only set the attribute if it does not exist (default template has it but reports as 1.2.0)
			if (tripleoscillators.item(i).toElement().attribute("useWaveTable" + QString::number(j)) == "")
			{
				tripleoscillators.item(i).toElement().setAttribute("useWaveTable" + QString::number(j), 0);
			}
		}
	}
}


// Remove FX prefix from mixer and related nodes
//void DataFile::upgrade_mixerRename()


// Rename BB to pattern and TCO to clip
//void DataFile::upgrade_bbTcoRename()


// Set LFO speed to 0.01 on projects made before sample-and-hold PR
//void DataFile::upgrade_sampleAndHold()

/*
template<class Up>
void update(QDomDocument& document, const int start, int& i)
{
    if (i >= start)
	{
        Up{document}();
    } 
    ++i;
}


template<class... Ups>
void updaters (QDomDocument& document, const unsigned int start)
{
	int i = 0;
	// This fold expression instances and calls each update functor in order
    // begining with index "start"
    ( update<Ups>(document, start, i), ... );
}
*/


void DataFile::upgrade()
{
	// Runs all necessary upgrade methods
	unsigned int start = m_fileVersion;
	
	upgrade<UpgradeTo0_2_1_20070501>(0,start);
	upgrade<UpgradeTo0_2_1_20070508>(1,start);
	upgrade<UpgradeTo0_3_0_RC2>(2,start);
	upgrade<UpgradeTo0_3_0>(3,start);
	upgrade<UpgradeTo0_4_0_20080104>(4,start);
	upgrade<UpgradeTo0_4_0_20080118>(5,start);
	upgrade<UpgradeTo0_4_0_20080129>(6,start);
	upgrade<UpgradeTo0_4_0_20080409>(7,start);
	upgrade<UpgradeTo0_4_0_20080607>(8,start);
	upgrade<UpgradeTo0_4_0_20080622>(9,start);
	upgrade<UpgradeTo0_4_0_beta1>(10,start);
	upgrade<UpgradeTo0_4_0_rc2>(11,start);
	upgrade<UpgradeTo1_0_99>(12,start);
	upgrade<UpgradeTo1_1_0>(13,start);
	upgrade<UpgradeTo1_1_91>(14,start);
	upgrade<UpgradeTo1_2_0_RC3>(15,start);
	upgrade<UpgradeTo1_3_0>(16,start);
	upgrade<UpgradeExtendedNoteRange>(19,start);
	upgrade<UpgradeMixerRename>(21,start);
	upgrade<UpgradeRenameBBTCO>(22,start);
	upgrade<UpgradeSampleAndHold>(23,start);
	// Add your upgrader functor here...
	// and increment DATAFILE_VERSION in DataFile.h

	// Bump the file version (which should be the size of the upgrade methods vector)
	m_fileVersion = DATAFILE_VERSION;

	// update document meta data
	documentElement().setAttribute( "version", m_fileVersion );
	documentElement().setAttribute( "type", typeName( type() ) );
	documentElement().setAttribute( "creator", "LMMS" );
	documentElement().setAttribute( "creatorversion", LMMS_VERSION );

	if( type() == Type::SongProject || type() == Type::SongProjectTemplate )
	{
		// Time-signature
		if ( !m_head.hasAttribute( "timesig_numerator" ) )
		{
			m_head.setAttribute( "timesig_numerator", 4 );
			m_head.setAttribute( "timesig_denominator", 4 );
		}

		if( !m_head.hasAttribute( "mastervol" ) )
		{
			m_head.setAttribute( "mastervol", 100 );
		}
	}
}


void DataFile::loadData( const QByteArray & _data, const QString & _sourceFile )
{
	QString errorMsg;
	int line = -1, col = -1;
	if( !setContent( _data, &errorMsg, &line, &col ) )
	{
		// parsing failed? then try to uncompress data
		QByteArray uncompressed = qUncompress( _data );
		if( !uncompressed.isEmpty() )
		{
			if( setContent( uncompressed, &errorMsg, &line, &col ) )
			{
				line = col = -1;
			}
		}
		if( line >= 0 && col >= 0 )
		{
			using gui::SongEditor;

			qWarning() << "at line" << line << "column" << errorMsg;
			if (gui::getGUI() != nullptr)
			{
				QMessageBox::critical( nullptr,
					SongEditor::tr( "Error in file" ),
					SongEditor::tr( "The file %1 seems to contain "
							"errors and therefore can't be "
							"loaded." ).
								arg( _sourceFile ) );
			}

			return;
		}
	}

	QDomElement root = documentElement();
	m_type = type( root.attribute( "type" ) );
	m_head = root.elementsByTagName( "head" ).item( 0 ).toElement();

	if (!root.hasAttribute("version") || root.attribute("version")=="1.0")
	{
		// The file versioning is now a unsigned int, not maj.min, so we use
		// legacyFileVersion() to retrieve the appropriate version
		m_fileVersion = legacyFileVersion();
	}
	else
	{
		bool success;
		m_fileVersion = root.attribute( "version" ).toUInt( &success );
		if( !success ) qWarning("File Version conversion failure.");
	}

	if (root.hasAttribute("creatorversion"))
	{
		using gui::SongEditor;

		// compareType defaults to All, so it doesn't have to be set here
		ProjectVersion createdWith = root.attribute("creatorversion");
		ProjectVersion openedWith = LMMS_VERSION;

		if (createdWith.setCompareType(ProjectVersion::CompareType::Minor)
		 !=  openedWith.setCompareType(ProjectVersion::CompareType::Minor)
		 && gui::getGUI() != nullptr && root.attribute("type") == "song"
		){
			auto projectType = _sourceFile.endsWith(".mpt") ?
				SongEditor::tr("template") : SongEditor::tr("project");

			gui::TextFloat::displayMessage(
				SongEditor::tr("Version difference"),
				SongEditor::tr("This %1 was created with LMMS %2")
				.arg(projectType).arg(createdWith.getVersion()),
				embed::getIconPixmap("whatsthis", 24, 24),
				2500
			);
		}
	}

	// Perform upgrade routines
	if (m_fileVersion < UPGRADE_METHODS.size()) { upgrade(); }

	m_content = root.elementsByTagName(typeName(m_type)).item(0).toElement();
}


unsigned int DataFile::legacyFileVersion()
{
	// Version of LMMs that created this project
	ProjectVersion creator =
		documentElement().attribute( "creatorversion" ).
		replace( "svn", "" );

	// Get an iterator pointing at the first upgrade we need to run (or at the end if there is no such upgrade)
	auto firstRequiredUpgrade = std::upper_bound( UPGRADE_VERSIONS.begin(), UPGRADE_VERSIONS.end(), creator );

	// Convert the iterator to an index, which is our file version (starting at 0)
	return std::distance( UPGRADE_VERSIONS.begin(), firstRequiredUpgrade );
}


} // namespace lmms
