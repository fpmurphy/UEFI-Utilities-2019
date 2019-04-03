/* ASN.1 Object identifier (OID) registry
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

#include <errno.h>

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>

#include "oid_registry.h"
#include "oid_registry_data.h"  

/*
 * Find an OID registration for the specified data
 * @data: Binary representation of the OID
 * @datasize: Size of the binary representation
 */
enum OID 
Lookup_OID(const void *data, long datasize)
{
    const unsigned char *octets = data;
    enum OID oid;
    unsigned char xhash;
    unsigned i, j, k, hash;
    long len;

    /* Hash the OID data */
    hash = datasize - 1;
    for (i = 0; i < datasize; i++)
        hash += octets[i] * 33;
    hash = (hash >> 24) ^ (hash >> 16) ^ (hash >> 8) ^ hash;
    hash &= 0xff;

    /* Binary search the OID registry.  OIDs are stored in ascending order
     * of hash value then ascending order of size and then in ascending
     * order of reverse value.
     */
    i = 0;
    k = OID__NR;
    while (i < k) {
        j = (i + k) / 2;

        xhash = oid_search_table[j].hash;
        if (xhash > hash) {
            k = j;
            continue;
        }
        if (xhash < hash) {
            i = j + 1;
            continue;
        }

        oid = oid_search_table[j].oid;
        len = oid_index[oid + 1] - oid_index[oid];
        if (len > datasize) {
            k = j;
            continue;
        }
        if (len < datasize) {
            i = j + 1;
            continue;
        }

        /* Variation is most likely to be at the tail end of the
         * OID, so do the comparison in reverse.
         */
        while (len > 0) {
            unsigned char a = oid_data[oid_index[oid] + --len];
            unsigned char b = octets[len];
            if (a > b) {
                k = j;
                goto next;
            }
            if (a < b) {
                i = j + 1;
                goto next;
            }
        }
        return oid;
    next:
        ;
    }

    return OID__NR;
}


/*
 * Print an Object Identifier into a buffer
 * @data: The encoded OID to print
 * @datasize: The size of the encoded OID
 * @buffer: The buffer to render into
 * @bufsize: The size of the buffer
 *
 * The OID is rendered into the buffer in "a.b.c.d" format and the number of
 * bytes is returned.  -EBADMSG is returned if the data could not be intepreted
 * and -ENOBUFS if the buffer was too small.
 */
int 
Sprint_OID(const void *data, long datasize, CHAR16 *buffer, long bufsize)
{
    const unsigned char *v = data, *end = v + datasize;
    UINT64 num;
    UINT8 n;
    long ret;
    int count;

    if (v >= end)
        return -EBADMSG;

    n = (UINT8)*v++;
    UnicodeSPrint(buffer, (UINTN)bufsize, (CHAR16 *)L"%d.%d", n / 40, n % 40);
    ret = count = StrLen(buffer);
    buffer += count;
    bufsize -= count;
    if (bufsize == 0)
        return -ENOBUFS;

    while (v < end) {
        num = 0;
        n = (UINT8)*v++;
        if (!(n & 0x80)) {
            num = n;
        } else {
            num = n & 0x7f;
            do {
                if (v >= end)
                    return -EBADMSG;
                n = (UINT8)*v++;
                num <<= 7;
                num |= n & 0x7f;
            } while (n & 0x80);
        }
        UnicodeSPrint(buffer, (UINTN)bufsize, (CHAR16 *)L".%ld", num);
        ret += count = StrLen(buffer);
        buffer += count;
        bufsize -= count;
        if (bufsize == 0)
            return -ENOBUFS;
    }

    return ret;
}
