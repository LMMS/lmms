/*
 * ConfigManager.cpp - implementation of class ConfigManager
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QDomElement>
#include <QDir>
#include <QMessageBox>
#include <QApplication>
#include <QStandardPaths>
#include <QTextStream>

#include "ConfigManager.h"
#include "MainWindow.h"
#include "ProjectVersion.h"
#include "GuiApplication.h"

#include "lmmsversion.h"

namespace lmms
{


// Vector with all the upgrade methods
const std::vector<ConfigManager::UpgradeMethod> ConfigManager::UPGRADE_METHODS = {
	&ConfigManager::upgrade_1_1_90    ,    &ConfigManager::upgrade_1_1_91,
	&ConfigManager::upgrade_1_2_2
};

static inline QString ensureTrailingSlash(const QString & s )
{
	if(! s.isEmpty() && !s.endsWith('/') && !s.endsWith('\\'))
	{
		return s + '/';
	}
	return s;
}


ConfigManager * ConfigManager::s_instanceOfMe = nullptr;


ConfigManager::ConfigManager() :
	m_version(defaultVersion()),
	m_configVersion( UPGRADE_METHODS.size() )
{
	if (QFileInfo::exists(qApp->applicationDirPath() + PORTABLE_MODE_FILE))
	{
		initPortableWorkingDir();
	}
	else
	{
		initInstalledWorkingDir();
	}
	m_dataDir = "data:/";
	m_vstDir = m_workingDir + "vst/";
	m_sf2Dir = m_workingDir + SF2_PATH;
	m_gigDir = m_workingDir + GIG_PATH;
	m_themeDir = defaultThemeDir();
	if (std::getenv("LMMS_DATA_DIR"))
	{
		QDir::addSearchPath("data", QString::fromLocal8Bit(std::getenv("LMMS_DATA_DIR")));
	}
	initDevelopmentWorkingDir();

#ifdef LMMS_BUILD_WIN32
	QDir::addSearchPath("data", qApp->applicationDirPath() + "/data/");
#else
	QDir::addSearchPath("data", qApp->applicationDirPath().section('/', 0, -2) + "/share/lmms/");
#endif

}




ConfigManager::~ConfigManager()
{
	saveConfigFile();
}


void ConfigManager::upgrade_1_1_90()
{
	// Remove trailing " (bad latency!)" string which was once saved with PulseAudio
	if(value("mixer", "audiodev").startsWith("PulseAudio ("))
	{
		setValue("mixer", "audiodev", "PulseAudio");
	}

	// MidiAlsaRaw used to store the device info as "Device" instead of "device"
	if (value("MidiAlsaRaw", "device").isNull())
	{
		// copy "device" = "Device" and then delete the old "Device" (further down)
		QString oldDevice = value("MidiAlsaRaw", "Device");
		setValue("MidiAlsaRaw", "device", oldDevice);
	}
	if (!value("MidiAlsaRaw", "device").isNull())
	{
		// delete the old "Device" in the case that we just copied it to "device"
		//   or if the user somehow set both the "Device" and "device" fields
		deleteValue("MidiAlsaRaw", "Device");
	}
}

void ConfigManager::upgrade_1_1_91()
{
	// rename displaydbv to displaydbfs
	if (!value("app", "displaydbv").isNull())
	{
		setValue("app", "displaydbfs", value("app", "displaydbv"));
		deleteValue("app", "displaydbv");
	}
}

void ConfigManager::upgrade_1_2_2()
{
	// Since mixer has been renamed to audioengine, we need to transfer the
	// attributes from the old element to the new one
	std::vector<QString> attrs = {
		"audiodev", "mididev", "framesperaudiobuffer", "hqaudio", "samplerate"
	};

	for (auto attr : attrs)
	{
		if (!value("mixer", attr).isNull())
		{
			setValue("audioengine", attr, value("mixer", attr));
			deleteValue("mixer", attr);
		}
	}

	m_settings.remove("mixer");
}

void ConfigManager::upgrade()
{
	// Skip the upgrade if versions match
	if (m_version == LMMS_VERSION)
	{
		return;
	}

	// Runs all necessary upgrade methods
	std::size_t max = std::min(static_cast<std::size_t>(m_configVersion), UPGRADE_METHODS.size());
	std::for_each( UPGRADE_METHODS.begin() + max, UPGRADE_METHODS.end(),
		[this](UpgradeMethod um)
		{
			(this->*um)();
		}
	);
	
	ProjectVersion createdWith = m_version;
	
	// Don't use old themes as they break the UI (i.e. 0.4 != 1.0, etc)
	if (createdWith.setCompareType(ProjectVersion::CompareType::Minor) != LMMS_VERSION)
	{
		m_themeDir = defaultThemeDir();
	}

	// Bump the version, now that we are upgraded
	m_version = LMMS_VERSION;
	m_configVersion = UPGRADE_METHODS.size();
}

QString ConfigManager::defaultVersion() const
{
	return LMMS_VERSION;
}

WindowEmbed::Method ConfigManager::vstEmbedMethod() const
{
	const auto methods = WindowEmbed::availableMethods();
	const auto defaultMethod = methods.empty() ? WindowEmbed::Method::None : methods.back();
	const auto defaultMethodStr = QString{WindowEmbed::toString(defaultMethod).data()};
	const auto currentMethodStr = value("ui", "vstembedmethod", defaultMethodStr);
	const auto currentMethod = WindowEmbed::toEnum(currentMethodStr.toStdString());
	return std::find(methods.begin(), methods.end(), currentMethod) != methods.end()
		? currentMethod
		: defaultMethod;
}

bool ConfigManager::hasWorkingDir() const
{
	return QDir(m_workingDir).exists();
}


void ConfigManager::setWorkingDir(const QString & workingDir)
{
	m_workingDir = ensureTrailingSlash(QDir::cleanPath(workingDir));
}




void ConfigManager::setVSTDir(const QString & vstDir)
{
	m_vstDir = ensureTrailingSlash(vstDir);
}




void ConfigManager::setLADSPADir(const QString & ladspaDir)
{
	m_ladspaDir = ladspaDir;
}




void ConfigManager::setSTKDir(const QString & stkDir)
{
#ifdef LMMS_HAVE_STK
	m_stkDir = ensureTrailingSlash(stkDir);
#endif
}




void ConfigManager::setSF2Dir(const QString & sf2Dir)
{
	m_sf2Dir = sf2Dir;
}




void ConfigManager::setSF2File(const QString & sf2File)
{
#ifdef LMMS_HAVE_FLUIDSYNTH
	m_sf2File = sf2File;
#endif
}




void ConfigManager::setGIGDir(const QString & gigDir)
{
	m_gigDir = gigDir;
}




void ConfigManager::setThemeDir(const QString & themeDir)
{
	m_themeDir = ensureTrailingSlash(themeDir);
}




void ConfigManager::setBackgroundPicFile(const QString & backgroundPicFile)
{
	m_backgroundPicFile = backgroundPicFile;
}




void ConfigManager::createWorkingDir()
{
	QDir().mkpath(m_workingDir);

	QDir().mkpath(userProjectsDir());
	QDir().mkpath(userTemplateDir());
	QDir().mkpath(userSamplesDir());
	QDir().mkpath(userPresetsDir());
	QDir().mkpath(userGigDir());
	QDir().mkpath(userSf2Dir());
	QDir().mkpath(userVstDir());
	QDir().mkpath(userLadspaDir());
}



void ConfigManager::addRecentlyOpenedProject(const QString & file)
{
	QFileInfo recentFile(file);
	if(recentFile.suffix().toLower() == "mmp" ||
		recentFile.suffix().toLower() == "mmpz" ||
		recentFile.suffix().toLower() == "mpt")
	{
		m_recentlyOpenedProjects.removeAll(file);
		if(m_recentlyOpenedProjects.size() > 50)
		{
			m_recentlyOpenedProjects.removeLast();
		}
		m_recentlyOpenedProjects.push_front(file);
		ConfigManager::inst()->saveConfigFile();
	}
}




QString ConfigManager::value(const QString& cls, const QString& attribute, const QString& defaultVal) const
{
	if (m_settings.find(cls) != m_settings.end())
	{
		for (const auto& setting : m_settings[cls])
		{
			if (setting.first == attribute)
			{
				return setting.second;
			}
		}
	}
	return defaultVal;
}




void ConfigManager::setValue(const QString & cls,
				const QString & attribute,
				const QString & value)
{
	if(m_settings.contains(cls))
	{
		for(QPair<QString, QString>& pair : m_settings[cls])
		{
			if(pair.first == attribute)
			{
				if (pair.second != value)
				{
					pair.second = value;
					emit valueChanged(cls, attribute, value);
				}
				return;
			}
		}
	}
	// not in map yet, so we have to add it...
	m_settings[cls].push_back(qMakePair(attribute, value));
}


void ConfigManager::deleteValue(const QString & cls, const QString & attribute)
{
	if(m_settings.contains(cls))
	{
		for(stringPairVector::iterator it = m_settings[cls].begin();
					it != m_settings[cls].end(); ++it)
		{
			if((*it).first == attribute)
			{
				m_settings[cls].erase(it);
				return;
			}
		}
	}
}


void ConfigManager::loadConfigFile(const QString & configFile)
{
	// read the XML file and create DOM tree
	// Allow configuration file override through --config commandline option
	if (!configFile.isEmpty())
	{
		m_lmmsRcFile = configFile;
	}

	QFile cfg_file(m_lmmsRcFile);
	QDomDocument dom_tree;

	if(cfg_file.open(QIODevice::ReadOnly))
	{
		QString errorString;
		int errorLine, errorCol;
		if(dom_tree.setContent(&cfg_file, false, &errorString, &errorLine, &errorCol))
		{
			// get the head information from the DOM
			QDomElement root = dom_tree.documentElement();

			QDomNode node = root.firstChild();

			// Cache LMMS version
			if (!root.attribute("version").isNull()) {
				m_version = root.attribute("version");
			}

			// Get the version of the configuration file (for upgrade purposes)
			if( root.attribute("configversion").isNull() )
			{
				m_configVersion = legacyConfigVersion(); // No configversion attribute found
			}
			else
			{
				bool success;
				m_configVersion = root.attribute("configversion").toUInt(&success);
				if( !success ) qWarning("Config Version conversion failure.");
			}

			// create the settings-map out of the DOM
			while(!node.isNull())
			{
				if(node.isElement() &&
					node.toElement().hasAttributes ())
				{
					stringPairVector attr;
					QDomNamedNodeMap node_attr =
						node.toElement().attributes();
					for(int i = 0; i < node_attr.count();
									++i)
					{
						QDomNode n = node_attr.item(i);
						if(n.isAttr())
						{
							attr.push_back(qMakePair(n.toAttr().name(),
											n.toAttr().value()));
						}
					}
					m_settings[node.nodeName()] = attr;
				}
				else if(node.nodeName() == "recentfiles")
				{
					m_recentlyOpenedProjects.clear();
					QDomNode n = node.firstChild();
					while(!n.isNull())
					{
						if(n.isElement() && n.toElement().hasAttributes())
						{
							m_recentlyOpenedProjects <<
									n.toElement().attribute("path");
						}
						n = n.nextSibling();
					}
				}
				node = node.nextSibling();
			}

			if(value("paths", "theme") != "")
			{
				m_themeDir = value("paths", "theme");
#ifdef LMMS_BUILD_WIN32
				// Detect a QDir/QFile hang on Windows
				// see issue #3417 on github
				bool badPath = (m_themeDir == "/" || m_themeDir == "\\");
#else
				bool badPath = false;
#endif

				if(badPath || !QDir(m_themeDir).exists() ||
						!QFile(m_themeDir + "/style.css").exists())
				{
					m_themeDir = defaultThemeDir();
				}
				m_themeDir = ensureTrailingSlash(m_themeDir);
			}
			setWorkingDir(value("paths", "workingdir"));

			setGIGDir(value("paths", "gigdir") == "" ? gigDir() : value("paths", "gigdir"));
			setSF2Dir(value("paths", "sf2dir") == "" ? sf2Dir() : value("paths", "sf2dir"));
			setVSTDir(value("paths", "vstdir"));
			setLADSPADir(value("paths", "ladspadir"));
		#ifdef LMMS_HAVE_STK
			setSTKDir(value("paths", "stkdir"));
		#endif
		#ifdef LMMS_HAVE_FLUIDSYNTH
			setSF2File(value("paths", "defaultsf2"));
		#endif
			setBackgroundPicFile(value("paths", "backgroundtheme"));
		}
		else if (gui::getGUI() != nullptr)
		{
			QMessageBox::warning(nullptr, gui::MainWindow::tr("Configuration file"),
									gui::MainWindow::tr("Error while parsing configuration file at line %1:%2: %3").
													arg(errorLine).
													arg(errorCol).
													arg(errorString));
		}
		cfg_file.close();
	}

	// Plugins are searched recursively, blacklist problematic locations
	if( m_vstDir.isEmpty() || m_vstDir == QDir::separator() || m_vstDir == "/" ||
			m_vstDir == ensureTrailingSlash( QDir::homePath() ) ||
			!QDir( m_vstDir ).exists() )
	{
#ifdef LMMS_BUILD_WIN32
		QString programFiles = QString::fromLocal8Bit(getenv("ProgramFiles"));
		m_vstDir =  programFiles + "/VstPlugins/";
#else
		m_vstDir =  m_workingDir + "plugins/vst/";
#endif
	}

	if(m_ladspaDir.isEmpty() )
	{
		m_ladspaDir = userLadspaDir();
	}

#ifdef LMMS_HAVE_STK
	if(m_stkDir.isEmpty() || m_stkDir == QDir::separator() || m_stkDir == "/" ||
			!QDir(m_stkDir).exists())
	{
#if defined(LMMS_BUILD_WIN32)
		m_stkDir = m_dataDir + "stk/rawwaves/";
#else
		// Look for bundled raw waves first
		m_stkDir = qApp->applicationDirPath() + "/../share/stk/rawwaves/";
		// Try system installations if not exists
		if (!QDir(m_stkDir).exists())
		{
			m_stkDir = "/usr/local/share/stk/rawwaves/";
		}
		if (!QDir(m_stkDir).exists())
		{
			m_stkDir = "/usr/share/stk/rawwaves/";
		}
#endif
	}
#endif // LMMS_HAVE_STK

	upgrade();

	QStringList searchPaths;
	if (std::getenv("LMMS_THEME_PATH"))
		searchPaths << std::getenv("LMMS_THEME_PATH");
	searchPaths << themeDir() << defaultThemeDir();
	QDir::setSearchPaths("resources", searchPaths);

	// Create any missing subdirectories in the working dir, but only if the working dir exists
	if(hasWorkingDir())
	{
		createWorkingDir();
	}
}




void ConfigManager::saveConfigFile()
{
	setValue("paths", "theme", m_themeDir);
	setValue("paths", "workingdir", m_workingDir);
	setValue("paths", "vstdir", m_vstDir);
	setValue("paths", "gigdir", m_gigDir);
	setValue("paths", "sf2dir", m_sf2Dir);
	setValue("paths", "ladspadir", m_ladspaDir);
#ifdef LMMS_HAVE_STK
	setValue("paths", "stkdir", m_stkDir);
#endif
#ifdef LMMS_HAVE_FLUIDSYNTH
	setValue("paths", "defaultsf2", m_sf2File);
#endif
	setValue("paths", "backgroundtheme", m_backgroundPicFile);

	QDomDocument doc("lmms-config-file");

	QDomElement lmms_config = doc.createElement("lmms");
	lmms_config.setAttribute("version", m_version);
	lmms_config.setAttribute("configversion", m_configVersion);
	doc.appendChild(lmms_config);

	for (auto it = m_settings.begin(); it != m_settings.end(); ++it)
	{
		QDomElement n = doc.createElement(it.key());
		for (const auto& [first, second] : *it)
		{
			n.setAttribute(first, second);
		}
		lmms_config.appendChild(n);
	}

	QDomElement recent_files = doc.createElement("recentfiles");

	for (const auto& recentlyOpenedProject : m_recentlyOpenedProjects)
	{
		QDomElement n = doc.createElement("file");
		n.setAttribute("path", recentlyOpenedProject);
		recent_files.appendChild(n);
	}
	lmms_config.appendChild(recent_files);

	QString xml = "<?xml version=\"1.0\"?>\n" + doc.toString(2);

	QFile outfile(m_lmmsRcFile);
	if(!outfile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		using gui::MainWindow;

		QString title, message;
		title = MainWindow::tr("Could not open file");
		message = MainWindow::tr("Could not open file %1 "
					"for writing.\nPlease make "
					"sure you have write "
					"permission to the file and "
					"the directory containing the "
					"file and try again!"
						).arg(m_lmmsRcFile);
		if (gui::getGUI() != nullptr)
		{
			QMessageBox::critical(nullptr, title, message,
						QMessageBox::Ok,
						QMessageBox::NoButton);
		}
		return;
	}

	outfile.write(xml.toUtf8());
	outfile.close();
}

void ConfigManager::initPortableWorkingDir()
{
	QString applicationPath = qApp->applicationDirPath();
	m_workingDir = applicationPath + "/lmms-workspace/";
	m_lmmsRcFile = applicationPath + "/.lmmsrc.xml";
}

void ConfigManager::initInstalledWorkingDir()
{
	m_workingDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/lmms/";
	m_lmmsRcFile = QDir::home().absolutePath() +"/.lmmsrc.xml";
	// Detect < 1.2.0 working directory as a courtesy
	if ( QFileInfo( QDir::home().absolutePath() + "/lmms/projects/" ).exists() )
		m_workingDir = QDir::home().absolutePath() + "/lmms/";
}

void ConfigManager::initDevelopmentWorkingDir()
{
	// If we're in development (lmms is not installed) let's get the source and
	// binary directories by reading the CMake Cache
	QDir appPath = qApp->applicationDirPath();
	// If in tests, get parent directory
	if (appPath.dirName() == "tests") {
		appPath.cdUp();
	}
	QFile cmakeCache(appPath.absoluteFilePath("CMakeCache.txt"));
	if (cmakeCache.exists()) {
		cmakeCache.open(QFile::ReadOnly);
		QTextStream stream(&cmakeCache);

		// Find the lines containing something like lmms_SOURCE_DIR:static=<dir>
		// and lmms_BINARY_DIR:static=<dir>
		int done = 0;
		while(! stream.atEnd())
		{
			QString line = stream.readLine();

			if (line.startsWith("lmms_SOURCE_DIR:")) {
				QString srcDir = line.section('=', -1).trimmed();
				QDir::addSearchPath("data", srcDir + "/data/");
				done++;
			}
			if (line.startsWith("lmms_BINARY_DIR:")) {
				m_lmmsRcFile = line.section('=', -1).trimmed() +  QDir::separator() +
							   ".lmmsrc.xml";
				done++;
			}
			if (done == 2)
			{
				break;
			}
		}

		cmakeCache.close();
	}
}

// If configversion is not present, we will convert the LMMS version to the appropriate
// configuration file version for backwards compatibility.
unsigned int ConfigManager::legacyConfigVersion()
{
	ProjectVersion createdWith = m_version;

	createdWith.setCompareType(ProjectVersion::CompareType::Build);

	if( createdWith < "1.1.90" )
	{
		return 0;
	}
	else if( createdWith < "1.1.91" )
	{
		return 1;
	}
	else
	{
		return 2;
	}
}


} // namespace lmms
