/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __ENCODE_H__
#define __ENCODE_H__

#include "newspost.h"

#define BYTES_PER_LINE 45
#define UU_CHARACTERS_PER_LINE 64

/* fills fillme with encoded data, and returns how many characters it wrote */
long get_encoded_part(newspost_data *data, file_entry *file, 
		      int partnumber, char *fillme);

/* returns the buffer size that should used for the encoded data */
long get_buffer_size_per_encoded_part(newspost_data *data);

/* returns the total number of parts it will take to encode */
int get_number_of_encoded_parts(newspost_data *data, file_entry *file);

#endif /* __ENCODE_H__ */
