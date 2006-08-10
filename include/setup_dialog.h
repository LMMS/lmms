/*
 * setup_dialog.h - dialog for setting up LMMS
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _SETUP_DIALOG_H
#define _SETUP_DIALOG_H

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QDialog>
#include <QtCore/QMap>

#else

#include <qdialog.h>
#include <qmap.h>

#endif


#include "audio_device.h"
#include "midi_client.h"
#include "ladspa_manager.h"


class QComboBox;
class QLabel;
class QLineEdit;
class QSlider;

class tabBar;


class setupDialog : public QDialog, public engineObject
{
	Q_OBJECT
public:
	enum configTabs
	{
		GENERAL_SETTINGS, DIRECTORY_SETTINGS, PERFORMANCE_SETTINGS,
		AUDIO_SETTINGS, MIDI_SETTINGS
	} ;

	setupDialog( engine * _engine,
				configTabs _tab_to_open = GENERAL_SETTINGS );
	virtual ~setupDialog();


protected slots:
	virtual void accept( void );


private slots:
	// general settings widget
	void setBufferSize( int _value );
	void resetBufSize( void );
	void displayBufSizeHelp( void );

	// directory settings widget
	void setWorkingDir( const QString & _wd );
	void setVSTDir( const QString & _vd );
	void setArtworkDir( const QString & _ad );
	void setFLDir( const QString & _fd );
	void setLADSPADir( const QString & _fd );

	// audio settings widget
	void audioInterfaceChanged( const QString & _driver );
	void displayAudioHelp( void );

	// MIDI settings widget
	void midiInterfaceChanged( const QString & _driver );
	void displayMIDIHelp( void );


	void toggleToolTips( bool _disabled );
	void toggleKnobUsability( bool _classical );
	void toggleGIMPLikeWindows( bool _enabled );
	void toggleNoWizard( bool _enabled );
	void toggleNoMsgAfterSetup( bool _enabled );
	void toggleDisplaydBV( bool _enabled );

	void openWorkingDir( void );
	void openVSTDir( void );
	void openArtworkDir( void );
	void openFLDir( void );
	void openLADSPADir( void );


	void toggleDisableChActInd( bool _disabled );
	void toggleManualChPiano( bool _enabled );


private:
	tabBar * m_tabBar;

	QSlider * m_bufSizeSlider;
	QLabel * m_bufSizeLbl;
	int m_bufferSize;

	bool m_disableToolTips;
	bool m_classicalKnobUsability;
	bool m_gimpLikeWindows;
	bool m_noWizard;
	bool m_noMsgAfterSetup;
	bool m_displaydBV;


	QLineEdit * m_wdLineEdit;
	QLineEdit * m_vdLineEdit;
	QLineEdit * m_adLineEdit;
	QLineEdit * m_fdLineEdit;
#ifdef LADSPA_SUPPORT
	QLineEdit * m_ladLineEdit;
#endif

	QString m_workingDir;
	QString m_vstDir;
	QString m_artworkDir;
	QString m_flDir;
#ifdef LADSPA_SUPPORT
	QString m_ladDir;
#endif

	bool m_disableChActInd;
	bool m_manualChPiano;


	typedef QMap<QString, audioDevice::setupWidget *> aswMap;
	typedef QMap<QString, midiClient::setupWidget *> mswMap;

	QComboBox * m_audioInterfaces;
	aswMap m_audioIfaceSetupWidgets;

	QComboBox * m_midiInterfaces;
	mswMap m_midiIfaceSetupWidgets;


} ;


#endif
