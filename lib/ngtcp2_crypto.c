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
#include "ngtcp2_crypto.h"

#include <string.h>
#include <assert.h>

#include "ngtcp2_str.h"
#include "ngtcp2_conv.h"

int ngtcp2_crypto_km_new(ngtcp2_crypto_km **pckm, const uint8_t *key,
                         size_t keylen, const uint8_t *iv, size_t ivlen,
                         ngtcp2_mem *mem) {
  size_t len;
  uint8_t *p;

  len = sizeof(ngtcp2_crypto_km) + keylen + ivlen;

  *pckm = ngtcp2_mem_malloc(mem, len);
  if (*pckm == NULL) {
    return NGTCP2_ERR_NOMEM;
  }

  p = (uint8_t *)(*pckm) + sizeof(ngtcp2_crypto_km);
  (*pckm)->key = p;
  (*pckm)->keylen = keylen;
  p = ngtcp2_cpymem(p, key, keylen);
  (*pckm)->iv = p;
  (*pckm)->ivlen = ivlen;
  /*p = */ ngtcp2_cpymem(p, iv, ivlen);

  return 0;
}

void ngtcp2_crypto_km_del(ngtcp2_crypto_km *ckm, ngtcp2_mem *mem) {
  if (ckm == NULL) {
    return;
  }

  ngtcp2_mem_free(mem, ckm);
}

void ngtcp2_crypto_create_nonce(uint8_t *dest, const uint8_t *iv, size_t ivlen,
                                uint64_t pkt_num) {
  size_t i;

  memcpy(dest, iv, ivlen);
  pkt_num = bswap64(pkt_num);

  for (i = 0; i < 8; ++i) {
    dest[ivlen - 8 + i] ^= ((uint8_t *)&pkt_num)[i];
  }
}

ssize_t ngtcp2_encode_transport_params(uint8_t *dest, size_t destlen,
                                       uint8_t exttype,
                                       const ngtcp2_transport_params *params) {
  uint8_t *p;
  size_t len = 2 /* transport parameters length */ +
               8 /* initial_max_stream_data */ + 8 /* initial_max_data */
               + 6 /* idle_timeout */ + 6 /* initial_max_streams_bidi */ +
               6 /* initial_max_streams_uni */;
  size_t i;
  size_t vlen;

  switch (exttype) {
  case NGTCP2_TRANSPORT_PARAMS_TYPE_CLIENT_HELLO:
    vlen = sizeof(uint32_t);
    break;
  case NGTCP2_TRANSPORT_PARAMS_TYPE_ENCRYPTED_EXTENSIONS:
    vlen = sizeof(uint32_t) + 1 + params->v.ee.len * sizeof(uint32_t);
    if (params->stateless_reset_token_present) {
      len += 20;
    }
    break;
  default:
    return NGTCP2_ERR_INVALID_ARGUMENT;
  }

  len += vlen;

  if (params->max_packet_size != NGTCP2_MAX_PKT_SIZE) {
    len += 6;
  }
  if (params->ack_delay_exponent != NGTCP2_DEFAULT_ACK_DELAY_EXPONENT) {
    len += 5;
  }

  if (destlen < len) {
    return NGTCP2_ERR_NOBUF;
  }

  p = dest;

  switch (exttype) {
  case NGTCP2_TRANSPORT_PARAMS_TYPE_CLIENT_HELLO:
    p = ngtcp2_put_uint32be(p, params->v.ch.initial_version);
    break;
  case NGTCP2_TRANSPORT_PARAMS_TYPE_ENCRYPTED_EXTENSIONS:
    p = ngtcp2_put_uint32be(p, params->v.ee.negotiated_version);
    *p++ = (uint8_t)(params->v.ee.len * sizeof(uint32_t));
    for (i = 0; i < params->v.ee.len; ++i) {
      p = ngtcp2_put_uint32be(p, params->v.ee.supported_versions[i]);
    }
    break;
  }

  p = ngtcp2_put_uint16be(p, (uint16_t)(len - vlen - sizeof(uint16_t)));

  p = ngtcp2_put_uint16be(p, NGTCP2_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA);
  p = ngtcp2_put_uint16be(p, 4);
  p = ngtcp2_put_uint32be(p, params->initial_max_stream_data);

  p = ngtcp2_put_uint16be(p, NGTCP2_TRANSPORT_PARAM_INITIAL_MAX_DATA);
  p = ngtcp2_put_uint16be(p, 4);
  p = ngtcp2_put_uint32be(p, params->initial_max_data);

  p = ngtcp2_put_uint16be(p, NGTCP2_TRANSPORT_PARAM_IDLE_TIMEOUT);
  p = ngtcp2_put_uint16be(p, 2);
  p = ngtcp2_put_uint16be(p, params->idle_timeout);

  p = ngtcp2_put_uint16be(p, NGTCP2_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_BIDI);
  p = ngtcp2_put_uint16be(p, 2);
  p = ngtcp2_put_uint16be(p, params->initial_max_streams_bidi);

  p = ngtcp2_put_uint16be(p, NGTCP2_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_UNI);
  p = ngtcp2_put_uint16be(p, 2);
  p = ngtcp2_put_uint16be(p, params->initial_max_streams_uni);

  if (exttype == NGTCP2_TRANSPORT_PARAMS_TYPE_ENCRYPTED_EXTENSIONS &&
      params->stateless_reset_token_present) {
    p = ngtcp2_put_uint16be(p, NGTCP2_TRANSPORT_PARAM_STATELESS_RESET_TOKEN);
    p = ngtcp2_put_uint16be(p, sizeof(params->stateless_reset_token));
    p = ngtcp2_cpymem(p, params->stateless_reset_token,
                      sizeof(params->stateless_reset_token));
  }

  if (params->max_packet_size != NGTCP2_MAX_PKT_SIZE) {
    p = ngtcp2_put_uint16be(p, NGTCP2_TRANSPORT_PARAM_MAX_PACKET_SIZE);
    p = ngtcp2_put_uint16be(p, 2);
    p = ngtcp2_put_uint16be(p, params->max_packet_size);
  }

  if (params->ack_delay_exponent != NGTCP2_DEFAULT_ACK_DELAY_EXPONENT) {
    p = ngtcp2_put_uint16be(p, NGTCP2_TRANSPORT_PARAM_ACK_DELAY_EXPONENT);
    p = ngtcp2_put_uint16be(p, 1);
    *p++ = params->ack_delay_exponent;
  }

  assert((size_t)(p - dest) == len);

  return (ssize_t)len;
}

int ngtcp2_decode_transport_params(ngtcp2_transport_params *params,
                                   uint8_t exttype, const uint8_t *data,
                                   size_t datalen) {
  uint32_t flags = 0;
  const uint8_t *p, *end;
  size_t supported_versionslen;
  size_t i;
  uint16_t param_type;
  size_t valuelen;
  size_t vlen;

  p = data;
  end = data + datalen;

  switch (exttype) {
  case NGTCP2_TRANSPORT_PARAMS_TYPE_CLIENT_HELLO:
    if ((size_t)(end - p) < sizeof(uint32_t)) {
      return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
    }
    params->v.ch.initial_version = ngtcp2_get_uint32(p);
    p += sizeof(uint32_t);
    vlen = sizeof(uint32_t);
    break;
  case NGTCP2_TRANSPORT_PARAMS_TYPE_ENCRYPTED_EXTENSIONS:
    if ((size_t)(end - p) < sizeof(uint32_t) + 1) {
      return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
    }
    params->v.ee.negotiated_version = ngtcp2_get_uint32(p);
    p += sizeof(uint32_t);
    supported_versionslen = *p++;
    if ((size_t)(end - p) < supported_versionslen ||
        supported_versionslen % sizeof(uint32_t)) {
      return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
    }
    params->v.ee.len = supported_versionslen / sizeof(uint32_t);
    for (i = 0; i < supported_versionslen;
         i += sizeof(uint32_t), p += sizeof(uint32_t)) {
      params->v.ee.supported_versions[i / sizeof(uint32_t)] =
          ngtcp2_get_uint32(p);
    }
    vlen = sizeof(uint32_t) + 1 + supported_versionslen;
    break;
  default:
    return NGTCP2_ERR_INVALID_ARGUMENT;
  }

  if ((size_t)(end - p) < sizeof(uint16_t)) {
    return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
  }

  if (vlen + sizeof(uint16_t) + ngtcp2_get_uint16(p) != datalen) {
    return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
  }
  p += sizeof(uint16_t);

  /* Set default values */
  params->initial_max_streams_bidi = 0;
  params->initial_max_streams_uni = 0;
  params->max_packet_size = NGTCP2_MAX_PKT_SIZE;
  params->ack_delay_exponent = NGTCP2_DEFAULT_ACK_DELAY_EXPONENT;
  params->stateless_reset_token_present = 0;

  for (; (size_t)(end - p) >= sizeof(uint16_t) * 2;) {
    param_type = ngtcp2_get_uint16(p);
    p += sizeof(uint16_t);
    switch (param_type) {
    case NGTCP2_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA:
    case NGTCP2_TRANSPORT_PARAM_INITIAL_MAX_DATA:
      flags |= 1u << param_type;
      if (ngtcp2_get_uint16(p) != sizeof(uint32_t)) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      p += sizeof(uint16_t);
      if ((size_t)(end - p) < sizeof(uint32_t)) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      switch (param_type) {
      case NGTCP2_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA:
        params->initial_max_stream_data = ngtcp2_get_uint32(p);
        break;
      case NGTCP2_TRANSPORT_PARAM_INITIAL_MAX_DATA:
        params->initial_max_data = ngtcp2_get_uint32(p);
        break;
      }
      p += sizeof(uint32_t);
      break;
    case NGTCP2_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_BIDI:
    case NGTCP2_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_UNI:
      flags |= 1u << param_type;
      if (ngtcp2_get_uint16(p) != sizeof(uint16_t)) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      p += sizeof(uint16_t);
      if ((size_t)(end - p) < sizeof(uint16_t)) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      switch (param_type) {
      case NGTCP2_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_BIDI:
        params->initial_max_streams_bidi = ngtcp2_get_uint16(p);
        break;
      case NGTCP2_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_UNI:
        params->initial_max_streams_uni = ngtcp2_get_uint16(p);
        break;
      }
      p += sizeof(uint16_t);
      break;
    case NGTCP2_TRANSPORT_PARAM_IDLE_TIMEOUT:
      flags |= 1u << NGTCP2_TRANSPORT_PARAM_IDLE_TIMEOUT;
      if (ngtcp2_get_uint16(p) != sizeof(uint16_t)) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      p += sizeof(uint16_t);
      if ((size_t)(end - p) < sizeof(uint16_t)) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      params->idle_timeout = ngtcp2_get_uint16(p);
      p += sizeof(uint16_t);
      break;
    case NGTCP2_TRANSPORT_PARAM_MAX_PACKET_SIZE:
      flags |= 1u << NGTCP2_TRANSPORT_PARAM_MAX_PACKET_SIZE;
      if (ngtcp2_get_uint16(p) != sizeof(uint16_t)) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      p += sizeof(uint16_t);
      if ((size_t)(end - p) < sizeof(uint16_t)) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      params->max_packet_size = ngtcp2_get_uint16(p);
      p += sizeof(uint16_t);
      break;
    case NGTCP2_TRANSPORT_PARAM_STATELESS_RESET_TOKEN:
      if (exttype != NGTCP2_TRANSPORT_PARAMS_TYPE_ENCRYPTED_EXTENSIONS) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      flags |= 1u << NGTCP2_TRANSPORT_PARAM_STATELESS_RESET_TOKEN;
      if (ngtcp2_get_uint16(p) != sizeof(params->stateless_reset_token)) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      p += sizeof(uint16_t);
      if ((size_t)(end - p) < sizeof(params->stateless_reset_token)) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }

      memcpy(params->stateless_reset_token, p,
             sizeof(params->stateless_reset_token));
      params->stateless_reset_token_present = 1;

      p += sizeof(params->stateless_reset_token);
      break;
    case NGTCP2_TRANSPORT_PARAM_ACK_DELAY_EXPONENT:
      flags |= 1u << NGTCP2_TRANSPORT_PARAM_ACK_DELAY_EXPONENT;
      if (ngtcp2_get_uint16(p) != 1) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      p += sizeof(uint16_t);
      if (p == end) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      if (*p > 20) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      params->ack_delay_exponent = *p++;
      break;
    default:
      /* Ignore unknown parameter */
      valuelen = ngtcp2_get_uint16(p);
      p += sizeof(uint16_t);
      if ((size_t)(end - p) < valuelen) {
        return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      p += valuelen;
      break;
    }
  }

  if (end - p != 0) {
    return NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM;
  }

#define NGTCP2_REQUIRED_TRANSPORT_PARAMS                                       \
  ((1u << NGTCP2_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA) |                    \
   (1u << NGTCP2_TRANSPORT_PARAM_INITIAL_MAX_DATA) |                           \
   (1u << NGTCP2_TRANSPORT_PARAM_IDLE_TIMEOUT))

  if ((flags & NGTCP2_REQUIRED_TRANSPORT_PARAMS) !=
      NGTCP2_REQUIRED_TRANSPORT_PARAMS) {
    return NGTCP2_ERR_REQUIRED_TRANSPORT_PARAM;
  }

  return 0;
}
