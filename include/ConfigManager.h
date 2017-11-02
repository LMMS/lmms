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


#ifndef CONFIG_MGR_H
#define CONFIG_MGR_H

#include "lmmsconfig.h"

#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include "export.h"
#include "MemoryManager.h"


class LmmsCore;


const QString PROJECTS_PATH = "projects/";
const QString TEMPLATE_PATH = "templates/";
const QString PRESETS_PATH = "presets/";
const QString SAMPLES_PATH = "samples/";
const QString GIG_PATH = "samples/gig/";
const QString SF_PATH = "samples/soundfont/";
const QString LADSPA_PATH ="plugins/ladspa/";
const QString DEFAULT_THEME_PATH = "themes/default/";
const QString TRACK_ICON_PATH = "track_icons/";
const QString LOCALE_PATH = "locale/";


class EXPORT ConfigManager
{
	MM_OPERATORS
public:
	static inline ConfigManager * inst()
	{
		if(s_instanceOfMe == NULL )
		{
			s_instanceOfMe = new ConfigManager();
		}
		return s_instanceOfMe;
	}
//-----------------------------------------------------------
	const QString & dataDir() const
	{
		return m_dataDir;
	}

	const QString & workingDir() const
	{
		return m_workingDir;
	}
//-----------------------------------------------------------
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
//-----------------------------------------------------------
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
//-----------------------------------------------------------
	const QString & vstDir() const
	{
		return m_vstDir;
	}

	const QString & ladspaDir() const
	{
		return m_ladspaDir;
	}

	const QString & sfDir() const
	{
		return m_sfDir;
	}

#ifdef LMMS_HAVE_FLUIDSYNTH
	const QString & sfFile() const
	{
		return m_sfFile;
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
//-----------------------------------------------------------
	QString userVstDir() const
	{
		return m_vstDir;
	}

	QString userLadspaDir() const
	{
		return workingDir() + LADSPA_PATH;
	}

	QString userSfDir() const
	{
		return workingDir() + SF_PATH;
	}

	QString userGigDir() const
	{
		return workingDir() + GIG_PATH;
	}
//-----------------------------------------------------------
	QString defaultThemeDir() const
	{
		return m_dataDir + DEFAULT_THEME_PATH;
	}

	QString themeDir() const
	{
		return m_themeDir;
	}

	QString trackIconsDir() const
	{
		return m_dataDir + TRACK_ICON_PATH;
	}

	const QString & backgroundPicFile() const
	{
		return m_backgroundPicFile;
	}
//-----------------------------------------------------------
	const QString recoveryFile() const
	{
		return m_workingDir + "recover.mmp";
	}

	inline const QStringList & recentlyOpenedProjects() const
	{
		return m_recentlyOpenedProjects;
	}
//-----------------------------------------------------------
	const QString & version() const
	{
		return m_version;
	}

	QString defaultVersion() const;
//-----------------------------------------------------------
	QString localeDir() const
	{
		return m_dataDir + LOCALE_PATH;
	}
//--------------------------------------------------

	// Returns true if the working dir (e.g. ~/lmms) exists on disk.
	bool hasWorkingDir() const;

	void addRecentlyOpenedProject(const QString & _file);

	const QString & value(const QString & cls,
					const QString & attribute) const;
	const QString & value(const QString & cls,
					const QString & attribute,
					const QString & defaultVal) const;
	void setValue(const QString & cls, const QString & attribute,
						const QString & value);
	void deleteValue(const QString & cls, const QString & attribute);

	void loadConfigFile(const QString & configFile = "");
	void saveConfigFile();


	void setWorkingDir(const QString & workingDir);
	void setVSTDir(const QString & vstDir);
	void setLADSPADir(const QString & ladspaDir);
	void setSFDir(const QString & sfDir);
	void setSFFile(const QString & sfFile);
	void setSTKDir(const QString & stkDir);
	void setGIGDir(const QString & gigDir);
	void setThemeDir(const QString & themeDir);
	void setBackgroundPicFile(const QString & backgroundPicFile);
	//void setVersion(const QString & _cv); <- This doesn't seem to be used anywhere.

	// Creates the working directory & subdirectories on disk.
	void createWorkingDir();


private:
	static ConfigManager * s_instanceOfMe;

	ConfigManager();
	ConfigManager(const ConfigManager & _c);
	~ConfigManager();

	void upgrade_1_1_90();
	void upgrade_1_1_91();
	void upgrade();

	QString m_workingDir;
	QString m_dataDir;
	QString m_vstDir;
	QString m_ladspaDir;
	QString m_sfDir;
#ifdef LMMS_HAVE_FLUIDSYNTH
	QString m_sfFile;
#endif
#ifdef LMMS_HAVE_STK
	QString m_stkDir;
#endif
	QString m_gigDir;
	QString m_themeDir;
	QString m_backgroundPicFile;
	QString m_lmmsRcFile;
	QString m_version;
	QStringList m_recentlyOpenedProjects;

	typedef QVector<QPair<QString, QString> > stringPairVector;
	typedef QMap<QString, stringPairVector> settingsMap;
	settingsMap m_settings;


	friend class LmmsCore;

} ;

#endif
