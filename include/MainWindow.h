/*
 * main_window.h - declaration of class MainWindow, the main window of LMMS
 *
 * Copyright (c) 2004-2013 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtCore/QTimer>
#include <QtCore/QList>
#include <QtGui/QMainWindow>
#include <QtGui/QWhatsThis>

class QAction;
class QDomElement;
class QGridLayout;
class QMdiArea;

class configManager;
class PluginView;
class toolButton;


class MainWindow : public QMainWindow
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

	int addWidgetToToolBar( QWidget * _w, int _row = -1, int _col = -1 );
	void addSpacingToToolBar( int _size );


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
	bool saveProjectAsNewVersion( void );
	void showSettingsDialog( void );
	void aboutLMMS( void );
	void help( void );
	void toggleAutomationEditorWin( void );
	void toggleBBEditorWin( bool forceShow = false );
	void toggleSongEditorWin( void );
	void toggleProjectNotesWin( void );
	void toggleFxMixerWin( void );
	void togglePianoRollWin( void );
	void toggleControllerRack( void );

	void undo( void );
	void redo( void );


protected:
	virtual void closeEvent( QCloseEvent * _ce );
	virtual void focusOutEvent( QFocusEvent * _fe );
	virtual void keyPressEvent( QKeyEvent * _ke );
	virtual void keyReleaseEvent( QKeyEvent * _ke );
	virtual void timerEvent( QTimerEvent * _ev );


private:
	MainWindow( void );
	MainWindow( const MainWindow & );
	virtual ~MainWindow();

	void finalize( void );

	void toggleWindow( QWidget *window, bool forceShow = false );


	QMdiArea * m_workspace;

	QWidget * m_toolBar;
	QGridLayout * m_toolBarLayout;

	QMenu * m_templatesMenu;
	QMenu * m_recentlyOpenedProjectsMenu;
	int m_custom_templates_count;

	struct keyModifiers
	{
		keyModifiers() :
			m_ctrl( false ),
			m_shift( false ),
			m_alt( false )
		{
		}
		bool m_ctrl;
		bool m_shift;
		bool m_alt;
	} m_keyMods;

	QMenu * m_toolsMenu;
	QList<PluginView *> m_tools;

	QBasicTimer m_updateTimer;
	QTimer m_autoSaveTimer;


	friend class engine;


private slots:
	void browseHelp( void );
	void fillTemplatesMenu( void );
	void openRecentlyOpenedProject( QAction * _action );
	void showTool( QAction * _idx );
	void updateRecentlyOpenedProjectsMenu( void );


	void autoSave();

signals:
	void periodicUpdate( void );

} ;

#endif

