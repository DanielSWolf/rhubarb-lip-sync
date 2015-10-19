/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2015 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */

/*
 * bitarr.c -- Bit array manipulations implementation.
 */

#ifdef __APPLE__
#include <architecture/byte_order.h>
#elif __linux__
#include <endian.h>
#elif !defined(_WIN32) && !defined(_WIN64)
#include <arpa/nameser_compat.h>
#endif 

#include "sphinxbase/bitarr.h"

/**
 * Union to map float to integer to store latter in bit array
 */
typedef union { 
    float f; 
    uint32 i; 
} float_enc;

#define SIGN_BIT (0x80000000)

/**
 * Shift bits depending on byte order in system.
 * Fun fact: __BYTE_ORDER is wrong on Solaris Sparc, but the version without __ is correct.
 * @param bit is an offset last byte
 * @param length - amount of bits for required for digit that is going to be read
 * @return shift forgiven architecture
 */
static uint8 get_shift(uint8 bit, uint8 length)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    return bit;
#elif BYTE_ORDER == BIG_ENDIAN
    return 64 - length - bit;
#else
#error "Bit packing code isn't written for your byte order."
#endif
}

/**
 * Read uint64 value from the given address
 * @param address to read from
 * @param pointer to value where to save read value
 * @return uint64 value that was read
 */
static uint64 read_off(bitarr_address_t address)
{
#if defined(__arm) || defined(__arm__)
    uint64 value64;
    const uint8 *base_off = (const uint8 *)(address.base) + (address.offset >> 3);
    memcpy(&value64, base_off, sizeof(value64));
    return value64;
#else
    return *(const uint64*)((const uint8 *)(address.base) + (address.offset >> 3));
#endif
}

uint64 bitarr_read_int57(bitarr_address_t address, uint8 length, uint64 mask)
{
    return (read_off(address) >> get_shift(address.offset & 7, length)) & mask;
}

void bitarr_write_int57(bitarr_address_t address, uint8 length, uint64 value) 
{
#if defined(__arm) || defined(__arm__)
    uint64 value64;
    uint8 *base_off = (uint8 *)(address.base) + (address.offset >> 3);
    memcpy(&value64, base_off, sizeof(value64));
    value64 |= (value << get_shift(address.offset & 7, length));
    memcpy(base_off, &value64, sizeof(value64));
#else
    *(uint64 *)((uint8 *)(address.base) + (address.offset >> 3)) |= (value << get_shift(address.offset & 7, length));
#endif
}

uint32 bitarr_read_int25(bitarr_address_t address, uint8 length, uint32 mask) 
{
#if defined(__arm) || defined(__arm__)
    uint32 value32;
    const uint8 *base_off = (const uint8_t*)(address.base) + (address.offset >> 3);
    memcpy(&value32, base_off, sizeof(value32));
    return (value32 >> get_shift(address.offset & 7, length)) & mask;
#else
    return (*(const uint32_t*)((const uint8_t*)(address.base) + (address.offset >> 3)) >> get_shift(address.offset & 7, length)) & mask;
#endif
}

void bitarr_write_int25(bitarr_address_t address, uint8 length, uint32 value)
{
#if defined(__arm) || defined(__arm__)
    uint32 value32;
    uint8 *base_off = (uint8 *)(address.base) + (address.offset >> 3);
    memcpy(&value32, base_off, sizeof(value32));
    value32 |= (value << get_shift(address.offset & 7, length));
    memcpy(base_off, &value32, sizeof(value32));
#else
    *(uint32_t *)((uint8 *)(address.base) + (address.offset >> 3)) |= (value << get_shift(address.offset & 7, length));
#endif
}

float bitarr_read_negfloat(bitarr_address_t address) 
{
    float_enc encoded;
    encoded.i = (uint32)(read_off(address) >> get_shift(address.offset & 7, 31));
    // Sign bit set means negative.  
    encoded.i |= SIGN_BIT;
    return encoded.f;
}

void bitarr_write_negfloat(bitarr_address_t address, float value) 
{
    float_enc encoded;
    encoded.f = value;
    encoded.i &= ~SIGN_BIT;
    bitarr_write_int57(address, 31, encoded.i);
}

float bitarr_read_float(bitarr_address_t address)
{
    float_enc encoded;
    encoded.i = (uint32)(read_off(address) >> get_shift(address.offset & 7, 32));
    return encoded.f;
}

void bitarr_write_float(bitarr_address_t address, float value) 
{
    float_enc encoded;
    encoded.f = value;
    bitarr_write_int57(address, 32, encoded.i);
}

void bitarr_mask_from_max(bitarr_mask_t *bit_mask, uint32 max_value)
{
    bit_mask->bits = bitarr_required_bits(max_value);
    bit_mask->mask = (uint32)((1ULL << bit_mask->bits) - 1);
}

uint8 bitarr_required_bits(uint32 max_value)
{
    uint8 res;

    if (!max_value) return 0;
    res = 1;
    while (max_value >>= 1) res++;
    return res;
}