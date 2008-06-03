/*
 * live_tool.cpp - tool for live performance
 *
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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
#include <QtGui/QLayout>

#include "live_tool.h"
#include "bb_track_container.h"
#include "engine.h"
#include "song.h"

#ifdef Q_WS_X11

#include <X11/Xlib.h>

#endif

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"




extern "C"
{

plugin::descriptor PLUGIN_EXPORT livetool_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"LiveTool",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Tool for live performance" ),
	"Javier Serrano Polo <jasp00/at/users.sourceforge.net>",
	0x0100,
	plugin::Tool,
	new pluginPixmapLoader( "logo" ),
	NULL
} ;


// neccessary for getting instance out of shared lib
plugin * PLUGIN_EXPORT lmms_plugin_main( model * _parent, void * _data )
{
	return( new liveTool( _parent ) );
}

}




liveTool::liveTool( model * _parent ) :
	tool( &livetool_plugin_descriptor, _parent )
{
}




liveTool::~liveTool()
{
}




QString liveTool::nodeName( void ) const
{
	return( livetool_plugin_descriptor.name );
}







liveToolView::liveToolView( tool * _tool ) :
	toolView( _tool )
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
	if( parentWidget() )
	{
		parentWidget()->hide();
		parentWidget()->layout()->setSizeConstraint(
							QLayout::SetFixedSize );
	}
}




liveToolView::~liveToolView()
{
}




void liveToolView::keyPressEvent( QKeyEvent * _ke )
{
	switch( _ke->key() )
	{
		case Qt::Key_Space:
			if( engine::getSong()->isPlaying() )
			{
				engine::getSong()->pause();
			}
			else if( engine::getSong()->isPaused() &&
				engine::getSong()->playMode() ==
							song::Mode_PlaySong )
			{
				engine::getSong()->resumeFromPause();
			}
			else
			{
				engine::getSong()->play();
			}
			break;

		default:
			_ke->ignore();
			break;
	}
}




void liveToolView::mousePressEvent( QMouseEvent * _me )
{
	// MDI window gets focus otherwise
	setFocus();
	_me->accept();
}




#ifdef Q_WS_X11
bool liveToolView::x11Event( XEvent * _xe )
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




void liveToolView::toggleInstrument( int _n )
{
	if( _n < engine::getBBTrackContainer()->tracks().count() )
	{
		track * t = engine::getBBTrackContainer()->tracks().at( _n );
		t->setMuted( !t->isMuted() );
	}
}





#include "live_tool.moc"
