/*
    Copyright (C) 2002 Steve Harris <steve@plugin.org.uk>

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
*/

#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/mman.h>
#endif

#include "blo.h"

/* Create the lookup tables needed to generate the bandlimited signals, we
 * create sin tables (As well as the usual sin, square, saw), which is almost
 * completely pointless, but doesn't cost much ram.
 *
 * Memory use is roughly 8 * table_size * num_of_harmonics, eg. 2048 point
 * tables with 64 harmonics costs ~1Meg. The oscilators will be accurate down
 * to sample_rate / (2 * number_of_harmonics) Hz, below that they will sound a
 * bit soft.
 */

blo_h_tables *blo_h_tables_new(int table_size)
{
	blo_h_tables *this;
	float *all_tables = NULL;
	float *table;
	float table_size_f = table_size;
	float max;
	unsigned int table_count = 0;
	int i;
	unsigned int h;
	size_t all_tables_size = sizeof(float) * (table_size + BLO_TABLE_WR)
		                        * (BLO_N_HARMONICS - 1) * 2;
#ifndef WIN32
	int shm_fd;
#endif
	char shm_path[128];

	this = malloc(sizeof(blo_h_tables));
	this->alloc_size = all_tables_size;
	this->table_size = table_size;
	this->table_mask = table_size - 1;
	this->store_type = BLO_MMAP;

	snprintf(shm_path, 128, "/blo-1-%dx%dx%d.tbl", BLO_N_WAVES,
			BLO_N_HARMONICS, table_size + BLO_TABLE_WR);
#ifndef WIN32
	if ((shm_fd = shm_open(shm_path, O_RDONLY, 0)) > 0) {
		/* There is an existing SHM segment that matches what we want */

		all_tables = mmap(0, all_tables_size, PROT_READ, MAP_SHARED,
				shm_fd, 0);
		close(shm_fd);
		this->alloc_space = all_tables;

		/* Map the pointers to the correct places in SHM */

		/* The zero harmonics tables (trivial) */
		table = BLO_NEXT_TABLE;
		for (i=0; i<BLO_N_WAVES; i++) {
			this->h_tables[i][0] = table;
		}

		/* The 1st harmonic table (trivial) */
		table = BLO_NEXT_TABLE;
		for (i=0; i<BLO_N_WAVES; i++) {
			this->h_tables[i][1] = table;
		}

		/* The sin 2nd+ harmonics sine tables (trivial) */
		for (h=2; h<BLO_N_HARMONICS; h++) {
			this->h_tables[BLO_SINE][h] = table;
		}

		/* The tri 2nd+ harmonics tables */
		table = this->h_tables[BLO_TRI][1];
		for (h=2; h<BLO_N_HARMONICS; h++) {
			if (h % 2 == 0) {
				/* Even harmonic - add no partials */
				this->h_tables[BLO_TRI][h] = table;
			} else {
				/* Odd harmonic add sin(hp)/h^2 */
				table = BLO_NEXT_TABLE;
				this->h_tables[BLO_TRI][h] = table;
			}
		}

		/* The square 2nd+ harmonics tables */
		table = this->h_tables[BLO_SQUARE][1];
		for (h=2; h<BLO_N_HARMONICS; h++) {
			if (h % 2 == 0) {
				/* Even harmonic - add no partials */
				this->h_tables[BLO_SQUARE][h] = table;
			} else {
				/* Odd harmonic add sin(hp)/h */
				table = BLO_NEXT_TABLE;
				this->h_tables[BLO_SQUARE][h] = table;
			}
		}

		/* The saw 2nd+ harmonics tables */
		for (h=2; h<BLO_N_HARMONICS; h++) {
			/* All harmonics add sin(hp)/h */
			table = BLO_NEXT_TABLE;
			this->h_tables[BLO_SAW][h] = table;
		}

		return this;
	} else if ((shm_fd = shm_open(shm_path, O_CREAT | O_RDWR, 0644)) > 0) {
		/* There is no existing SHM segment, but we can make one */
		int err = ftruncate(shm_fd, all_tables_size);

		if (!err) {
			all_tables = mmap(0, all_tables_size, PROT_READ | PROT_WRITE,
					MAP_SHARED, shm_fd, 0);
		}
		close(shm_fd);
	}
#endif

	/* Fallback case, can't map a SHM segment, just malloc it and suffer */
	if (!all_tables) {
		all_tables = malloc(all_tables_size);
		this->store_type = BLO_MALLOC;
	}
	this->alloc_space = all_tables;

	/* Calculate the harmonic amplitudes and place the index pointers */

	/* Make a zero harmonics table (trivial) */
	table = BLO_NEXT_TABLE;
	for (i=0; i<table_size + BLO_TABLE_WR; i++) {
		table[i] = 0.0f;
	}
	for (i=0; i<BLO_N_WAVES; i++) {
		this->h_tables[i][0] = table;
	}

	/* Make a 1st harmonic table (trivial) */
	table = BLO_NEXT_TABLE;
	for (i=0; i<table_size + BLO_TABLE_WR; i++) {
		table[i] = BLO_SIN_GEN((float)i);
	}
	for (i=0; i<BLO_N_WAVES; i++) {
		this->h_tables[i][1] = table;
	}

	/* Make the sin 2nd+ harmonics tables (trivial) */
	for (h=2; h<BLO_N_HARMONICS; h++) {
		this->h_tables[BLO_SINE][h] = table;
	}

	/* Make the tri 2nd+ harmonics tables */
	table = this->h_tables[BLO_TRI][1];
	for (h=2; h<BLO_N_HARMONICS; h++) {
		if (h % 2 == 0) {
			/* Even harmonic - add no partials */
			this->h_tables[BLO_TRI][h] = table;
		} else {
			float sign = 1.0f;
			if (h % 4 == 3) {
				sign = -1.0f;
			}
			/* Odd harmonic add sin(hp)/h^2 */
			table = BLO_NEXT_TABLE;
			this->h_tables[BLO_TRI][h] = table;
			for (i=0; i<table_size + BLO_TABLE_WR; i++) {
				table[i] = this->h_tables[BLO_TRI][h - 1][i] +
				 sign * BLO_SIN_GEN((float)i * (float)h) /
				 ((float)h * (float) h);
			}
		}
	}

	/* Make the square 2nd+ harmonics tables */
	table = this->h_tables[BLO_SQUARE][1];
	for (h=2; h<BLO_N_HARMONICS; h++) {
		if (h % 2 == 0) {
			/* Even harmonic - add no partials */
			this->h_tables[BLO_SQUARE][h] = table;
		} else {
			/* Odd harmonic add sin(hp)/h */
			table = BLO_NEXT_TABLE;
			this->h_tables[BLO_SQUARE][h] = table;
			for (i=0; i<table_size + BLO_TABLE_WR; i++) {
				table[i] =
			       	 this->h_tables[BLO_SQUARE][h - 1][i] +
				 BLO_SIN_GEN((float)i * (float)h) / (float)h;
			}
		}
	}

	/* Make the saw 2nd+ harmonics tables */
	for (h=2; h<BLO_N_HARMONICS; h++) {
		/* All harmonics add sin(hp)/h */
		table = BLO_NEXT_TABLE;
		this->h_tables[BLO_SAW][h] = table;
		for (i=0; i<table_size + BLO_TABLE_WR; i++) {
			table[i] = this->h_tables[BLO_SAW][h - 1][i] +
			 BLO_SIN_GEN((float)i * (float)h) / (float)h;
		}
	}

	/* Normalise table levels */
	for (h=1; h<table_count; h++) {
		table = all_tables + (h * (table_size + BLO_TABLE_WR));
		max = 0.0f;
		for (i=0; i<table_size; i++) {
			if (fabs(table[i]) > max) {
				max = fabs(table[i]);
			}
		}
		max = 1.0f / max;
		for (i=0; i<table_size + BLO_TABLE_WR; i++) {
			table[i] *= max;
		}
	}

#ifndef WIN32
	msync(all_tables, all_tables_size, MS_ASYNC);
#endif

	return this;
}

void blo_h_tables_free(blo_h_tables *tables)
{
	if (tables->store_type == BLO_MMAP) {
#ifndef WIN32
		munmap(tables->alloc_space, tables->alloc_size);
#endif
	} else {
		free(tables->alloc_space);
	}
	free(tables);
}

blo_h_osc *blo_h_new(blo_h_tables *tables, unsigned int wave, float sample_rate)
{
	blo_h_osc *this = malloc(sizeof(blo_h_osc));

	this->tables = tables;
	this->wave = wave;
	this->sample_rate = sample_rate;
	this->nyquist = sample_rate * 0.49f;
	this->ph.all = 0;
	this->ph_coef = ((float)(tables->table_size) * 65536.0f) / sample_rate;
	this->ph_mask = tables->table_size * 65536 - 1;
	this->table_mask = tables->table_mask;
	this->table_size = tables->table_size;
	this->table = tables->h_tables[0][0];
	this->table_b = tables->h_tables[0][0];

	return this;
}

void blo_h_free(blo_h_osc *osc)
{
	free(osc);
}
