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

namespace lmms
{


static void findIds(const QDomElement& elem, QList<jo_id_t>& idList);


// QMap with the DOM elements that access file resources
const DataFile::ResourcesMap DataFile::ELEMENTS_WITH_RESOURCES = {
{ "sampleclip", {"src"} },
{ "audiofileprocessor", {"src"} },
};

// Vector with all the upgrade methods
const std::vector<DataFile::UpgradeMethod> DataFile::UPGRADE_METHODS = {
	&DataFile::upgrade_0_2_1_20070501   ,   &DataFile::upgrade_0_2_1_20070508,
	&DataFile::upgrade_0_3_0_rc2        ,   &DataFile::upgrade_0_3_0,
	&DataFile::upgrade_0_4_0_20080104   ,   &DataFile::upgrade_0_4_0_20080118,
	&DataFile::upgrade_0_4_0_20080129   ,   &DataFile::upgrade_0_4_0_20080409,
	&DataFile::upgrade_0_4_0_20080607   ,   &DataFile::upgrade_0_4_0_20080622,
	&DataFile::upgrade_0_4_0_beta1      ,   &DataFile::upgrade_0_4_0_rc2,
	&DataFile::upgrade_1_0_99           ,   &DataFile::upgrade_1_1_0,
	&DataFile::upgrade_1_1_91           ,   &DataFile::upgrade_1_2_0_rc3,
	&DataFile::upgrade_1_3_0            ,   &DataFile::upgrade_noHiddenClipNames,
	&DataFile::upgrade_automationNodes  ,   &DataFile::upgrade_extendedNoteRange,
	&DataFile::upgrade_defaultTripleOscillatorHQ,
	&DataFile::upgrade_mixerRename      ,   &DataFile::upgrade_bbTcoRename,
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

	const auto s_types = std::array<TypeDescStruct, DataFile::TypeCount>
	{
		TypeDescStruct{ DataFile::UnknownType, "unknown" },
		TypeDescStruct{ DataFile::SongProject, "song" },
		TypeDescStruct{ DataFile::SongProjectTemplate, "songtemplate" },
		TypeDescStruct{ DataFile::InstrumentTrackSettings, "instrumenttracksettings" },
		TypeDescStruct{ DataFile::DragNDropData, "dnddata" },
		TypeDescStruct{ DataFile::ClipboardData, "clipboard-data" },
		TypeDescStruct{ DataFile::JournalData, "journaldata" },
		TypeDescStruct{ DataFile::EffectSettings, "effectsettings" },
		TypeDescStruct{ DataFile::MidiClip, "midiclip" }
	};
}




DataFile::DataFile( Type type ) :
	QDomDocument( "lmms-project" ),
	m_fileName(""),
	m_content(),
	m_head(),
	m_type( type ),
	m_fileVersion( UPGRADE_METHODS.size() )
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
	case Type::UnknownType:
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
		case SongProject:
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
		case SongProjectTemplate:
			if( extension != "mpt" )
			{
				return _fn + ".mpt";
			}
			break;
		case InstrumentTrackSettings:
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
	if( type() == SongProject || type() == SongProjectTemplate
					|| type() == InstrumentTrackSettings )
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
	for( int i = 0; i < TypeCount; ++i )
	{
		if( s_types[i].m_name == typeName )
		{
			return static_cast<DataFile::Type>( i );
		}
	}

	// compat code
	if( typeName == "channelsettings" )
	{
		return DataFile::InstrumentTrackSettings;
	}

	return UnknownType;
}




QString DataFile::typeName( Type type )
{
	if( type >= UnknownType && type < TypeCount )
	{
		return s_types[type].m_name;
	}

	return s_types[UnknownType].m_name;
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


void DataFile::upgrade_0_2_1_20070501()
{
	// Upgrade to version 0.2.1-20070501
	QDomNodeList list = elementsByTagName( "arpandchords" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.hasAttribute( "arpdir" ) )
		{
			int arpdir = el.attribute( "arpdir" ).toInt();
			if( arpdir > 0 )
			{
				el.setAttribute( "arpdir", arpdir - 1 );
			}
			else
			{
				el.setAttribute( "arpdisabled", "1" );
			}
		}
	}

	list = elementsByTagName( "sampletrack" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.attribute( "vol" ) != "" )
		{
			el.setAttribute( "vol", LocaleHelper::toFloat(
					el.attribute( "vol" ) ) * 100.0f );
		}
		else
		{
			QDomNode node = el.namedItem(
						"automation-pattern" );
			if( !node.isElement() ||
				!node.namedItem( "vol" ).isElement() )
			{
				el.setAttribute( "vol", 100.0f );
			}
		}
	}

	list = elementsByTagName( "ladspacontrols" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QDomNode anode = el.namedItem( "automation-pattern" );
		QDomNode node = anode.firstChild();
		while( !node.isNull() )
		{
			if( node.isElement() )
			{
				QString name = node.nodeName();
				if( name.endsWith( "link" ) )
				{
					el.setAttribute( name,
						node.namedItem( "time" )
						.toElement()
						.attribute( "value" ) );
					QDomNode oldNode = node;
					node = node.nextSibling();
					anode.removeChild( oldNode );
					continue;
				}
			}
			node = node.nextSibling();
		}
	}

	QDomNode node = m_head.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.nodeName() == "bpm" )
			{
				int value = node.toElement().attribute(
						"value" ).toInt();
				if( value > 0 )
				{
					m_head.setAttribute( "bpm",
								value );
					QDomNode oldNode = node;
					node = node.nextSibling();
					m_head.removeChild( oldNode );
					continue;
				}
			}
			else if( node.nodeName() == "mastervol" )
			{
				int value = node.toElement().attribute(
						"value" ).toInt();
				if( value > 0 )
				{
					m_head.setAttribute(
						"mastervol", value );
					QDomNode oldNode = node;
					node = node.nextSibling();
					m_head.removeChild( oldNode );
					continue;
				}
			}
			else if( node.nodeName() == "masterpitch" )
			{
				m_head.setAttribute( "masterpitch",
					-node.toElement().attribute(
						"value" ).toInt() );
				QDomNode oldNode = node;
				node = node.nextSibling();
				m_head.removeChild( oldNode );
				continue;
			}
		}
		node = node.nextSibling();
	}
}


void DataFile::upgrade_0_2_1_20070508()
{
	// Upgrade to version 0.2.1-20070508 from some version greater than or equal to 0.2.1-20070501
	QDomNodeList list = elementsByTagName( "arpandchords" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.hasAttribute( "chorddisabled" ) )
		{
			el.setAttribute( "chord-enabled",
				!el.attribute( "chorddisabled" )
							.toInt() );
			el.setAttribute( "arp-enabled",
				!el.attribute( "arpdisabled" )
							.toInt() );
		}
		else if( !el.hasAttribute( "chord-enabled" ) )
		{
			el.setAttribute( "chord-enabled", true );
			el.setAttribute( "arp-enabled",
				el.attribute( "arpdir" ).toInt() != 0 );
		}
	}

	while( !( list = elementsByTagName( "channeltrack" ) ).isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "instrumenttrack" );
	}

	list = elementsByTagName( "instrumenttrack" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.hasAttribute( "vol" ) )
		{
			float value = LocaleHelper::toFloat( el.attribute( "vol" ) );
			value = roundf( value * 0.585786438f );
			el.setAttribute( "vol", value );
		}
		else
		{
			QDomNodeList vol_list = el.namedItem(
						"automation-pattern" )
					.namedItem( "vol" ).toElement()
					.elementsByTagName( "time" );
			for( int j = 0; !vol_list.item( j ).isNull();
								++j )
			{
				QDomElement timeEl = list.item( j )
							.toElement();
				int value = timeEl.attribute( "value" )
							.toInt();
				value = (int)roundf( value *
							0.585786438f );
				timeEl.setAttribute( "value", value );
			}
		}
	}
}


void DataFile::upgrade_0_3_0_rc2()
{
	// Upgrade to version 0.3.0-rc2 from some version greater than or equal to 0.2.1-20070508
	QDomNodeList list = elementsByTagName( "arpandchords" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.attribute( "arpdir" ).toInt() > 0 )
		{
			el.setAttribute( "arpdir",
				el.attribute( "arpdir" ).toInt() - 1 );
		}
	}
}


void DataFile::upgrade_0_3_0()
{
	// Upgrade to version 0.3.0 (final) from some version greater than or equal to 0.3.0-rc2
	QDomNodeList list;
	while( !( list = elementsByTagName(
				"pluckedstringsynth" ) ).isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "vibedstrings" );
		el.setAttribute( "active0", 1 );
	}

	while( !( list = elementsByTagName( "lb303" ) ).isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "lb302" );
	}

	while( !( list = elementsByTagName( "channelsettings" ) ).
							isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "instrumenttracksettings" );
	}
}


void DataFile::upgrade_0_4_0_20080104()
{
	// Upgrade to version 0.4.0-20080104 from some version greater than or equal to 0.3.0 (final)
	QDomNodeList list = elementsByTagName( "fx" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.hasAttribute( "fxdisabled" ) &&
			el.attribute( "fxdisabled" ).toInt() == 0 )
		{
			el.setAttribute( "enabled", 1 );
		}
	}
}


void DataFile::upgrade_0_4_0_20080118()
{
	// Upgrade to version 0.4.0-20080118 from some version greater than or equal to 0.4.0-20080104
	QDomNodeList list;
	while( !( list = elementsByTagName( "fx" ) ).isEmpty() )
	{
		QDomElement fxchain = list.item( 0 ).toElement();
		fxchain.setTagName( "fxchain" );
		QDomNode rack = list.item( 0 ).firstChild();
		QDomNodeList effects = rack.childNodes();
		// move items one level up
		while( effects.count() )
		{
			fxchain.appendChild( effects.at( 0 ) );
		}
		fxchain.setAttribute( "numofeffects",
			rack.toElement().attribute( "numofeffects" ) );
		fxchain.removeChild( rack );
	}
}


void DataFile::upgrade_0_4_0_20080129()
{
	// Upgrade to version 0.4.0-20080129 from some version greater than or equal to 0.4.0-20080118
	QDomNodeList list;
	while( !( list =
		elementsByTagName( "arpandchords" ) ).isEmpty() )
	{
		QDomElement aac = list.item( 0 ).toElement();
		aac.setTagName( "arpeggiator" );
		QDomNode cloned = aac.cloneNode();
		cloned.toElement().setTagName( "chordcreator" );
		aac.parentNode().appendChild( cloned );
	}
}


void DataFile::upgrade_0_4_0_20080409()
{
	// Upgrade to version 0.4.0-20080409 from some version greater than or equal to 0.4.0-20080129
	QStringList s;
	s << "note" << "pattern" << "bbtco" << "sampletco" << "time";
	for( QStringList::iterator it = s.begin(); it < s.end(); ++it )
	{
		QDomNodeList list = elementsByTagName( *it );
		for( int i = 0; !list.item( i ).isNull(); ++i )
		{
			QDomElement el = list.item( i ).toElement();
			el.setAttribute( "pos",
				el.attribute( "pos" ).toInt()*3 );
			el.setAttribute( "len",
				el.attribute( "len" ).toInt()*3 );
		}
	}
	QDomNodeList list = elementsByTagName( "timeline" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		el.setAttribute( "lp0pos",
			el.attribute( "lp0pos" ).toInt()*3 );
		el.setAttribute( "lp1pos",
			el.attribute( "lp1pos" ).toInt()*3 );
	}
}


void DataFile::upgrade_0_4_0_20080607()
{
	// Upgrade to version 0.4.0-20080607 from some version greater than or equal to 0.3.0-20080409
	QDomNodeList list;
	while( !( list = elementsByTagName( "midi" ) ).isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "midiport" );
	}
}


void DataFile::upgrade_0_4_0_20080622()
{
	// Upgrade to version 0.4.0-20080622 from some version greater than or equal to 0.3.0-20080607
	QDomNodeList list;
	while( !( list = elementsByTagName(
				"automation-pattern" ) ).isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "automationpattern" );
	}

	list = elementsByTagName( "bbtrack" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QString s = el.attribute( "name" );
		s.replace( QRegExp( "^Beat/Baseline " ),
						"Beat/Bassline " );
		el.setAttribute( "name", s );
	}
}


void DataFile::upgrade_0_4_0_beta1()
{
	// Upgrade to version 0.4.0-beta1 from some version greater than or equal to 0.4.0-20080622
	// convert binary effect-key-blobs to XML
	QDomNodeList list;
	list = elementsByTagName( "effect" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QString k = el.attribute( "key" );
		if( !k.isEmpty() )
		{
			const QList<QVariant> l =
				base64::decode( k, QVariant::List ).toList();
			if( !l.isEmpty() )
			{
				QString name = l[0].toString();
				QVariant u = l[1];
				EffectKey::AttributeMap m;
				// VST-effect?
				if( u.type() == QVariant::String )
				{
					m["file"] = u.toString();
				}
				// LADSPA-effect?
				else if( u.type() == QVariant::StringList )
				{
					const QStringList sl = u.toStringList();
					m["plugin"] = sl.value( 0 );
					m["file"] = sl.value( 1 );
				}
				EffectKey key( nullptr, name, m );
				el.appendChild( key.saveXML( *this ) );
			}
		}
	}
}


void DataFile::upgrade_0_4_0_rc2()
{
	// Upgrade to version 0.4.0-rc2 from some version greater than or equal to 0.4.0-beta1
	QDomNodeList list = elementsByTagName( "audiofileprocessor" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QString s = el.attribute( "src" );
		s.replace( "drumsynth/misc ", "drumsynth/misc_" );
		s.replace( "drumsynth/r&b", "drumsynth/r_n_b" );
		s.replace( "drumsynth/r_b", "drumsynth/r_n_b" );
		el.setAttribute( "src", s );
	}
	list = elementsByTagName( "lb302" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		int s = el.attribute( "shape" ).toInt();
		if( s >= 1 )
		{
			s--;
		}
		el.setAttribute( "shape", QString("%1").arg(s) );
	}
}


void DataFile::upgrade_1_0_99()
{
	jo_id_t last_assigned_id = 0;

	QList<jo_id_t> idList;
	findIds(documentElement(), idList);

	QDomNodeList list = elementsByTagName("ladspacontrols");
	for(int i = 0; !list.item(i).isNull(); ++i)
	{
		for(QDomNode node = list.item(i).firstChild(); !node.isNull();
			node = node.nextSibling())
		{
			QDomElement el = node.toElement();
			QDomNode data_child = el.namedItem("data");
			if(!data_child.isElement())
			{
				if (el.attribute("scale_type") == "log")
				{
					QDomElement me = createElement("data");
					me.setAttribute("value", el.attribute("data"));
					me.setAttribute("scale_type", "log");

					jo_id_t id;
					for(id = last_assigned_id + 1;
						idList.contains(id); id++)
					{
					}

					last_assigned_id = id;
					idList.append(id);
					me.setAttribute("id", id);
					el.appendChild(me);

				}
			}
		}
	}
}


void DataFile::upgrade_1_1_0()
{
	QDomNodeList list = elementsByTagName("fxchannel");
	for (int i = 1; !list.item(i).isNull(); ++i)
	{
		QDomElement el = list.item(i).toElement();
		QDomElement send = createElement("send");
		send.setAttribute("channel", "0");
		send.setAttribute("amount", "1");
		el.appendChild(send);
	}
}


void DataFile::upgrade_1_1_91()
{
	// Upgrade to version 1.1.91 from some version less than 1.1.91
	QDomNodeList list = elementsByTagName( "audiofileprocessor" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QString s = el.attribute( "src" );
		s.replace( QRegExp("/samples/bassloopes/"), "/samples/bassloops/" );
		el.setAttribute( "src", s );
	}

	list = elementsByTagName( "attribute" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.attribute( "name" ) == "plugin" && el.attribute( "value" ) == "vocoder-lmms" ) {
			el.setAttribute( "value", "vocoder" );
		}
	}

	list = elementsByTagName( "crossoevereqcontrols" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		// invert the mute LEDs
		for( int j = 1; j <= 4; ++j ){
			QString a = QString( "mute%1" ).arg( j );
			el.setAttribute( a, ( el.attribute( a ) == "0" ) ? "1" : "0" );
		}
	}

	list = elementsByTagName( "arpeggiator" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		// Swap elements ArpDirRandom and ArpDirDownAndUp
		if( el.attribute( "arpdir" ) == "3" )
		{
			el.setAttribute( "arpdir", "4" );
		}
		else if( el.attribute( "arpdir" ) == "4" )
		{
			el.setAttribute( "arpdir", "3" );
		}
	}
}


static void upgradeElement_1_2_0_rc2_42( QDomElement & el )
{
	if( el.hasAttribute( "syncmode" ) )
	{
		int syncmode = el.attribute( "syncmode" ).toInt();
		QStringList names;
		QDomNamedNodeMap atts = el.attributes();
		for( uint i = 0; i < atts.length(); i++ )
		{
			QString name = atts.item( i ).nodeName();
			if( name.endsWith( "_numerator" ) )
			{
				names << name.remove( "_numerator" )
								+ "_syncmode";
			}
		}
		for( QStringList::iterator it = names.begin(); it < names.end();
									++it )
		{
			el.setAttribute( *it, syncmode );
		}
	}

	QDomElement child = el.firstChildElement();
	while ( !child.isNull() )
	{
		upgradeElement_1_2_0_rc2_42( child );
		child = child.nextSiblingElement();
	}
}


void DataFile::upgrade_1_2_0_rc3()
{
	// Upgrade from earlier bbtrack beat note behaviour of adding
	// steps if a note is placed after the last step.
	QDomNodeList bbtracks = elementsByTagName( "bbtrack" );
	for( int i = 0; !bbtracks.item( i ).isNull(); ++i )
	{
		QDomNodeList patterns = bbtracks.item( i
				).toElement().elementsByTagName(
								"pattern" );
		for( int j = 0; !patterns.item( j ).isNull(); ++j )
		{
			int patternLength, steps;
			QDomElement el = patterns.item( j ).toElement();
			if( el.attribute( "len" ) != "" )
			{
				patternLength = el.attribute( "len" ).toInt();
				steps = patternLength / 12;
				el.setAttribute( "steps", steps );
			}
		}
	}

	// DataFile::upgrade_1_2_0_rc2_42
	QDomElement el = firstChildElement();
	while ( !el.isNull() )
	{
		upgradeElement_1_2_0_rc2_42( el );
		el = el.nextSiblingElement();
	}
}


/**
 *  Helper function to call a functor for all effect ports' DomElements,
 *  providing the functor with lists to add and remove DomElements. Helpful for
 *  patching port values from savefiles.
 */
template<class Ftor>
void iterate_ladspa_ports(QDomElement& effect, Ftor& ftor)
{
	// Head back up the DOM to upgrade ports
	QDomNodeList ladspacontrols = effect.elementsByTagName( "ladspacontrols" );
	for( int m = 0; !ladspacontrols.item( m ).isNull(); ++m )
	{
		QList<QDomElement> addList, removeList;
		QDomElement ladspacontrol = ladspacontrols.item( m ).toElement();
		for( QDomElement port = ladspacontrol.firstChild().toElement();
			!port.isNull(); port = port.nextSibling().toElement() )
		{
			QStringList parts = port.tagName().split("port");
			// Not a "port"
			if ( parts.size() < 2 )
			{
				continue;
			}
			int num = parts[1].toInt();

			// From Qt's docs of QDomNode:
			// * copying a QDomNode is OK, they still have the same
			//   pointer to the "internal" QDomNodePrivate.
			// * Also, they are using linked lists, which means
			//   deleting or appending QDomNode does not invalidate
			//   any other pointers.
			// => Inside ftor, you can (and should) push back the
			//    QDomElements by value, not references
			// => The loops below for adding and removing don't
			//    invalidate any other QDomElements
			ftor(port, num, addList, removeList);
		}

		// Add ports marked for adding
		for ( QDomElement e : addList )
		{
			ladspacontrol.appendChild( e );
		}
		// Remove ports marked for removal
		for ( QDomElement e : removeList )
		{
			ladspacontrol.removeChild( e );
		}
	}
}

// helper function if you need to print a QDomNode
QDebug operator<<(QDebug dbg, const QDomNode& node)
{
	QString s;
	QTextStream str(&s, QIODevice::WriteOnly);
	node.save(str, 2);
	dbg << qPrintable(s);
	return dbg;
}

void DataFile::upgrade_1_3_0()
{
	QDomNodeList list = elementsByTagName( "instrument" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.attribute( "name" ) == "papu" )
		{
			el.setAttribute( "name", "freeboy" );
			QDomNodeList children = el.elementsByTagName( "papu" );
			for( int j = 0; !children.item( j ).isNull(); ++j )
			{
				QDomElement child = children.item( j ).toElement();
				child.setTagName( "freeboy" );
			}
		}
		else if( el.attribute( "name" ) == "OPL2" )
		{
			el.setAttribute( "name", "opulenz" );
			QDomNodeList children = el.elementsByTagName( "OPL2" );
			for( int j = 0; !children.item( j ).isNull(); ++j )
			{
				QDomElement child = children.item( j ).toElement();
				child.setTagName( "opulenz" );
			}
		}
	}

	list = elementsByTagName( "effect" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement effect = list.item( i ).toElement();
		if( effect.attribute( "name" ) == "ladspaeffect" )
		{
			QDomNodeList keys = effect.elementsByTagName( "key" );
			for( int j = 0; !keys.item( j ).isNull(); ++j )
			{
				QDomElement key = keys.item( j ).toElement();
				QDomNodeList attributes = key.elementsByTagName( "attribute" );
				for( int k = 0; !attributes.item( k ).isNull(); ++k )
				{
					// Effect name changes

					QDomElement attribute = attributes.item( k ).toElement();
					const QString attrName = attribute.attribute("name");
					const QString attrVal = attribute.attribute("value");
					const QString plugin = attrName == "plugin" ? attrVal : "";

					static const std::map<QString, QString> pluginNames = {
						{"Sidechaincompressor", "SidechainCompressor"},
						{"Sidechaingate", "SidechainGate"},
						{"Multibandcompressor", "MultibandCompressor"},
						{"Multibandgate", "MultibandGate"},
						{"Multibandlimiter", "MultibandLimiter"},
					};

					if (attrName == "file" && (attrVal == "calf" || attrVal == "calf.so" ))
					{
						attribute.setAttribute( "value", "veal" );
					}

					const auto newName = pluginNames.find(plugin);
					if (newName != pluginNames.end())
					{
						attribute.setAttribute("value", newName->second);
					}

					// Handle port changes

					if (plugin == "MultibandLimiter" ||	plugin == "MultibandCompressor" || plugin == "MultibandGate")
					{
						auto fn = [&](QDomElement& port, int num, QList<QDomElement>&, QList<QDomElement>& removeList)
						{
							// Mark ports for removal
							if ( num >= 18 && num <= 23 )
							{
								removeList << port;
							}
							// Bump higher ports up 6 positions
							else if ( num >= 24 )
							{
								// port01...port010, etc
								QString name( "port0" );
								name.append( QString::number( num -6 ) );
								port.setTagName( name );
							}
						};
						iterate_ladspa_ports(effect, fn);
					}

					else if (plugin == "Pulsator")
					{
						auto fn = [&](QDomElement& port, int num, QList<QDomElement>& addList, QList<QDomElement>& removeList)
						{
							switch(num)
							{
								case 16:
								{
									// old freq is now at port 25
									QDomElement portCopy = createElement("port025");
									portCopy.setAttribute("data", port.attribute("data"));
									addList << portCopy;
									// remove old freq port
									removeList << port;
									// set the "timing" port to choose port23+2=port25 (timing in Hz)
									QDomElement timing = createElement("port022");
									timing.setAttribute("data", 2);
									addList << timing;
									break;
								}
								// port 18 (modulation) => 17
								case 17:
									port.setTagName("port016");
									break;
								case 18:
								{
									// leave port 18 (offsetr), but add port 17 (offsetl)
									QDomElement offsetl = createElement("port017");
									offsetl.setAttribute("data", 0.0f);
									addList << offsetl;
									// additional: bash port 21 to 1
									QDomElement pulsewidth = createElement("port021");
									pulsewidth.setAttribute("data", 1.0f);
									addList << pulsewidth;
									break;
								}
							}


						};
						iterate_ladspa_ports(effect, fn);
					}

					else if (plugin == "VintageDelay")
					{
						auto fn = [&](QDomElement& port, int num, QList<QDomElement>& addList, QList<QDomElement>& )
						{
							switch(num)
							{
								case 4:
								{
									// BPM is now port028
									port.setTagName("port028");
									// bash timing to BPM
									QDomElement timing = createElement("port027");
									timing.setAttribute("data", 0);
									addList << timing;

									// port 5 and 6 (in, out gain) need to be bashed to 1:
									QDomElement input = createElement("port05");
									input.setAttribute("data", 1.0f);
									addList << input;
									QDomElement output = createElement("port06");
									output.setAttribute("data", 1.0f);
									addList << output;

									break;
								}
								default:
									// all other ports increase by 10
									QString name( "port0" );
									name.append( QString::number( num + 10 ) );
									port.setTagName( name );
							}


						};
						iterate_ladspa_ports(effect, fn);
					}

					else if (plugin == "Equalizer5Band" || plugin == "Equalizer8Band" || plugin == "Equalizer12Band")
					{
						// NBand equalizers got 4 q nobs inserted. We need to shift everything else...
						// HOWEVER: 5 band eq has only 2 q nobs inserted (no LS/HS filters)
						bool band5 = plugin == "Equalizer5Band";
						auto fn = [&](QDomElement& port, int num, QList<QDomElement>& addList, QList<QDomElement>& )
						{
							if(num == 4)
							{
								// don't modify port 4, but some other ones:
								int zoom_port;
								if (plugin == "Equalizer5Band")
									zoom_port = 36;
								else if (plugin == "Equalizer8Band")
									zoom_port = 48;
								else // 12 band
									zoom_port = 64;
								// bash zoom to 0.25
								QString name( "port0" );
								name.append( QString::number( zoom_port ) );
								QDomElement timing = createElement(name);
								timing.setAttribute("data", 0.25f);
								addList << timing;
							}
							// the following code could be refactored, but I did careful code-reading
							// to prevent copy-paste-errors
							if(num == 18)
							{
								// 18 => 19
								port.setTagName("port019");
								// insert port 18 (q)
								QDomElement q = createElement("port018");
								q.setAttribute("data", 0.707f);
								addList << q;
							}
							else if(num >= 19 && num <= 20)
							{
								// num += 1
								QString name( "port0" );
								name.append( QString::number( num + 1 ) );
								port.setTagName( name );
							}
							else if(num == 21)
							{
								// 21 => 23
								port.setTagName("port023");
								// insert port 22 (q)
								QDomElement q = createElement("port022");
								q.setAttribute("data", 0.707f);
								addList << q;
							}
							else if(num >= 22 && (num <= 23 || band5))
							{
								// num += 2
								QString name( "port0" );
								name.append( QString::number( num + 2 ) );
								port.setTagName( name );
							}
							else if(num == 24 && !band5)
							{
								// 24 => 27
								port.setTagName("port027");
								// insert port 26 (q)
								QDomElement q = createElement("port026");
								q.setAttribute("data", 0.707f);
								addList << q;
							}
							else if(num >= 25 && num <= 26 && !band5)
							{
								// num += 3
								QString name( "port0" );
								name.append( QString::number( num + 3 ) );
								port.setTagName( name );
							}
							else if(num == 27 && !band5)
							{
								// 27 => 31
								port.setTagName("port031");
								// insert port 30 (q)
								QDomElement q = createElement("port030");
								q.setAttribute("data", 0.707f);
								addList << q;
							}
							else if(num >= 28 && !band5)
							{
								// num += 4
								QString name( "port0" );
								name.append( QString::number( num + 4 ) );
								port.setTagName( name );
							}
						};
						iterate_ladspa_ports(effect, fn);
					}

					else if (plugin == "Saturator")
					{
						auto fn = [&](QDomElement& port, int num, QList<QDomElement>&, QList<QDomElement>& )
						{
							// These ports have been shifted a bit weird...
							if( num == 7 )
							{
								port.setTagName("port015");
							}
							else if(num == 12)
							{
								port.setTagName("port016");
							}
							else if(num == 13)
							{
								port.setTagName("port017");
							}
							else if ( num >= 15 )
							{
								QString name( "port0" );
								name.append( QString::number( num + 3 ) );
								port.setTagName( name );
							}
						};
						iterate_ladspa_ports(effect, fn);
					}

					else if (plugin == "StereoTools")
					{
						auto fn = [&](QDomElement& port, int num, QList<QDomElement>&, QList<QDomElement>& )
						{
							// This effect can not be back-ported due to bugs in the old version,
							// or due to different behaviour. We thus port all parameters we can,
							// and bash all new parameters (in this case, s.level and m.level) to
							// their new defaults (both 1.0f in this case)

							if( num == 23 || num == 25 )
							{
								port.setAttribute("data", 1.0f);
							}
						};
						iterate_ladspa_ports(effect, fn);
					}

					else if (plugin == "amPitchshift")
					{
						auto fn = [&](QDomElement& port, int num, QList<QDomElement>&, QList<QDomElement>& removeList)
						{
							switch (num)
							{
							case 0:
								port.setTagName("port01");
								break;
							case 1:
								port.setTagName("port03");
								break;
							case 10:
								port.setTagName("port11");
								break;
							case 11:
								port.setTagName("port13");
								break;
							}
						};
						iterate_ladspa_ports(effect, fn);
					}
				}
			}
		}
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
	auto affected = [](const QDomElement& instrument)
	{
		return instrument.attribute("name") == "zynaddsubfx" ||
			instrument.attribute("name") == "vestige" ||
			instrument.attribute("name") == "lv2instrument" ||
			instrument.attribute("name") == "carlapatchbay" ||
			instrument.attribute("name") == "carlarack";
	};

	if (!elementsByTagName("song").item(0).isNull())
	{
		// Dealing with a project file, go through all the tracks
		QDomNodeList tracks = elementsByTagName("track");
		for (int i = 0; !tracks.item(i).isNull(); i++)
		{
			// Ignore BB container tracks
			if (tracks.item(i).toElement().attribute("type").toInt() == 1) { continue; }

			QDomNodeList instruments = tracks.item(i).toElement().elementsByTagName("instrument");
			if (instruments.isEmpty()) { continue; }
			QDomElement instrument = instruments.item(0).toElement();
			// Raise the base note of every instrument by 12 to compensate for the change
			// of A4 key code from 57 to 69. This ensures that notes are labeled correctly.
			instrument.parentNode().toElement().setAttribute(
				"basenote",
				instrument.parentNode().toElement().attribute("basenote").toInt() + 12);
			// Raise the pitch of all notes in patterns assigned to instruments not affected
			// by #1857 by an octave. This negates the base note change for normal instruments,
			// but leaves the MIDI-based instruments sounding an octave lower, preserving their
			// pitch in existing projects.
			if (!affected(instrument))
			{
				QDomNodeList patterns = tracks.item(i).toElement().elementsByTagName("pattern");
				for (int i = 0; !patterns.item(i).isNull(); i++)
				{
					QDomNodeList notes = patterns.item(i).toElement().elementsByTagName("note");
					for (int i = 0; !notes.item(i).isNull(); i++)
					{
						notes.item(i).toElement().setAttribute(
							"key",
							notes.item(i).toElement().attribute("key").toInt() + 12
						);
					}
				}
			}
		}
	}
	else
	{
		// Dealing with a preset, not a song
		QDomNodeList presets = elementsByTagName("instrumenttrack");
		if (presets.item(0).isNull()) { return; }
		QDomElement preset = presets.item(0).toElement();
		// Common correction for all instrument presets (make base notes match the new MIDI range).
		// NOTE: Many older presets do not have any basenote defined, assume they were "made for 57".
		// (Specifying a default like this also happens to fix a FileBrowser bug where previews of presets
		// with undefined basenote always play with the basenote inherited from previously played preview.)
		int oldBase = preset.attribute("basenote", "57").toInt();
		preset.setAttribute("basenote", oldBase + 12);
		// Extra correction for Zyn, VeSTige, LV2 and Carla (to preserve the original buggy behavior).
		QDomNodeList instruments = presets.item(0).toElement().elementsByTagName("instrument");
		if (instruments.isEmpty()) { return; }
		QDomElement instrument = instruments.item(0).toElement();
		if (affected(instrument))
		{
			preset.setAttribute("basenote", preset.attribute("basenote").toInt() + 12);
		}
	}
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
void DataFile::upgrade_mixerRename()
{
	// Change nodename <fxmixer> to <mixer>
	QDomNodeList fxmixer = elementsByTagName("fxmixer");
	for (int i = 0; !fxmixer.item(i).isNull(); ++i)
	{
		fxmixer.item(i).toElement().setTagName("mixer");
	}

	// Change nodename <fxchannel> to <mixerchannel>
	QDomNodeList fxchannel = elementsByTagName("fxchannel");
	for (int i = 0; !fxchannel.item(i).isNull(); ++i)
	{
		fxchannel.item(i).toElement().setTagName("mixerchannel");
	}

	// Change the attribute fxch of element <instrumenttrack> to mixch
	QDomNodeList fxch = elementsByTagName("instrumenttrack");
	for(int i = 0; !fxch.item(i).isNull(); ++i)
	{
		if(fxch.item(i).toElement().hasAttribute("fxch"))
		{
			fxch.item(i).toElement().setAttribute("mixch", fxch.item(i).toElement().attribute("fxch"));
			fxch.item(i).toElement().removeAttribute("fxch");
		}
	}
}


// Rename BB to pattern and TCO to clip
void DataFile::upgrade_bbTcoRename()
{
	std::vector<std::pair<const char *, const char *>> names {
		{"automationpattern", "automationclip"},
		{"bbtco", "patternclip"},
		{"pattern", "midiclip"},
		{"sampletco", "sampleclip"},
		{"bbtrack", "patterntrack"},
		{"bbtrackcontainer", "patternstore"},
	};
	// Replace names of XML tags
	for (auto name : names)
	{
		QDomNodeList elements = elementsByTagName(name.first);
		for (int i = 0; !elements.item(i).isNull(); ++i)
		{
			elements.item(i).toElement().setTagName(name.second);
		}
	}
	// Replace "Beat/Bassline" with "Pattern" in track names
	QDomNodeList elements = elementsByTagName("track");
	for (int i = 0; !elements.item(i).isNull(); ++i)
	{
		auto e = elements.item(i).toElement();
		static_assert(Track::PatternTrack == 1, "Must be type=1 for backwards compatibility");
		if (e.attribute("type").toInt() == Track::PatternTrack)
		{
			e.setAttribute("name", e.attribute("name").replace("Beat/Bassline", "Pattern"));
		}
	}
}


void DataFile::upgrade()
{
	// Runs all necessary upgrade methods
	std::size_t max = std::min(static_cast<std::size_t>(m_fileVersion), UPGRADE_METHODS.size());
	std::for_each( UPGRADE_METHODS.begin() + max, UPGRADE_METHODS.end(),
		[this](UpgradeMethod um)
		{
			(this->*um)();
		}
	);

	// Bump the file version (which should be the size of the upgrade methods vector)
	m_fileVersion = UPGRADE_METHODS.size();

	// update document meta data
	documentElement().setAttribute( "version", m_fileVersion );
	documentElement().setAttribute( "type", typeName( type() ) );
	documentElement().setAttribute( "creator", "LMMS" );
	documentElement().setAttribute( "creatorversion", LMMS_VERSION );

	if( type() == SongProject || type() == SongProjectTemplate )
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

		if (createdWith.setCompareType(ProjectVersion::Minor)
		 !=  openedWith.setCompareType(ProjectVersion::Minor)
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


void findIds(const QDomElement& elem, QList<jo_id_t>& idList)
{
	if(elem.hasAttribute("id"))
	{
		idList.append(elem.attribute("id").toInt());
	}
	QDomElement child = elem.firstChildElement();
	while(!child.isNull())
	{
		findIds(child, idList);
		child = child.nextSiblingElement();
	}
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
