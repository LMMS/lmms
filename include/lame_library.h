/*
 * lame_library.h - Manages loading and unloading of lame library dynamically
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


#ifndef _LAME_LIBRARY_H_
#define _LAME_LIBRARY_H_

#include <QLibrary>
#include "lame.h"

#include "config_mgr.h"

class LameLibrary {
	private:
		// functions we'll be importing from lame
		typedef lame_global_flags *lame_init_t(void);
		typedef int lame_init_params_t(lame_global_flags*);
		typedef const char* get_lame_version_t(void);

		typedef int lame_encode_buffer_t (
			  lame_global_flags* gf,
			  const short int	buffer_l [],
			  const short int	buffer_r [],
			  const int		  nsamples,
			  unsigned char *	mp3buf,
			  const int		  mp3buf_size );

		typedef int lame_encode_buffer_interleaved_t(
			  lame_global_flags* gf,
			  short int		  pcm[],
			  int				num_samples,   /* per channel */
			  unsigned char*	 mp3buf,
			  int				mp3buf_size );

		typedef int lame_encode_flush_t(
			  lame_global_flags *gf,
			  unsigned char*	 mp3buf,
			  int				size );

		typedef int lame_close_t(lame_global_flags*);

		typedef int lame_set_in_samplerate_t(lame_global_flags*, int);
		typedef int lame_set_out_samplerate_t(lame_global_flags*, int);
		typedef int lame_set_num_channels_t(lame_global_flags*, int );
		typedef int lame_set_quality_t(lame_global_flags*, int);
		typedef int lame_set_brate_t(lame_global_flags*, int);
		typedef int lame_set_VBR_t(lame_global_flags *, vbr_mode);
		typedef int lame_set_VBR_q_t(lame_global_flags *, int);
		typedef int lame_set_VBR_min_bitrate_kbps_t(lame_global_flags *, int);
		typedef int lame_set_mode_t(lame_global_flags *, MPEG_mode);
		typedef int lame_set_preset_t(lame_global_flags *, int);
		typedef int lame_set_error_protection_t(lame_global_flags *, int);
		typedef int lame_set_disable_reservoir_t(lame_global_flags *, int);
		typedef int lame_set_padding_type_t(lame_global_flags *, Padding_type);
		typedef int lame_set_bWriteVbrTag_t(lame_global_flags *, int);
		typedef size_t lame_get_lametag_frame_t(const lame_global_flags *, unsigned char* buffer, size_t size);
		typedef void lame_mp3_tags_fid_t(lame_global_flags *, FILE *);

		typedef int lame_set_findReplayGain_t(lame_global_flags *, int);
		typedef int lame_set_VBR_quality_t(lame_global_flags *, float);
		typedef int lame_set_VBR_mean_bitrate_kbps_t(lame_global_flags *, int);
		typedef int lame_set_VBR_max_bitrate_kbps_t(lame_global_flags *, int);

		typedef int lame_decode_init_t(void);
		typedef int lame_decode1_headers_t(unsigned char *, int, short *,
			short *, mp3data_struct *);
		typedef int lame_decode_headers_t(unsigned char *, int, short *,
			short *, mp3data_struct *);
		typedef int lame_decode_t(unsigned char *, int, short *, short *);
		typedef int lame_decode_exit_t(void);


	public:

		LameLibrary(); // loads lame library
		~LameLibrary(); // unloads lame library


		bool isLoaded(); // returns whether or not lame is correctly attached

		/* function pointers to the symbols we get from the library */
		lame_init_t* lame_init;
		lame_init_params_t* lame_init_params;
		lame_encode_buffer_t* lame_encode_buffer;
		lame_encode_buffer_interleaved_t* lame_encode_buffer_interleaved;
		lame_encode_flush_t* lame_encode_flush;
		lame_close_t* lame_close;
		get_lame_version_t* get_lame_version;

		lame_set_in_samplerate_t* lame_set_in_samplerate;
		lame_set_out_samplerate_t* lame_set_out_samplerate;
		lame_set_num_channels_t* lame_set_num_channels;
		lame_set_quality_t* lame_set_quality;
		lame_set_brate_t* lame_set_brate;
		lame_set_VBR_t* lame_set_VBR;
		lame_set_VBR_q_t* lame_set_VBR_q;
		lame_set_VBR_min_bitrate_kbps_t* lame_set_VBR_min_bitrate_kbps;
		lame_set_mode_t* lame_set_mode;
		lame_set_preset_t* lame_set_preset;
		lame_set_error_protection_t* lame_set_error_protection;
		lame_set_disable_reservoir_t *lame_set_disable_reservoir;
		lame_set_padding_type_t *lame_set_padding_type;
		lame_set_bWriteVbrTag_t *lame_set_bWriteVbrTag;
		lame_get_lametag_frame_t *lame_get_lametag_frame;
		lame_mp3_tags_fid_t *lame_mp3_tags_fid;
		lame_set_findReplayGain_t *lame_set_findReplayGain;
		lame_set_VBR_quality_t *lame_set_VBR_quality;
		lame_set_VBR_mean_bitrate_kbps_t *lame_set_VBR_mean_bitrate_kbps;
		lame_set_VBR_max_bitrate_kbps_t *lame_set_VBR_max_bitrate_kbps;

		lame_decode_init_t *lame_decode_init;
		lame_decode1_headers_t *lame_decode1_headers;
		lame_decode_headers_t *lame_decode_headers;
		lame_decode_t *lame_decode;
		lame_decode_exit_t *lame_decode_exit;

	private:
		QLibrary * m_lameLib; // lame .so file

};


#endif

/* vim: set tw=0 expandtab: */
