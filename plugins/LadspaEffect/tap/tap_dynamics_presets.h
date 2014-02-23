/*                                                     -*- linux-c -*-
    Copyright (C) 2004 Tom Szilagyi
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    $Id: tap_dynamics_presets.h,v 1.1 2004/05/01 16:15:06 tszilagyi Exp $
*/


/* Number of dynamics presets */
#define NUM_MODES 15


/* Dynamics presets data */
DYNAMICS_DATA dyn_data[NUM_MODES] = {

	{ /* 2:1 compression starting at -6 dB */
		4,
		{
			{-80.0f, -80.0f},
			{-6.0f, -6.0f},
			{0.0f, -3.8f},
			{20.0f, 3.5f},
		},
	},

	{ /* 2:1 compression starting at -9 dB */
		4,
		{
			{-80.0f, -80.0f},
			{-9.0f, -9.0f},
			{0.0f, -5.3f},
			{20.0f, 2.9f},
		},
	},

	{ /* 2:1 compression starting at -12 dB */
		4,
		{
			{-80.0f, -80.0f},
			{-12.0f, -12.0f},
			{0.0f, -6.8f},
			{20.0f, 1.9f},
		},
	},

	{ /* 2:1 compression starting at -18 dB */
		4,
		{
			{-80.0f, -80.0f},
			{-18.0f, -18.0f},
			{0.0f, -9.8f},
			{20.0f, -0.7f},
		},
	},

	{ /* 2.5:1 compression starting at -12 dB */
		4,
		{
			{-80.0f, -80.0f},
			{-12.0f, -12.0f},
			{0.0f, -7.5f},
			{20.0f, 0.0f},
		},
	},

	{ /* 3:1 compression starting at -12 dB */
		4,
		{
			{-80.0f, -80.0f},
			{-12.0f, -12.0f},
			{0.0f, -9.0f},
			{20.0f, -4.0f},
		},
	},

	{ /* 3:1 compression starting at -15 dB */
		4,
		{
			{-80.0f, -80.0f},
			{-15.0f, -15.0f},
			{0.0f, -10.8f},
			{20.0f, -5.2f},
		},
	},

	{ /* Compressor/Gate */
		5,
		{
			{-80.0f, -105.0f},
			{-62.0f, -80.0f},
			{-15.4f, -15.4f},
			{0.0f, -12.0f},
			{20.0f, -7.6f},
		},
	},

	{ /* Expander */
		8,
		{
			{-80.0f, -169.0f},
			{-54.0f, -80.0f},
			{-49.5f, -64.6f},
			{-41.1f, -41.1f},
			{-25.8f, -15.0f},
			{-10.8f, -4.5f},
			{0.0f, 0.0f},
			{20.0f, 8.3f},
		},
	},

	{ /* Hard limiter at -6 dB */
		3,
		{
			{-80.0f, -80.0f},
			{-6.0f, -6.0f},
			{20.0f, -6.0f},
		},
	},

	
	{ /* Hard limiter at -12 dB */
		3,
		{
			{-80.0f, -80.0f},
			{-12.0f, -12.0f},
			{20.0f, -12.0f},
		},
	},

	{ /* Hard noise gate at -35 dB */
		4,
		{
			{-80.0f, -115.0f},
			{-35.1f, -80.0f},
			{-35.0f, -35.0f},
			{20.0f, 20.0f},
		},
	},

	{ /* Soft limiter */
		5,
		{
			{-80.0f, -80.0f},
			{-12.4f, -12.4f},
			{-6.0f, -8.0f},
			{0.0f, -6.8f},
			{20.0f, -2.8f},
		},
	},

	{ /* Soft knee comp/gate (-24 dB threshold) */
		8,
		{
			{-80.0f, -113.7f},
			{-46.3f, -80.0f},
			{-42.0f, -56.8f},
			{-33.6f, -36.3f},
			{-24.0f, -24.0f},
			{-11.1f, -15.4f},
			{0.0f, -12.0f},
			{20.0f, -5.8f},
		},
	},

	{ /* Soft noise gate below -36 dB */
		7,
		{
			{-80.0f, -104.0f},
			{-56.0f, -80.0f},
			{-51.8f, -67.2f},
			{-44.7f, -49.3f},
			{-34.0f, -34.0f},
			{0.0f, 0.0f},
			{20.0f, 20.0f},
		},
	},

	
	
	
	
	
	/* You can add your own presets here.
	 * Please read the docs about the format.
	 */




};

