/*
 * Copyright 2019 The OpenSSL Project Authors. All Rights Reserved.
 * Copyright (c) 2019, Oracle and/or its affiliates.  All rights reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <string.h>
#include <openssl/err.h>
#include <openssl/cryptoerr.h>
#include <openssl/params.h>
#include "internal/cryptlib.h"
#include "internal/param_build.h"

typedef union {
    OSSL_UNION_ALIGN;
} OSSL_PARAM_BLD_BLOCK;

#define ALIGN_SIZE  sizeof(OSSL_PARAM_BLD_BLOCK)

static size_t bytes_to_blocks(size_t bytes)
{
    return (bytes + ALIGN_SIZE - 1) / ALIGN_SIZE;
}

static OSSL_PARAM_BLD_DEF *param_push(OSSL_PARAM_BLD *bld, const char *key,
                                      int size, size_t alloc, int type,
                                      int secure)
{
    OSSL_PARAM_BLD_DEF *pd;

    if (bld->curr >= OSSL_PARAM_BLD_MAX) {
        CRYPTOerr(CRYPTO_F_PARAM_PUSH, CRYPTO_R_TOO_MANY_RECORDS);
        return NULL;
    }
    pd = bld->params + bld->curr++;
    memset(pd, 0, sizeof(*pd));
    pd->key = key;
    pd->type = type;
    pd->size = size;
    pd->alloc_blocks = bytes_to_blocks(size);
    if ((pd->secure = secure) != 0)
        bld->secure_blocks += pd->alloc_blocks;
    else
        bld->total_blocks += pd->alloc_blocks;
    return pd;
}

static int param_push_num(OSSL_PARAM_BLD *bld, const char *key,
                          void *num, size_t size, int type)
{
    OSSL_PARAM_BLD_DEF *pd = param_push(bld, key, size, size, type, 0);

    if (pd == NULL)
        return 0;
    if (size > sizeof(pd->num)) {
        CRYPTOerr(CRYPTO_F_PARAM_PUSH_NUM, CRYPTO_R_TOO_MANY_BYTES);
        return 0;
    }
    memcpy(&pd->num, num, size);
    return 1;
}

void ossl_param_bld_init(OSSL_PARAM_BLD *bld)
{
    memset(bld, 0, sizeof(*bld));
}

int ossl_param_bld_push_int(OSSL_PARAM_BLD *bld, const char *key, int num)
{
    return param_push_num(bld, key, &num, sizeof(num), OSSL_PARAM_INTEGER);
}

int ossl_param_bld_push_uint(OSSL_PARAM_BLD *bld, const char *key,
                             unsigned int num)
{
    return param_push_num(bld, key, &num, sizeof(num),
                          OSSL_PARAM_UNSIGNED_INTEGER);
}

int ossl_param_bld_push_long(OSSL_PARAM_BLD *bld, const char *key,
                             long int num)
{
    return param_push_num(bld, key, &num, sizeof(num), OSSL_PARAM_INTEGER);
}

int ossl_param_bld_push_ulong(OSSL_PARAM_BLD *bld, const char *key,
                              unsigned long int num)
{
    return param_push_num(bld, key, &num, sizeof(num),
                          OSSL_PARAM_UNSIGNED_INTEGER);
}

int ossl_param_bld_push_int32(OSSL_PARAM_BLD *bld, const char *key,
                              int32_t num)
{
    return param_push_num(bld, key, &num, sizeof(num), OSSL_PARAM_INTEGER);
}

int ossl_param_bld_push_uint32(OSSL_PARAM_BLD *bld, const char *key,
                               uint32_t num)
{
    return param_push_num(bld, key, &num, sizeof(num),
                          OSSL_PARAM_UNSIGNED_INTEGER);
}

int ossl_param_bld_push_int64(OSSL_PARAM_BLD *bld, const char *key,
                              int64_t num)
{
    return param_push_num(bld, key, &num, sizeof(num), OSSL_PARAM_INTEGER);
}

int ossl_param_bld_push_uint64(OSSL_PARAM_BLD *bld, const char *key,
                               uint64_t num)
{
    return param_push_num(bld, key, &num, sizeof(num),
                          OSSL_PARAM_UNSIGNED_INTEGER);
}

int ossl_param_bld_push_size_t(OSSL_PARAM_BLD *bld, const char *key,
                               size_t num)
{
    return param_push_num(bld, key, &num, sizeof(num),
                          OSSL_PARAM_UNSIGNED_INTEGER);
}

int ossl_param_bld_push_double(OSSL_PARAM_BLD *bld, const char *key,
                               double num)
{
    return param_push_num(bld, key, &num, sizeof(num), OSSL_PARAM_REAL);
}

int ossl_param_bld_push_BN(OSSL_PARAM_BLD *bld, const char *key,
                           const BIGNUM *bn)
{
    int sz = -1, secure = 0;
    OSSL_PARAM_BLD_DEF *pd;

    if (bn != NULL) {
        sz = BN_num_bytes(bn);
        if (sz < 0) {
            CRYPTOerr(CRYPTO_F_OSSL_PARAM_BLD_PUSH_BN,
                      CRYPTO_R_ZERO_LENGTH_NUMBER);
            return 0;
        }
        if (BN_get_flags(bn, BN_FLG_SECURE) == BN_FLG_SECURE)
            secure = 1;
    }
    pd = param_push(bld, key, sz, sz >= 0 ? sz : 0,
                    OSSL_PARAM_UNSIGNED_INTEGER, secure);
    if (pd == NULL)
        return 0;
    pd->bn = bn;
    return 1;
}

int ossl_param_bld_push_utf8_string(OSSL_PARAM_BLD *bld, const char *key,
                                    char *buf, size_t bsize)
{
    OSSL_PARAM_BLD_DEF *pd;

    if (bsize == 0) {
        bsize = strlen(buf) + 1;
    } else if (bsize > INT_MAX) {
        CRYPTOerr(CRYPTO_F_OSSL_PARAM_BLD_PUSH_UTF8_STRING,
                  CRYPTO_R_STRING_TOO_LONG);
        return 0;
    }
    pd = param_push(bld, key, bsize, bsize, OSSL_PARAM_UTF8_STRING, 0);
    if (pd == NULL)
        return 0;
    pd->string = buf;
    return 1;
}

int ossl_param_bld_push_utf8_ptr(OSSL_PARAM_BLD *bld, const char *key,
                                 char *buf, size_t bsize)
{
    OSSL_PARAM_BLD_DEF *pd;

    if (bsize == 0) {
        bsize = strlen(buf) + 1;
    } else if (bsize > INT_MAX) {
        CRYPTOerr(CRYPTO_F_OSSL_PARAM_BLD_PUSH_UTF8_PTR,
                  CRYPTO_R_STRING_TOO_LONG);
        return 0;
    }
    pd = param_push(bld, key, bsize, sizeof(buf), OSSL_PARAM_UTF8_PTR, 0);
    if (pd == NULL)
        return 0;
    pd->string = buf;
    return 1;
}

int ossl_param_bld_push_octet_string(OSSL_PARAM_BLD *bld, const char *key,
                                     void *buf, size_t bsize)
{
    OSSL_PARAM_BLD_DEF *pd;

    if (bsize > INT_MAX) {
        CRYPTOerr(CRYPTO_F_OSSL_PARAM_BLD_PUSH_OCTET_STRING,
                  CRYPTO_R_STRING_TOO_LONG);
        return 0;
    }
    pd = param_push(bld, key, bsize, bsize, OSSL_PARAM_OCTET_STRING, 0);
    if (pd == NULL)
        return 0;
    pd->string = buf;
    return 1;
}

int ossl_param_bld_push_octet_ptr(OSSL_PARAM_BLD *bld, const char *key,
                                  void *buf, size_t bsize)
{
    OSSL_PARAM_BLD_DEF *pd;

    if (bsize > INT_MAX) {
        CRYPTOerr(CRYPTO_F_OSSL_PARAM_BLD_PUSH_OCTET_PTR,
                  CRYPTO_R_STRING_TOO_LONG);
        return 0;
    }
    pd = param_push(bld, key, bsize, sizeof(buf), OSSL_PARAM_OCTET_PTR, 0);
    if (pd == NULL)
        return 0;
    pd->string = buf;
    return 1;
}

static OSSL_PARAM *param_bld_convert(OSSL_PARAM_BLD *bld, OSSL_PARAM *param,
                                     OSSL_PARAM_BLD_BLOCK *blk,
                                     OSSL_PARAM_BLD_BLOCK *secure)
{
    size_t i;
    OSSL_PARAM_BLD_DEF *pd;
    void *p;

    for (i = 0; i < bld->curr; i++) {
        pd = bld->params + i;
        param[i].key = pd->key;
        param[i].data_type = pd->type;
        param[i].data_size = pd->size;
        param[i].return_size = 0;

        if (pd->secure) {
            p = secure;
            secure += pd->alloc_blocks;
        } else {
            p = blk;
            blk += pd->alloc_blocks;
        }
        param[i].data = p;
        if (pd->bn != NULL) {
            /* BIGNUM */
            BN_bn2nativepad(pd->bn, (unsigned char *)p, pd->size);
        } else if (pd->type == OSSL_PARAM_OCTET_PTR
                   || pd->type == OSSL_PARAM_UTF8_PTR) {
            /* PTR */
            *(void **)p = pd->string;
        } else if (pd->type == OSSL_PARAM_OCTET_STRING
                   || pd->type == OSSL_PARAM_UTF8_STRING) {
            if (pd->string != NULL)
                memcpy(p, pd->string, pd->size);
            else
                memset(p, 0, pd->size);
        } else {
            /* Number, but could also be a NULL BIGNUM */
            if (pd->size > sizeof(pd->num))
                memset(p, 0, pd->size);
            else if (pd->size > 0)
                memcpy(p, &pd->num, pd->size);
        }
    }
    param[i] = OSSL_PARAM_construct_end();
    return param;
}

OSSL_PARAM *ossl_param_bld_to_param(OSSL_PARAM_BLD *bld, void **secure)
{
    OSSL_PARAM_BLD_BLOCK *blk, *s = NULL;
    OSSL_PARAM *param;
    const size_t p_blks = bytes_to_blocks((bld->curr + 1) * sizeof(*param));
    const size_t total = ALIGN_SIZE * (p_blks + bld->total_blocks);

    if (bld->secure_blocks > 0) {
        if (secure == NULL) {
            CRYPTOerr(CRYPTO_F_OSSL_PARAM_BLD_TO_PARAM,
                      CRYPTO_R_INVALID_NULL_ARGUMENT);
            return NULL;
        }
        s = OPENSSL_secure_malloc(bld->secure_blocks * ALIGN_SIZE);
        if (s == NULL) {
            CRYPTOerr(CRYPTO_F_OSSL_PARAM_BLD_TO_PARAM,
                      CRYPTO_R_SECURE_MALLOC_FAILURE);
            return NULL;
        }
    }
    param = OPENSSL_malloc(total);
    if (param == NULL) {
        CRYPTOerr(CRYPTO_F_OSSL_PARAM_BLD_TO_PARAM, ERR_R_MALLOC_FAILURE);
        OPENSSL_secure_free(s);
        return NULL;
    }
    if (secure != NULL)
        *secure = s;
    blk = p_blks + (OSSL_PARAM_BLD_BLOCK *)(param);
    param_bld_convert(bld, param, blk, s);
    return param;
}

OSSL_PARAM *ossl_param_bld_to_param_ex(OSSL_PARAM_BLD *bld, OSSL_PARAM *params,
                                       size_t param_n, void *data,
                                       size_t data_n, void *secure,
                                       size_t secure_n)
{
    if (params == NULL || data == NULL) {
        CRYPTOerr(CRYPTO_F_OSSL_PARAM_BLD_TO_PARAM_EX,
                  CRYPTO_R_INVALID_NULL_ARGUMENT);
        return NULL;
    }
    if (param_n < bld->curr + 1) {
        CRYPTOerr(CRYPTO_F_OSSL_PARAM_BLD_TO_PARAM_EX,
                  CRYPTO_R_INSUFFICIENT_PARAM_SIZE);
        return NULL;
    }
    if (data_n < ALIGN_SIZE * bld->total_blocks) {
        CRYPTOerr(CRYPTO_F_OSSL_PARAM_BLD_TO_PARAM_EX,
                  CRYPTO_R_INSUFFICIENT_DATA_SPACE);
        return NULL;
    }
    if (bld->secure_blocks > 0 && secure_n < ALIGN_SIZE * bld->secure_blocks) {
        CRYPTOerr(CRYPTO_F_OSSL_PARAM_BLD_TO_PARAM_EX,
                  CRYPTO_R_INSUFFICIENT_SECURE_DATA_SPACE);
        return NULL;
    }
    param_bld_convert(bld, params, (OSSL_PARAM_BLD_BLOCK *)data,
                      (OSSL_PARAM_BLD_BLOCK *)secure);
    return params;
}