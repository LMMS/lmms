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

	struct Config
	{
		struct Ui
		{
			bool animateafp = true;
			std::string autoscroll = "stepped"; // TODO enum
			bool compacttrackbuttons = false;
			std::string detachbehavior = "show"; // TODO enum; other: "hide", "detached"
			bool disableautoquit = true; // enable?
			bool displaywaveform = false;
			bool enableautosave = true;
			bool enablerunningautosave = false; // maybe unite with prev.?
			bool letpreviewsfinish = false;
			bool mixerchanneldeletionwarning = true;
			bool oneinstrumenttrackwindow = false;
			bool printnotelabels = false;
			uint32_t saveinterval = 2;
			bool showfaderticks = false;
			bool sidebaronright = false;
			bool smoothscroll = false;
			bool trackdeletionwarning = true;
			bool vstalwaysontop = false;
			std::string vstembedmethod = "none"; // TODO enum
		} ui;

		struct App
		{
			bool configured = false; // default to true?
			bool disablebackup = false; // -> enable?
			std::string language = "en"; // TODO enum?
			std::string loopmarkermode = "dual"; // TODO enum
			bool nanhandler = true;
			bool nommpz = false; // -> compressmmp?
			bool openlastproject = false;
			bool sololegacybehavior = false;
		} app;

		struct Paths // TODO use std::filesystem::path
		{
			std::string backgroundtheme;
			std::string defaultsf2;
			std::string gigdir;
			std::string ladspadir;
			std::string sf2dir;
			std::string stkdir;
			std::string theme = "data:/themes/default";
			std::string vstdir;
			std::string workingdir;
		} paths;

		struct Tooltips { bool disabled = false; } tooltips; // -> enable?

		struct AudioEngine
		{
			std::string audiodev;
			std::string mididev;
			uint32_t framesperaudiobuffer = 256;
			uint32_t samplerate = 44100;
		} audioengine;

		struct AudioAlsa // TODO make these a union?
		{
			uint32_t channels = 2;
			std::string device = "null";
		} audioalsa;

		struct AudioJack
		{
			std::string clientname = "lmms";
			std::string output1;
			std::string output2;
			std::string input1;
			std::string input2;
		} audiojack;

		struct AudioOss
		{
			uint32_t channels = 2;
			std::string device = "/dev/dsp";
		} audiooss;

		struct AudioPA
		{
			uint32_t channels = 2;
			std::string device = "default";
		} audiopa;

		struct AudioPortAudio
		{
			std::string inputdevice;
			std::string outputdevice;
			uint32_t inputchannels = 0;
			uint32_t outputchannels = 0;
			std::string backend;
		} audioportaudio;

		struct AudioSDL
		{
			std::string inputdevice;
			std::string device;
		} audiosdl;

		struct AudioSndio
		{
			uint32_t channels = 2;
			std::string device;
		} audiosndio;

		struct AudioSoundio
		{
			std::string backend;
			std::string out_device_id;
			std::string out_device_raw; // TODO probably bool
		} audiosoundio;

		struct Midi
		{
			bool autoquantize = false;
			std::string midiautoassign = "none"; // TODO check string
		} midi;

		struct MidiAlsaRaw { std::string device = "default"; } MidiAlsaRaw;
		struct MidiAlsaSeq { std::string device = "default"; } Midialsaseq;
		struct MidiJack { std::string device = "lmms"; } MidiJack;
		struct MidiSndio { std::string device; } MidiSndio;
		struct MidiOss { std::string device = "/dev/midi"; } midioss;

		//std::vector<std::string> recentfiles; // TODO
		//std::vector<std::string> favoriteitems; // TODO
	} config;


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
