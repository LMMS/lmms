/*
 * main_window.h - declaration of class mainWindow, the main window of LMMS
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H

#include "qt3support.h"

#ifdef QT4

#include <QMainWindow>
#include <QWorkspace>
#include <QWhatsThis>

#else

#include <qmainwindow.h>
#include <qworkspace.h>
#include <qwhatsthis.h>

#endif


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "types.h"
#include "engine.h"


class QGridLayout;
class QSplashScreen;

class configManager;
class toolButton;



class mainWindow : public QMainWindow, public engineObject
{
	Q_OBJECT
public:
	inline QWorkspace * workspace( void )
	{
		return( m_workspace );
	}

	inline QWidget * toolBar( void )
	{
		return( m_toolBar );
	}

	int addWidgetToToolBar( QWidget * _w, int _row = -1, int _col = -1 );
	void addSpacingToToolBar( int _size );

	void FASTCALL resetWindowTitle( const QString & _add = "" );

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

	static QSplashScreen * s_splashScreen;


public slots:
	inline void emptySlot( void )
	{
	}
	inline void enterWhatsThisMode( void )
	{
		QWhatsThis::enterWhatsThisMode();
	}
	void createNewProject( void );
	void createNewProjectFromTemplate( int _idx );
	void openProject( void );
	bool saveProject( void );
	bool saveProjectAs( void );
	void showSettingsDialog( void );
	void aboutLMMS( void );
	void help( void );
	void toggleBBEditorWin( void );
	void toggleSongEditorWin( void );
	void toggleProjectNotesWin( void );
	void togglePianoRollWin( void );


protected:
	virtual void closeEvent( QCloseEvent * _ce );
	virtual void focusOutEvent( QFocusEvent * _fe );
	virtual void keyPressEvent( QKeyEvent * _ke );
	virtual void keyReleaseEvent( QKeyEvent * _ke );


private:
	mainWindow( engine * _engine );
	mainWindow( const mainWindow & );
	virtual ~mainWindow();

	void finalize( void );


	QWorkspace * m_workspace;

	QWidget * m_toolBar;
	QGridLayout * m_toolBarLayout;

	QMenu * m_templatesMenu;

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


	friend class engine;

} ;

#endif

