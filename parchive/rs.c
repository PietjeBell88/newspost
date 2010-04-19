/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001  Willem Monsuwe (willem@stack.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 *
 * Modified for Newspost by Jim Faulkner and William McBrine
 */

/* Reed-Solomon coding */

#include <stdio.h>
#include <string.h>
#include "types.h"
#include "fileops.h"
#include "rs.h"
#include "util.h"
#include "par.h"

/*
 Calculations over a Galois Field, GF(8)
*/

u8 gl[0x100], ge[0x200];

/**
*** Private Declarations and Routines
**/

/* Multiply: a*b = exp(log(a) + log(b)) */
static int
gmul(int a, int b)
{
	if ((a == 0) || (b == 0)) return 0;
	return ge[gl[a] + gl[b]];
}

/* Divide: a/b = exp(log(a) - log(b)) */
static int
gdiv(int a, int b)
{
	if ((a == 0) || (b == 0)) return 0;
	return ge[gl[a] - gl[b] + 255];
}

/* Power: a^b = exp(log(a) * b) */
static int
gpow(int a, int b)
{
	if (a == 0) return 0;
	return ge[(gl[a] * b) % 255];
}

/* Initialise log and exp tables using generator x^8 + x^4 + x^3 + x^2 + 1 */
static void
ginit(void)
{
	unsigned int b, l;

	b = 1;
	for (l = 0; l < 0xff; l++) {
		gl[b] = l;
		ge[l] = b;
		b += b;
		if (b & 0x100) b ^= 0x11d;
	}
	for (l = 0xff; l < 0x200; l++)
		ge[l] = ge[l - 0xff];
}

/* Fill in a LUT */
static void
make_lut(u8 lut[0x100], int m)
{
	int j;
	for (j = 0x100; --j; )
		lut[j] = ge[gl[m] + gl[j]];
	lut[0] = 0;
}

#define MT(i,j)     (mt[((i) * Q) + (j)])
#define IMT(i,j)   (imt[((i) * N) + (j)])
#define MULS(i,j) (muls[((i) * N) + (j)])

/**
*** Public Routines
**/

int
recreate(xfile_t *in, xfile_t *out)
{
	int i, j, k, l, M, N, Q, R;
	u8 *mt, *imt, *muls;
	u8 buf[0x10000], *work;
	i64 s, size;
	/* i64 perc; */

	ginit();

	/* Count number of recovery files */
	for (i = Q = R = 0; in[i].filenr; i++) {
		if (in[i].files) {
			R++;
			/* Get max. matrix row size */
			for (k = 0; in[i].files[k]; k++) {
				if (in[i].files[k] > Q)
					Q = in[i].files[k];
			}
		} else {
			if (in[i].filenr > Q)
				Q = in[i].filenr;
		}
	}
	N = i;

	/* Count number of volumes to output */
	for (i = j = M = 0; out[i].filenr; i++) {
		M++;
		if (out[i].files) {
			j++;
			/* Get max. matrix row size */
			for (k = 0; out[i].files[k]; k++) {
				if (out[i].files[k] > Q)
					Q = out[i].files[k];
			}
		} else {
			if (out[i].filenr > Q)
				Q = out[i].filenr;
		}
	}
	R += j;
	Q += j;

	CNEW(mt, R * Q);
	CNEW(imt, R * N);

	/* Fill in matrix rows for recovery files */
	for (i = j = 0; in[i].filenr; i++) {
		if (!in[i].files)
			continue;
		for (k = 0; in[i].files[k]; k++)
			MT(j, in[i].files[k]-1) = gpow(k+1, in[i].filenr - 1);
		IMT(j, i) = 1;
		j++;
	}

	/* Fill in matrix rows for output recovery files */
	for (i = 0, l = Q; out[i].filenr; i++) {
		if (!out[i].files)
			continue;
		for (k = 0; out[i].files[k]; k++)
			MT(j, out[i].files[k]-1) = gpow(k+1, out[i].filenr - 1);
		--l;
		/* Fudge filenr */
		out[i].filenr = l + 1;
		MT(j, l) = 1;
		j++;
	}
/*
	if (cmd.loglevel > 0) {
		fprintf(stderr, "Matrix input:\n");
		for (i = 0; i < R; i++) {
			fprintf(stderr, "| ");
			for (j = 0; j < Q; j++)
				fprintf(stderr, "%02x ", MT(i, j));
			fprintf(stderr, "| ");
			for (j = 0; j < N; j++)
				fprintf(stderr, "%02x ", IMT(i, j));
			fprintf(stderr, "|\n");
		}
	}
*/
	/* Use (virtual) rows from data files to eliminate columns */
	for (i = 0; in[i].filenr; i++) {
		if (in[i].files)
			continue;
		k = in[i].filenr - 1;
		/* MT would have a 1 at (i, k), IMT a 1 at (i, i)
		   IMT(j,i) -= MT(j,k) * IMT(i,i)  (is MT(j, k))
		   MT(j,k) -= MT(j,k) *  MT(i,k)  (becomes 0)
		*/
		for (j = 0; j < R; j++) {
			IMT(j, i) ^= MT(j, k);
			MT(j, k) = 0;
		}
	}
/*
	if (cmd.loglevel > 0) {
		fprintf(stderr, "Matrix after data file elimination:\n");
		for (i = 0; i < R; i++) {
			fprintf(stderr, "| ");
			for (j = 0; j < Q; j++)
				fprintf(stderr, "%02x ", MT(i, j));
			fprintf(stderr, "| ");
			for (j = 0; j < N; j++)
				fprintf(stderr, "%02x ", IMT(i, j));
			fprintf(stderr, "|\n");
		}
	}
*/
	/* Eliminate columns using the remaining rows, so we get I.
	   The accompanying matrix will be the inverse
	*/
	for (i = 0; i < R; i++) {
		int d, l;
		/* Find first non-zero entry */
		for (l = 0; (l < Q) && !MT(i, l); l++)
			;
		if (l == Q) continue;
		d = MT(i, l);
		/* Scale the matrix so MT(i, l) becomes 1 */
		for (j = 0; j < Q; j++)
			MT(i, j) = gdiv(MT(i, j), d);
		for (j = 0; j < N; j++)
			IMT(i, j) = gdiv(IMT(i, j), d);
		/* Eliminate the column in the other matrices */
		for (k = 0; k < R; k++) {
			if (k == i) continue;
			d = MT(k, l);
			for (j = 0; j < Q; j++)
				MT(k, j) ^= gmul(MT(i, j), d);
			for (j = 0; j < N; j++)
				IMT(k, j) ^= gmul(IMT(i, j), d);
		}
	}
/*
	if (cmd.loglevel > 0) {
		fprintf(stderr, "Matrix after gaussian elimination:\n");
		for (i = 0; i < R; i++) {
			fprintf(stderr, "| ");
			for (j = 0; j < Q; j++)
				fprintf(stderr, "%02x ", MT(i, j));
			fprintf(stderr, "| ");
			for (j = 0; j < N; j++)
				fprintf(stderr, "%02x ", IMT(i, j));
			fprintf(stderr, "|\n");
		}
	}
*/
	/* Make the multiplication tables */
	CNEW(muls, M * N);

	for (i = 0; out[i].filenr; i++) {
		/* File #x: The row IMT(j) for which MT(j,x) = 1 */
		for (j = 0; j < R; j++) {
			k = out[i].filenr - 1;
			if (MT(j, k) != 1)
				continue;
			/* All other values should be 0 */
			for (k = 0; !MT(j, k); k++)
				;
			if (k != out[i].filenr - 1)
				continue;
			for (k++; (k < Q) && !MT(j, k); k++)
				;
			if (k != Q)
				continue;
			break;
		}
		/* Did we find a suitable row ? */
		if (j == R) {
			out[i].size = 0;
			continue;
		}
		for (k = 0; k < N; k++)
			MULS(i, k) = IMT(j, k);
	}
	free(mt);
	free(imt);
/*
	if (cmd.loglevel > 0) {
		fprintf(stderr, "Multipliers:\n");
		for (i = 0; i < M; i++) {
			fprintf(stderr, "| ");
			for (j = 0; j < N; j++)
				fprintf(stderr, "%02x ", MULS(i, j));
			fprintf(stderr, "|\n");
		}
	}
*/

	/* Check for columns with all-zeroes */
	for (j = 0; j < N; j++) {
		for (i = 0; i < M; i++)
			if (MULS(i, j))
				break;
		/* This input file isn't used */
		if (i == M)
			in[j].size = 0;
	}

	/* Find out how much we should process in total */
	size = 0;
	for (i = 0; out[i].filenr; i++)
		if (size < out[i].size)
			size = out[i].size;

	/* Restore all the files at once */
	NEW(work, sizeof(buf) * M);

	/* perc = 0; */
	/* fprintf(stderr, "0%%"); fflush(stderr); */
	/* Process all files */
	for (s = 0; s < size; ) {
		i64 tr, r, q;
		u8 *p;

		/* Display progress 
		while (((s * 50) / size) > perc) {
			perc++;
			if (perc % 5) fprintf(stderr, ".");
			else fprintf(stderr, "%lld%%", (perc / 5) * 10);
			fflush(stderr);
		}
		*/

		/* See how much we should read */
		memset(work, 0, sizeof(buf) * M);
		for (i = 0; in[i].filenr; i++) {
			tr = sizeof(buf);
			if (tr > (in[i].size - s))
				tr = in[i].size - s;
			if (tr <= 0)
				continue;
			r = file_read(in[i].f, buf, tr);
			if (r < tr) {
/*
				perror("READ ERROR");
*/
				free(muls);
 				free(work);
				return 0;
			}
			for (j = 0; out[j].filenr; j++) {
				u8 lut[0x100];
				if (s >= out[j].size) continue;
				if (!MULS(j, i)) continue;
				/* Precalc LUT */
				make_lut(lut, MULS(j, i));
				p = work + (j * sizeof(buf));
				/* XOR it in, passed through the LUTs */
				for (q = r; --q >= 0; )
					p[q] ^= lut[buf[q]];
			}
		}
		for (j = 0; out[j].filenr; j++) {
			if (s >= out[j].size) continue;
			tr = sizeof(buf);
			if (tr > (out[j].size - s))
				tr = out[j].size - s;
			r = file_write(out[j].f, work + (j * sizeof(buf)), tr);
			if (r < tr) {
/*
				perror("WRITE ERROR");
*/
				free(muls);
				free(work);
				return 0;
			}
		}
		s += sizeof(buf);
	}
/*
	fprintf(stderr, "100%%\n"); fflush(stderr);
*/
	free(muls);
	free(work);
	return 1;
}
