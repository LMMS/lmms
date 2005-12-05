/*
 * audio_sdl.h - device-class that performs PCM-output via SDL
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUDIO_SDL_H
#define _AUDIO_SDL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SDL_SDL_AUDIO_H

#ifndef SDL_AUDIO_SUPPORT
#define SDL_AUDIO_SUPPORT
#endif


#include SDL_SDL_H
#include SDL_SDL_AUDIO_H


#include "audio_device.h"
#include "tab_widget.h"


class QLineEdit;


class audioSDL : public audioDevice
{
public:
	audioSDL( Uint32 _sample_rate, bool & _success_ful );
	~audioSDL();

	inline static QString name( void )
	{
		return( setupWidget::tr( "SDL (Simple DirectMedia Layer)" ) );
	}


	class setupWidget : public audioDevice::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings( void );

	private:
		QLineEdit * m_device;

	} ;


private:
	virtual void startProcessing( void );
	virtual void stopProcessing( void );

	static void sdlAudioCallback( void * _udata, Uint8 * _buf, int _len );

	SDL_AudioSpec m_audioHandle;

	surroundSampleFrame * m_outBuf;

	bool m_convertEndian;

} ;

#endif

#endif
