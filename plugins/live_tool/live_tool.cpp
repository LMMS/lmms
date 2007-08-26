/*
 * live_tool.cpp - tool for live performance
 *
 * Copyright (c) 2006-2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#include <QtGui/QKeyEvent>

#include "live_tool.h"
#include "bb_editor.h"
#include "engine.h"
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
				"Tool for live performance" ),
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
	const QPixmap background = PLUGIN_NAME::getIconPixmap( "artwork" );

	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), background );
	setPalette( pal );
	setFixedSize( background.size() );

	setWhatsThis( tr( 
		"This tool is intended to be used in live performances, though "
		"you can use it for music production as well.\n"
		"The following keys will work only if this window is active.\n"
		"The spacebar toggles play and pause in the Song Editor.\n"
		"F1-F10 keys mute the first 10 instruments in the "
		"Beat+Baseline Editor." ) );

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
			if( engine::getSongEditor()->playing() )
			{
				engine::getSongEditor()->pause();
			}
			else if( engine::getSongEditor()->paused() &&
				engine::getSongEditor()->playMode() ==
							songEditor::PLAY_SONG )
			{
				engine::getSongEditor()->resumeFromPause();
			}
			else
			{
				engine::getSongEditor()->play();
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
	if( _n > 0 && _n < engine::getBBEditor()->tracks().count() )
	{
		track * t = engine::getBBEditor()->tracks().at( _n );
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
