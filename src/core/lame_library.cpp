/*
 * lame_library.cpp - Manages loading and unloading of lame library dynamically
 *				 
 * Copyright (c) 2009 Andrew Kelley <superjoe30@gmail.com>
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


#include <QLibrary>
#include <QtGui/QMessageBox>


#include "lame_library.h"

LameLibrary::LameLibrary() : 
	m_lameLib( NULL )
{
	// dynamically load the lame library
	m_lameLib = new QLibrary(configManager::inst()->lameLibrary());
	if( ! m_lameLib->load() ) 
	{
		QMessageBox::information( NULL, QObject::tr( "Unable to load LAME" ),
			QObject::tr( "LMMS was unable to load Lame MP3 encoder. "
				"Please make sure "
				"you have Lame installed and then check your folder settings " 
				"and make sure you tell LMMS where libmp3lame.so.0 is." ),
			QMessageBox::Ok | QMessageBox::Default );
		delete m_lameLib;
		m_lameLib = NULL;
		return;
	}

	// grab the functions and stuff we need
	lame_init = (lame_init_t *)
	  m_lameLib->resolve("lame_init");
	get_lame_version = (get_lame_version_t *)
	  m_lameLib->resolve("get_lame_version");
	lame_init_params = (lame_init_params_t *)
	  m_lameLib->resolve("lame_init_params");
	lame_encode_buffer = (lame_encode_buffer_t *)
	  m_lameLib->resolve("lame_encode_buffer");
	lame_encode_buffer_interleaved = (lame_encode_buffer_interleaved_t *)
	  m_lameLib->resolve("lame_encode_buffer_interleaved");
	lame_encode_flush = (lame_encode_flush_t *)
	  m_lameLib->resolve("lame_encode_flush");
	lame_close = (lame_close_t *)
	  m_lameLib->resolve("lame_close");

	lame_set_in_samplerate = (lame_set_in_samplerate_t *)
	   m_lameLib->resolve("lame_set_in_samplerate");
	lame_set_out_samplerate = (lame_set_out_samplerate_t *)
	   m_lameLib->resolve("lame_set_out_samplerate");
	lame_set_num_channels = (lame_set_num_channels_t *)
	   m_lameLib->resolve("lame_set_num_channels");
	lame_set_quality = (lame_set_quality_t *)
	   m_lameLib->resolve("lame_set_quality");
	lame_set_brate = (lame_set_brate_t *)
	   m_lameLib->resolve("lame_set_brate");
	lame_set_VBR = (lame_set_VBR_t *)
	   m_lameLib->resolve("lame_set_VBR");
	lame_set_VBR_q = (lame_set_VBR_q_t *)
	   m_lameLib->resolve("lame_set_VBR_q");
	lame_set_VBR_min_bitrate_kbps = (lame_set_VBR_min_bitrate_kbps_t *)
	   m_lameLib->resolve("lame_set_VBR_min_bitrate_kbps");
	lame_set_mode = (lame_set_mode_t *) 
	   m_lameLib->resolve("lame_set_mode");
	lame_set_preset = (lame_set_preset_t *)
	   m_lameLib->resolve("lame_set_preset");
	lame_set_error_protection = (lame_set_error_protection_t *)
	   m_lameLib->resolve("lame_set_error_protection");
	lame_set_disable_reservoir = (lame_set_disable_reservoir_t *)
	   m_lameLib->resolve("lame_set_disable_reservoir");
	lame_set_padding_type = (lame_set_padding_type_t *)
	   m_lameLib->resolve("lame_set_padding_type");
	lame_set_bWriteVbrTag = (lame_set_bWriteVbrTag_t *)
	   m_lameLib->resolve("lame_set_bWriteVbrTag");

	lame_set_findReplayGain = (lame_set_findReplayGain_t *) 
		m_lameLib->resolve("lame_set_findReplayGain");
	lame_set_VBR_quality = (lame_set_VBR_quality_t *)
		m_lameLib->resolve("lame_set_VBR_quality");
	lame_set_VBR_mean_bitrate_kbps = (lame_set_VBR_mean_bitrate_kbps_t *)
		m_lameLib->resolve("lame_set_VBR_mean_bitrate_kbps");
	lame_set_VBR_max_bitrate_kbps = (lame_set_VBR_max_bitrate_kbps_t *)
		m_lameLib->resolve("lame_set_VBR_max_bitrate_kbps");

	lame_decode_init = (lame_decode_init_t*)
		m_lameLib->resolve("lame_decode_init");
	lame_decode1_headers = (lame_decode1_headers_t*)
		m_lameLib->resolve("lame_decode1_headers");
	lame_decode_headers = (lame_decode_headers_t*)
		m_lameLib->resolve("lame_decode_headers");
	lame_decode = (lame_decode_t*)
		m_lameLib->resolve("lame_decode");
	lame_decode_exit = (lame_decode_exit_t*)
		m_lameLib->resolve("lame_decode_exit");

	// These are optional
	lame_get_lametag_frame = (lame_get_lametag_frame_t *)
	   m_lameLib->resolve("lame_get_lametag_frame");
	lame_mp3_tags_fid = (lame_mp3_tags_fid_t *)
	   m_lameLib->resolve("lame_mp3_tags_fid");

	if (!lame_init ||
		!get_lame_version ||
		!lame_init_params ||
		!lame_encode_buffer ||
		!lame_encode_buffer_interleaved ||
		!lame_encode_flush ||
		!lame_close ||
		!lame_set_in_samplerate ||
		!lame_set_out_samplerate ||
		!lame_set_num_channels ||
		!lame_set_quality ||
		!lame_set_brate ||
		!lame_set_VBR ||
		!lame_set_VBR_q ||
		!lame_set_mode ||
		!lame_set_preset ||
		!lame_set_error_protection ||
		!lame_set_disable_reservoir ||
		!lame_set_padding_type ||
		!lame_set_bWriteVbrTag ||
		!lame_set_findReplayGain ||
		!lame_set_VBR_quality ||
		!lame_set_VBR_mean_bitrate_kbps ||
		!lame_set_VBR_max_bitrate_kbps ||
		!lame_decode_init ||
		!lame_decode1_headers ||
		!lame_decode_headers ||
		!lame_decode ||
		!lame_decode_exit)
	{
		// some symbols are missing
		QMessageBox::information( NULL, QObject::tr( "LAME missing symbols" ),
			QObject::tr( "Some symbols are missing from your Lame library. "
				"Make sure you have the latest version of Lame and/or LMMS "
				"installed." ), QMessageBox::Ok | QMessageBox::Default );
		m_lameLib->unload();
		delete m_lameLib;
		m_lameLib = NULL;
		return;
	}
}


LameLibrary::~LameLibrary()
{
	// release resources
	if( m_lameLib )
	{
		if( m_lameLib->isLoaded() )
			m_lameLib->unload();   

		delete m_lameLib;
		m_lameLib = NULL;
	}
}

// returns whether or not lame is correctly attached
bool LameLibrary::isLoaded()
{
	return m_lameLib != NULL;
}

/* vim: set tw=0 noexpandtab: */
