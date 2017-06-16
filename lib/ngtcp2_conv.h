/*
 * ngtcp2
 *
 * Copyright (c) 2017 ngtcp2 contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef NGTCP2_CONV_H
#define NGTCP2_CONV_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <ngtcp2/ngtcp2.h>

/*
 * ngtcp2_get_uint64 reads 8 bytes from |p| as 64 bits unsigned
 * integer encoded as network byte order, and returns it in host byte
 * order.
 */
uint64_t ngtcp2_get_uint64(const uint8_t *p);

/*
 * ngtcp2_get_uint32 reads 4 bytes from |p| as 32 bits unsigned
 * integer encoded as network byte order, and returns it in host byte
 * order.
 */
uint32_t ngtcp2_get_uint32(const uint8_t *p);

/*
 * ngtcp2_get_uint16 reads 2 bytes from |p| as 16 bits unsigned
 * integer encoded as network byte order, and returns it in host byte
 * order.
 */
uint16_t ngtcp2_get_uint16(const uint8_t *p);

#endif /* NGTCP2_CONV_H */
