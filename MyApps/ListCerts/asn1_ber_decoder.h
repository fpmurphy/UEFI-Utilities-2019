/* ASN.1 decoder
 *
 * Copyright (C) 2012 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

/*
 *  Copyright (c) 2012-2019 Finnbarr P. Murphy.   All rights reserved.
 *
 *  Modified to work in EDKII environment.
 *
 */


#ifndef _ASN1_DECODER_H
#define _ASN1_DECODER_H

/* FPM  - hack to handle any size_t issues */
typedef long  size_t __attribute__((aligned (8)));
#define unlikely(x) (x)


#include "asn1.h"

struct asn1_decoder;

extern int 
asn1_ber_decoder( const struct asn1_decoder *decoder,
		  void *context,
		  const unsigned char *data,
		  size_t datalen );

#endif /* _ASN1_DECODER_H */
