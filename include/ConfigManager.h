/*
 * ConfigManager.h - class ConfigManager, a class for managing LMMS-configuration
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_CONFIG_MANAGER_H
#define LMMS_CONFIG_MANAGER_H

#include "lmmsconfig.h"

#include <QMap>
#include <QPair>
#include <QStringList>
#include <QObject>

#include <vector>
#include "lmms_export.h"


namespace lmms
{


const QString PROJECTS_PATH = "projects/";
const QString TEMPLATE_PATH = "templates/";
const QString PRESETS_PATH = "presets/";
const QString SAMPLES_PATH = "samples/";
const QString GIG_PATH = "samples/gig/";
const QString SF2_PATH = "samples/soundfonts/";
const QString LADSPA_PATH ="plugins/ladspa/";
const QString DEFAULT_THEME_PATH = "themes/default/";
const QString TRACK_ICON_PATH = "track_icons/";
const QString LOCALE_PATH = "locale/";
const QString PORTABLE_MODE_FILE = "/portable_mode.txt";

class LMMS_EXPORT ConfigManager : public QObject
{
	Q_OBJECT

	using UpgradeMethod = void(ConfigManager::*)();

public:
	static inline ConfigManager * inst()
	{
		if(s_instanceOfMe == nullptr )
		{
			s_instanceOfMe = new ConfigManager();
		}
		return s_instanceOfMe;
	}


	const QString & workingDir() const
	{
		return m_workingDir;
	}

	void initPortableWorkingDir();

	void initInstalledWorkingDir();

	void initDevelopmentWorkingDir();

	const QString & dataDir() const
	{
		return m_dataDir;
	}

	QString factoryProjectsDir() const
	{
		return dataDir() + PROJECTS_PATH;
	}

	QString factoryTemplatesDir() const
	{
		return factoryProjectsDir() + TEMPLATE_PATH;
	}

	QString factoryPresetsDir() const
	{
		return dataDir() + PRESETS_PATH;
	}

	QString factorySamplesDir() const
	{
		return dataDir() + SAMPLES_PATH;
	}


	QString userProjectsDir() const
	{
		return workingDir() + PROJECTS_PATH;
	}

	QString userTemplateDir() const
	{
		return workingDir() + TEMPLATE_PATH;
	}

	QString userPresetsDir() const
	{
		return workingDir() + PRESETS_PATH;
	}

	QString userSamplesDir() const
	{
		return workingDir() + SAMPLES_PATH;
	}


	const QString & vstDir() const
	{
		return m_vstDir;
	}

	const QString & ladspaDir() const
	{
		return m_ladspaDir;
	}

	const QString & sf2Dir() const
	{
		return m_sf2Dir;
	}

#ifdef LMMS_HAVE_FLUIDSYNTH
	const QString & sf2File() const
	{
		return m_sf2File;
	}
#endif

#ifdef LMMS_HAVE_STK
	const QString & stkDir() const
	{
		return m_stkDir;
	}
#endif

	const QString & gigDir() const
	{
		return m_gigDir;
	}


	QString userVstDir() const
	{
		return m_vstDir;
	}

	QString userLadspaDir() const
	{
		return workingDir() + LADSPA_PATH;
	}

	QString userSf2Dir() const
	{
		return workingDir() + SF2_PATH;
	}

	QString userGigDir() const
	{
		return workingDir() + GIG_PATH;
	}

	QString defaultThemeDir() const
	{
		return m_dataDir + DEFAULT_THEME_PATH;
	}

	QString themeDir() const
	{
		return m_themeDir;
	}

	const QString & backgroundPicFile() const
	{
		return m_backgroundPicFile;
	}

	QString trackIconsDir() const
	{
		return m_dataDir + TRACK_ICON_PATH;
	}

	const QString recoveryFile() const
	{
		return m_workingDir + "recover.mmp";
	}

	inline const QStringList & recentlyOpenedProjects() const
	{
		return m_recentlyOpenedProjects;
	}

	const QStringList& favoriteItems() { return m_favoriteItems; }

	QString localeDir() const
	{
		return m_dataDir + LOCALE_PATH;
	}

	const QString & version() const
	{
		return m_version;
	}

	// Used when the configversion attribute is not present in a configuration file.
	// Returns the appropriate config file version based on the LMMS version.
	unsigned int legacyConfigVersion();

	QString defaultVersion() const;

	static bool enableBlockedPlugins();

	static QStringList availableVstEmbedMethods();
	QString vstEmbedMethod() const;

	// Returns true if the working dir (e.g. ~/lmms) exists on disk.
	bool hasWorkingDir() const;

	void addRecentlyOpenedProject(const QString & _file);

	void addFavoriteItem(const QString& item);
	void removeFavoriteItem(const QString& item);
	bool isFavoriteItem(const QString& item);

	QString value(const QString& cls, const QString& attribute, const QString& defaultVal = "") const;

	void setValue(const QString & cls, const QString & attribute,
						const QString & value);
	void deleteValue(const QString & cls, const QString & attribute);

	void loadConfigFile(const QString & configFile = "");
	void saveConfigFile();


	void setWorkingDir(const QString & workingDir);
	void setVSTDir(const QString & vstDir);
	void setLADSPADir(const QString & ladspaDir);
	void setSF2Dir(const QString & sf2Dir);
	void setSF2File(const QString & sf2File);
	void setSTKDir(const QString & stkDir);
	void setGIGDir(const QString & gigDir);
	void setThemeDir(const QString & themeDir);
	void setBackgroundPicFile(const QString & backgroundPicFile);

	// Creates the working directory & subdirectories on disk.
	void createWorkingDir();

signals:
	void valueChanged( QString cls, QString attribute, QString value );
	void favoritesChanged();

private:
	static ConfigManager * s_instanceOfMe;

	ConfigManager();
	ConfigManager(const ConfigManager & _c);
	~ConfigManager() override;

	void upgrade_1_1_90();
	void upgrade_1_1_91();
	void upgrade_1_2_2();
	void upgrade();

	// List of all upgrade methods
	static const std::vector<UpgradeMethod> UPGRADE_METHODS;

	QString m_workingDir;
	QString m_dataDir;
	QString m_vstDir;
	QString m_ladspaDir;
	QString m_sf2Dir;
#ifdef LMMS_HAVE_FLUIDSYNTH
	QString m_sf2File;
#endif
#ifdef LMMS_HAVE_STK
	QString m_stkDir;
#endif
	QString m_gigDir;
	QString m_themeDir;
	QString m_backgroundPicFile;
	QString m_lmmsRcFile;
	QString m_version;
	unsigned int m_configVersion;
	QStringList m_recentlyOpenedProjects;
	QStringList m_favoriteItems;

	using stringPairVector = std::vector<QPair<QString, QString>>;
	using settingsMap = QMap<QString, stringPairVector>;
	settingsMap m_settings;


	friend class Engine;
};


} // namespace lmms

#endif // LMMS_CONFIG_MANAGER_H
