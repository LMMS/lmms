/*
 * audio_dummy.h - dummy-audio-device
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUDIO_DUMMY_H
#define _AUDIO_DUMMY_H

#include "audio_device.h"


class audioDummy : public audioDevice
{
public:
	audioDummy( Uint32 _sample_rate, bool & _success_ful ) :
		audioDevice( _sample_rate, DEFAULT_CHANNELS )
	{
		_success_ful = TRUE;
	}

	virtual ~audioDummy()
	{
	}

	inline static QString name( void )
	{
		return( setupWidget::tr( "Dummy (no sound output)" ) );
	}


	class setupWidget : public audioDevice::setupWidget
	{
	public:
		setupWidget( QWidget * _parent ) :
			audioDevice::setupWidget( audioDummy::name(), _parent )
		{
		}

		virtual ~setupWidget()
		{
		}

		virtual void saveSettings( void )
		{
		}

	} ;


private:
	virtual void FASTCALL writeBufferToDev( surroundSampleFrame *,
						Uint32 /*_frames*/, float )
	{
		//usleep( (Uint32)( _frames * 1000.0f * 1000.0f /
		//					DEFAULT_SAMPLE_RATE ) );
	}

} ;


#endif
