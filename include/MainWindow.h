/*
 * MainWindow.h - declaration of class MainWindow, the main window of LMMS
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QtCore/QBasicTimer>
#include <QtCore/QTimer>
#include <QtCore/QList>
#include <QtGui/QMainWindow>

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
	QMdiArea* workspace()
	{
		return m_workspace;
	}

	QWidget* toolBar()
	{
		return m_toolBar;
	}

	int addWidgetToToolBar( QWidget * _w, int _row = -1, int _col = -1 );
	void addSpacingToToolBar( int _size );


	///
	/// \brief	Asks whether changes made to the project are to be saved.
	///
	/// Opens a dialog giving the user the choice to (a) confirm his choice
	/// (such as opening a new file), (b) save the current project before
	/// proceeding or (c) cancel the process.
	///
	/// Every function that replaces the current file (e.g. creates new file,
	/// opens another file...) must call this before and may only proceed if
	/// this function returns true.
	///
	/// \return	true if the user allows the software to proceed, false if they
	///         cancel the action.
	///
	bool mayChangeProject();


	void clearKeyModifiers();

	bool isCtrlPressed()
	{
		return m_keyMods.m_ctrl;
	}

	bool isShiftPressed()
	{
		return m_keyMods.m_shift;
	}

	bool isAltPressed()
	{
		return m_keyMods.m_alt;
	}

	static void saveWidgetState( QWidget * _w, QDomElement & _de );
	static void restoreWidgetState( QWidget * _w, const QDomElement & _de );


public slots:
	void resetWindowTitle();

	void emptySlot();
	void enterWhatsThisMode();
	void createNewProject();
	void createNewProjectFromTemplate( QAction * _idx );
	void openProject();
	bool saveProject();
	bool saveProjectAs();
	bool saveProjectAsNewVersion();
	void showSettingsDialog();
	void aboutLMMS();
	void help();
	void toggleAutomationEditorWin();
	void toggleBBEditorWin( bool forceShow = false );
	void toggleSongEditorWin();
	void toggleProjectNotesWin();
	void toggleFxMixerWin();
	void togglePianoRollWin();
	void toggleControllerRack();

	void updatePlayPauseIcons();

	void undo();
	void redo();


protected:
	virtual void closeEvent( QCloseEvent * _ce );
	virtual void focusOutEvent( QFocusEvent * _fe );
	virtual void keyPressEvent( QKeyEvent * _ke );
	virtual void keyReleaseEvent( QKeyEvent * _ke );
	virtual void timerEvent( QTimerEvent * _ev );


private:
	MainWindow();
	MainWindow( const MainWindow & );
	virtual ~MainWindow();

	void finalize();

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
	void browseHelp();
	void fillTemplatesMenu();
	void openRecentlyOpenedProject( QAction * _action );
	void showTool( QAction * _idx );
	void updateRecentlyOpenedProjectsMenu();


	void autoSave();

signals:
	void periodicUpdate();

} ;

#endif

