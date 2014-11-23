/*
 * setup_dialog.h - dialog for setting up LMMS
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef _SETUP_DIALOG_H
#define _SETUP_DIALOG_H

#include <QtGui/QDialog>
#include <QtCore/QMap>

#include "lmmsconfig.h"
#include "AudioDevice.h"
#include "MidiClient.h"


class QComboBox;
class QLabel;
class QLineEdit;
class QSlider;

class tabBar;


class setupDialog : public QDialog
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

	setupDialog( ConfigTabs _tab_to_open = GeneralSettings );
	virtual ~setupDialog();


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
	void setArtworkDir( const QString & _ad );
	void setFLDir( const QString & _fd );
	void setLADSPADir( const QString & _ld );
	void setSTKDir( const QString & _sd );
	void setDefaultSoundfont( const QString & _sf );
	void setBackgroundArtwork( const QString & _ba );
	
	// audio settings widget
	void audioInterfaceChanged( const QString & _driver );
	void displayAudioHelp();

	// MIDI settings widget
	void midiInterfaceChanged( const QString & _driver );
	void displayMIDIHelp();


	void toggleToolTips( bool _enabled );
	void toggleWarnAfterSetup( bool _enabled );
	void toggleDisplaydBV( bool _enabled );
	void toggleMMPZ( bool _enabled );
	void toggleHQAudioDev( bool _enabled );

	void openWorkingDir();
	void openVSTDir();
	void openArtworkDir();
	void openFLDir();
	void openLADSPADir();
	void openSTKDir();
	void openDefaultSoundfont();
	void openBackgroundArtwork();

	void toggleSmoothScroll( bool _enabled );
	void toggleAutoSave( bool _enabled );
	void toggleOneInstrumentTrackWindow( bool _enabled );
	void toggleCompactTrackButtons( bool _enabled );
	void toggleSyncVSTPlugins( bool _enabled );
	void toggleAnimateAFP( bool _enabled );
	void toggleNoteLabels( bool en );
	void toggleDisplayWaveform( bool en );


private:
	tabBar * m_tabBar;

	QSlider * m_bufSizeSlider;
	QLabel * m_bufSizeLbl;
	int m_bufferSize;

	bool m_toolTips;
	bool m_warnAfterSetup;
	bool m_displaydBV;
	bool m_MMPZ;
	bool m_hqAudioDev;


	QLineEdit * m_wdLineEdit;
	QLineEdit * m_vdLineEdit;
	QLineEdit * m_adLineEdit;
	QLineEdit * m_fdLineEdit;
	QLineEdit * m_ladLineEdit;
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
	QString m_flDir;
	QString m_ladDir;
#ifdef LMMS_HAVE_FLUIDSYNTH
	QString m_defaultSoundfont;
#endif
#ifdef LMMS_HAVE_STK
	QString m_stkDir;
#endif
	QString m_backgroundArtwork;

	bool m_smoothScroll;
	bool m_enableAutoSave;
	bool m_oneInstrumentTrackWindow;
	bool m_compactTrackButtons;
	bool m_syncVSTPlugins;
	bool m_animateAFP;
	bool m_printNoteLabels;
	bool m_displayWaveform;

	typedef QMap<QString, AudioDevice::setupWidget *> AswMap;
	typedef QMap<QString, MidiClient::setupWidget *> MswMap;
	typedef QMap<QString, QString> trMap;

	QComboBox * m_audioInterfaces;
	AswMap m_audioIfaceSetupWidgets;
	trMap m_audioIfaceNames;

	QComboBox * m_midiInterfaces;
	MswMap m_midiIfaceSetupWidgets;
	trMap m_midiIfaceNames;


} ;


#endif
