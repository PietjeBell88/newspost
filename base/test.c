/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Author: Jim Faulkner <newspost@sdf.lonestar.org>
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
 */
#include "newspost.h"

int main(int argc, char ** argv){
	boolean error = FALSE;

	if(sizeof(n_uint8) != 1){
		error = TRUE;
		fprintf(stderr, "\nYour unsigned chars are not 1 byte.");
	}
	if(sizeof(n_uint16) != 2){
		error = TRUE;
		fprintf(stderr, "\nYour unsigned short ints are not 2 bytes.");
	}
	if(sizeof(n_uint32) != 4){
		error = TRUE;
		fprintf(stderr, "\nYour unsigned ints are not 4 bytes.");
	}
	if(sizeof(n_int64) != 8){
		error = TRUE;
		fprintf(stderr, "\nYour long longs are not 8 bytes.");
	}
	if(error == TRUE){
		fprintf(stderr, "\nPlease edit base/newspost.h");
                fprintf(stderr, "\nThen run \"make clean\" and \"make\" again");
                fprintf(stderr, "\n\n");
		return -1;
	}
	else{
		return 0;
	}
}

