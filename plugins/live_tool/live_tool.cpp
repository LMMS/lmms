/*
 * live_tool.cpp - tool for live performance
 *
 * Copyright (c) 2006 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#include "live_tool.h"
#include "bb_editor.h"
#include "song_editor.h"

#ifdef Q_WS_X11

#include <X11/Xlib.h>

#endif

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"




extern "C"
{

plugin::descriptor live_tool_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"LiveTool",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"tool for live performance" ),
	"Javier Serrano Polo <jasp00/at/users.sourceforge.net>",
	0x0100,
	plugin::Tool,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) ),
	NULL
} ;

}




liveTool::liveTool( mainWindow * _window ) :
	tool( _window, &live_tool_plugin_descriptor )
{
	QPixmap background = PLUGIN_NAME::getIconPixmap( "artwork" );
	setPaletteBackgroundPixmap( background );
	setFixedSize( background.size() );

	hide();
}




liveTool::~liveTool()
{
}




QString liveTool::nodeName( void ) const
{
	return( live_tool_plugin_descriptor.name );
}




void liveTool::keyPressEvent( QKeyEvent * _ke )
{
	switch( _ke->key() )
	{
		case Qt::Key_Space:
			if( eng()->getSongEditor()->playing() )
			{
				eng()->getSongEditor()->pause();
			}
			else if( eng()->getSongEditor()->paused() &&
				eng()->getSongEditor()->playMode() ==
							songEditor::PLAY_SONG )
			{
				eng()->getSongEditor()->resumeFromPause();
			}
			else
			{
				eng()->getSongEditor()->play();
			}
			break;

		default:
			_ke->ignore();
			break;
	}
}




#ifdef Q_WS_X11
bool liveTool::x11Event( XEvent * _xe )
{
	if( _xe->type == KeyPress )
	{
		unsigned keycode = _xe->xkey.keycode;
		// F1 to F10
		if( 67 <= keycode && keycode <= 76 )
		{
			toggleInstrument( keycode - 67 );
			return( TRUE );
		}
	}
	return( FALSE );
}
#endif




void liveTool::toggleInstrument( int _n )
{
	bool track_exists;
	track * t = eng()->getBBEditor()->tracks().at( _n, &track_exists );
	if( track_exists )
	{
		t->setMuted( !t->muted() );
	}
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new liveTool( static_cast<mainWindow *>( _data ) ) );
}

}
