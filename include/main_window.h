/*
 * main_window.h - declaration of class mainWindow, the main window of LMMS
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H

#include <QtCore/QBasicTimer>
#include <QtCore/QList>
#include <QtGui/QMainWindow>
#include <QtGui/QWhatsThis>

class QAction;
class QDomElement;
class QGridLayout;
class QHBoxLayout;
class QMdiArea;
class QCheckBox;
class QRadioButton;

class ResourceBrowser;
class lcdSpinBox;
class meterDialog;
class automatableSlider;
class textFloat;

class configManager;
class pluginView;
class toolButton;

enum ProjectPlaybackMode
{
	PPM_Song = 0,
	PPM_BB = 1,
	PPM_PianoRoll
};

class mainWindow : public QMainWindow
{
	Q_OBJECT
public:
	inline QMdiArea * workspace( void )
	{
		return( m_workspace );
	}

	inline QWidget * toolBar( void )
	{
		return( m_toolBar );
	}

	//int addWidgetToToolBar( QWidget * _w, int _row = -1, int _col = -1 );
	//void addSpacingToToolBar( int _size );


	// every function that replaces current file (e.g. creates new file,
	// opens another file...) has to call this before and may only process
	// if this function returns true
	bool mayChangeProject( void );


	void clearKeyModifiers( void );

	inline bool isCtrlPressed( void )
	{
		return( m_keyMods.m_ctrl );
	}

	inline bool isShiftPressed( void )
	{
		return( m_keyMods.m_shift );
	}

	inline bool isAltPressed( void )
	{
		return( m_keyMods.m_alt );
	}

	static void saveWidgetState( QWidget * _w, QDomElement & _de );
	static void restoreWidgetState( QWidget * _w, const QDomElement & _de );

	inline ProjectPlaybackMode playbackMode() const
	{
		return m_playbackMode;	
	}

	void setPlaybackMode( ProjectPlaybackMode _playbackMode );


public slots:
	void resetWindowTitle( void );

	inline void emptySlot( void )
	{
	}
	inline void enterWhatsThisMode( void )
	{
		QWhatsThis::enterWhatsThisMode();
	}
	void createNewProject( void );
	void createNewProjectFromTemplate( QAction * _idx );
	void openProject( void );
	bool saveProject( void );
	bool saveProjectAs( void );
	void showSettingsDialog( void );
	void aboutLMMS( void );
	void help( void );
	void toggleAutomationEditorWin( void );
	void toggleBBEditorWin( void );
	void toggleSongEditorWin( void );
	void toggleProjectNotesWin( void );
	void toggleFxMixerWin( void );
	void togglePianoRollWin( void );
	void toggleControllerRack( void );

	void undo( void );
	void redo( void );

	void loadResource();

	void toggleRecordAutomation( bool );

protected:
	virtual void closeEvent( QCloseEvent * _ce );
	virtual void focusOutEvent( QFocusEvent * _fe );
	virtual void keyPressEvent( QKeyEvent * _ke );
	virtual void keyReleaseEvent( QKeyEvent * _ke );
	virtual void timerEvent( QTimerEvent * _ev );


private:
	mainWindow( void );
	mainWindow( const mainWindow & );
	virtual ~mainWindow();

	void finalize( void );

	void toggleWindow( QWidget * _w );


	QMdiArea * m_workspace;

	QWidget * m_toolBar;
	QHBoxLayout * m_toolBarLayout;
	QCheckBox * m_chkrAudio;
	QCheckBox * m_chkrAutomation;
	QCheckBox * m_chkrMidi;
	QRadioButton * m_radpSong;
	QRadioButton * m_radpBB;
	QRadioButton * m_radpPianoRoll;

	ProjectPlaybackMode m_playbackMode;
	
	lcdSpinBox * m_tempoSpinBox;
	meterDialog * m_timeSigDisplay;
		
	automatableSlider * m_masterVolumeSlider;
	automatableSlider * m_masterPitchSlider;
		
	textFloat * m_mvsStatus;
	textFloat * m_mpsStatus;
		
	QMenu * m_templatesMenu;
	QMenu * m_recentlyOpenedProjectsMenu;
	int m_custom_templates_count;

	struct keyModifiers
	{
		keyModifiers() :
			m_ctrl( FALSE ),
			m_shift( FALSE ),
			m_alt( FALSE )
		{
		}
		bool m_ctrl;
		bool m_shift;
		bool m_alt;
	} m_keyMods;

	QMenu * m_toolsMenu;
	QList<pluginView *> m_tools;

	QBasicTimer m_updateTimer;

	ResourceBrowser * m_resourceBrowser;

	friend class engine;


private slots:
	void setHighQuality( bool );
	
	void masterVolumeChanged( int _new_val );
	void masterVolumePressed( void );
	void masterVolumeMoved( int _new_val );
	void masterVolumeReleased( void );
	void masterPitchChanged( int _new_val );
	void masterPitchPressed( void );
	void masterPitchMoved( int _new_val );
	void masterPitchReleased( void );
		
	void browseHelp( void );
	void fillTemplatesMenu( void );
	void openRecentlyOpenedProject( QAction * _action );
	void showTool( QAction * _idx );
	void updateRecentlyOpenedProjectsMenu( void );

	void playbackSongClicked( bool );
	void playbackBBClicked( bool );
	void playbackPianoRollClicked( bool );
	
	void shortcutSpacePressed( void );
	void shortcutLPressed( void );

	void play( void );
	void record( void );
	void playAndRecord( void );
	void stop( void );

signals:
	void periodicUpdate( void );

} ;

#endif

