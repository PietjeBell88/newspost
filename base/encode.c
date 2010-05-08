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

/* Modified and yencoding added by William McBrine <wmcbrine@users.sf.net> */

#include "encode.h"
#include "../enc/uuencode.h"
#include "../enc/yencode.h"

/**
*** Public Routines
**/

/* returns the total number of parts it will take to encode */
int get_number_of_encoded_parts(newspost_data *data, file_entry *file) {

	long blocksize = BYTES_PER_LINE * data->lines;
	return ( (file->fileinfo.st_size / blocksize) +
		((file->fileinfo.st_size % blocksize) != 0) );
}

/* returns the buffer size that should be used for encoded data */
long get_buffer_size_per_encoded_part(newspost_data *data) {

	if (data->uuenc == FALSE) {
		/* Worst-case for yenc is twice the original size, plus
		 * the line endings; though realistically it will be only
		 * a few percent bigger than the original.
		 * Add 512 to cover ybegin, ypart and yend lines. */

		long blocksize = BYTES_PER_LINE * data->lines;
		return ((blocksize * 2) +
			((blocksize / YENC_LINE_LENGTH) * 4) + 512);
	}
	else {
		/* add the 512 just in case of an extremely 
		 * long file name in the begin line */
		return ((data->lines * UU_CHARACTERS_PER_LINE) + 512);
	}
}

/* fills fillme with encoded data, and returns how many chars it wrote */
/* fillme should be of size get_buffer_size_per_encoded_part(ed) bytes */
long get_encoded_part (newspost_data *data, file_entry *file, 
		  int partnumber, char *fillme) {
	FILE *fp;
	long message_size;

	char *pi = fillme;	/* pointer iterator */
	int total_parts = get_number_of_encoded_parts(data, file);

	if (!( (partnumber > 0) && (partnumber <= total_parts) ))
		return -1;
 
	fp = fopen(file->filename->data, "rb");
	message_size = BYTES_PER_LINE * data->lines;

	/* seek to the appropriate place if we're not already there */
	if (partnumber != 1)
		fseek(fp, (message_size * (partnumber - 1)), SEEK_SET);

	/* and encode */
	if (data->uuenc == FALSE) {
		long pbegin, pend, psize;
		n_uint32 crc = 0;

		pbegin = message_size * (partnumber - 1) + 1;
		pend = pbegin + message_size - 1;
		if (pend > file->fileinfo.st_size)
			pend = file->fileinfo.st_size;
		psize = pend - pbegin + 1;

		/* The first line (or two) */

		if (total_parts == 1)
			pi += sprintf(pi, "=ybegin line=%i size=%li "
				"name=%s\r\n", YENC_LINE_LENGTH,
				(long) file->fileinfo.st_size,
				n_basename(file->filename->data));
		else {
			pi += sprintf(pi, "=ybegin part=%i total=%i line=%i "
				"size=%li name=%s\r\n", partnumber,
				total_parts, YENC_LINE_LENGTH,
				(long) file->fileinfo.st_size,
				n_basename(file->filename->data));

			pi += sprintf(pi, "=ypart begin=%li end=%li\r\n",
				pbegin, pend);
		}

		/* The core */
		pi += yencode(fp, pi, psize, &crc);

		/* The last line */
		if (total_parts == 1)
			pi += sprintf(pi, "=yend size=%li crc32=%08x\r\n",
				psize, crc);
		else {
			pi += sprintf(pi, "=yend size=%li part=%i pcrc32=%08x",
				psize, partnumber, crc);

			if (partnumber == total_parts)
				pi += sprintf(pi, " crc32=%08x\r\n",
					file->crc);
			else
				pi += sprintf(pi, "\r\n");
		}
	}
	else {
		/* make sure the appropriate text is put at the beginning */
		if (partnumber == 1)
			pi += sprintf(pi, "begin 644 %s\r\n",
				n_basename(file->filename->data));

		pi += uu_encode(fp, pi, data->lines);
		
		/* make sure the appropriate text is put at the end */
		if (partnumber == total_parts) 
			pi += sprintf(pi, "end\r\n");
	}

	fclose(fp);
	return pi - fillme;
}
