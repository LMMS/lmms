
/*
 * SetupDialog.h - dialog for setting up LMMS
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

#ifndef SETUP_DIALOG_H
#define SETUP_DIALOG_H

#include <QDialog>
#include <QtCore/QMap>

#include "LedCheckbox.h"
#include "lmmsconfig.h"
#include "AudioDevice.h"
#include "MidiClient.h"
#include "MidiSetupWidget.h"

#include "AudioDeviceSetupWidget.h"


class QComboBox;
class QLabel;
class QLineEdit;
class QSlider;

class TabBar;

class SetupDialog : public QDialog
{
	Q_OBJECT
public:
	enum ConfigTabs
	{
		GeneralSettings,
		PathSettings,
		PerformanceSettings,
		AudioSettings,
		MidiSettings
	} ;

	SetupDialog( ConfigTabs _tab_to_open = GeneralSettings );
	virtual ~SetupDialog();


protected slots:
	virtual void accept();


private slots:
	// general settings widget
	void setBufferSize( int _value );
	void resetBufSize();
	void displayBufSizeHelp();

	// path settings widget
	void setWorkingDir( const QString & _wd );
	void setVSTDir( const QString & _vd );
	void setGIGDir( const QString & _gd );
	void setSF2Dir( const QString & _sfd );
	void setArtworkDir( const QString & _ad );
	void setLADSPADir( const QString & _ld );
	void setSTKDir( const QString & _sd );
	void setDefaultSoundfont( const QString & _sf );
	void setBackgroundArtwork( const QString & _ba );

	// performance settings widget
	void setAutoSaveInterval( int time );
	void resetAutoSave();
	void displaySaveIntervalHelp();

	// audio settings widget
	void audioInterfaceChanged( const QString & _driver );
	void displayAudioHelp();

	// MIDI settings widget
	void midiInterfaceChanged( const QString & _driver );
	void displayMIDIHelp();


	void toggleToolTips( bool _enabled );
	void toggleWarnAfterSetup( bool _enabled );
	void toggleDisplaydBFS( bool _enabled );
	void toggleMMPZ( bool _enabled );
	void toggleDisableBackup( bool _enabled );
	void toggleOpenLastProject( bool _enabled );
	void toggleHQAudioDev( bool _enabled );

	void openWorkingDir();
	void openVSTDir();
	void openGIGDir();
	void openSF2Dir();
	void openArtworkDir();
	void openLADSPADir();
	void openSTKDir();
	void openDefaultSoundfont();
	void openBackgroundArtwork();

	void toggleSmoothScroll( bool _enabled );
	void toggleAutoSave( bool _enabled );
	void toggleRunningAutoSave( bool _enabled );
	void toggleOneInstrumentTrackWindow( bool _enabled );
	void toggleCompactTrackButtons( bool _enabled );
	void toggleSyncVSTPlugins( bool _enabled );
	void toggleAnimateAFP( bool _enabled );
	void toggleNoteLabels( bool en );
	void toggleDisplayWaveform( bool en );
	void toggleDisableAutoquit( bool en );

	void setLanguage( int lang );


private:
	TabBar * m_tabBar;

	QSlider * m_bufSizeSlider;
	QLabel * m_bufSizeLbl;
	int m_bufferSize;

	bool m_toolTips;
	bool m_warnAfterSetup;
	bool m_displaydBFS;
	bool m_MMPZ;
	bool m_disableBackup;
	bool m_openLastProject;
	bool m_hqAudioDev;
	QString m_lang;
	QStringList m_languages;


	QLineEdit * m_wdLineEdit;
	QLineEdit * m_vdLineEdit;
	QLineEdit * m_adLineEdit;
	QLineEdit * m_ladLineEdit;
	QLineEdit * m_gigLineEdit;
	QLineEdit * m_sf2LineEdit;
#ifdef LMMS_HAVE_FLUIDSYNTH
	QLineEdit * m_sfLineEdit;
#endif
#ifdef LMMS_HAVE_STK
	QLineEdit * m_stkLineEdit;
#endif
	QLineEdit * m_baLineEdit;

	QString m_workingDir;
	QString m_vstDir;
	QString m_artworkDir;
	QString m_ladDir;
	QString m_gigDir;
	QString m_sf2Dir;
#ifdef LMMS_HAVE_FLUIDSYNTH
	QString m_defaultSoundfont;
#endif
#ifdef LMMS_HAVE_STK
	QString m_stkDir;
#endif
	QString m_backgroundArtwork;

	bool m_smoothScroll;
	bool m_enableAutoSave;
	bool m_enableRunningAutoSave;
	int m_saveInterval;
	QSlider * m_saveIntervalSlider;
	QLabel * m_saveIntervalLbl;
	LedCheckBox * m_autoSave;
	LedCheckBox * m_runningAutoSave;

	bool m_oneInstrumentTrackWindow;
	bool m_compactTrackButtons;
	bool m_syncVSTPlugins;
	bool m_animateAFP;
	bool m_printNoteLabels;
	bool m_displayWaveform;
	bool m_disableAutoQuit;

	typedef QMap<QString, AudioDeviceSetupWidget *> AswMap;
	typedef QMap<QString, MidiSetupWidget *> MswMap;
	typedef QMap<QString, QString> trMap;

	QComboBox * m_audioInterfaces;
	AswMap m_audioIfaceSetupWidgets;
	trMap m_audioIfaceNames;

	QComboBox * m_midiInterfaces;
	MswMap m_midiIfaceSetupWidgets;
	trMap m_midiIfaceNames;


} ;


#endif
