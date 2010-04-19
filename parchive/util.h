/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef UTIL_H
#define UTIL_H

#define HEXDIGIT(i) (((i) + (((i) < 0xA) ? '0' : ('a' - 0xA))))

#define NEW(ptr, size) ((ptr) = (malloc(sizeof(*(ptr)) * (size))))
#define CNEW(ptr, size) ((ptr) = (calloc(sizeof(*(ptr)), (size))))
#define RENEW(ptr, size) ((ptr) = (realloc((ptr), sizeof(*(ptr)) * (size))))
#define COPY(tgt, src, nel) (memcpy((tgt), (src), ((nel) * sizeof(*(tgt)))))

#endif /* UTIL_H */
