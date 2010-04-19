/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* md5.h - Declaration of functions and data types used for MD5 sum
   computing library functions.
   Copyright (C) 1995, 1996, 1999 Free Software Foundation, Inc.
   NOTE: The canonical source of this file is maintained with the GNU C
   Library.  Bugs can be reported to bug-glibc@prep.ai.mit.edu.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _MD5_H
#define _MD5_H 1

#include <stdio.h>

#include "types.h"

/* Compute MD5 message digest for bytes read from STREAM.  The
   resulting message digest number will be written into the 16 bytes
   beginning at RESBLOCK.  */
extern i64 md5_stream (FILE *stream, void *resblock);

/* Compute MD5 message digest for LEN bytes beginning at BUFFER.  The
   result is always in little endian byte order, so that a byte-wise
   output yields to the wanted ASCII representation of the message
   digest.  */
extern void *md5_buffer (const char *buffer, size_t len, void *resblock);

#endif
