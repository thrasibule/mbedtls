/**
 * \file oid.c
 *
 * \brief Object Identifier (OID) database
 *
 *  Copyright (C) 2006-2013, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "polarssl/config.h"

#if defined(POLARSSL_OID_C)

#include "polarssl/oid.h"
#include "polarssl/rsa.h"

#include <stdio.h>

/*
 * Macro to generate an internal function for oid_XXX_from_asn1() (used by
 * the other functions)
 */
#define FN_OID_TYPED_FROM_ASN1( TYPE_T, NAME, LIST )                              \
static const TYPE_T * oid_ ## NAME ## _from_asn1( const asn1_buf *oid )           \
{ return (const TYPE_T *) oid_descriptor_from_buf(LIST, sizeof(TYPE_T), oid->p, oid->len ); }

/*
 * Macro to generate a function for retrieving a single attribute from the
 * descriptor of an oid_descriptor_t wrapper.
 */
#define FN_OID_GET_DESCRIPTOR_ATTR1(FN_NAME, TYPE_T, TYPE_NAME, ATTR1_TYPE, ATTR1) \
int FN_NAME( const asn1_buf *oid, ATTR1_TYPE * ATTR1 )                  \
{                                                                       \
    const TYPE_T *data = oid_ ## TYPE_NAME ## _from_asn1( oid );        \
    if( data == NULL ) return ( POLARSSL_ERR_OID_NOT_FOUND );           \
    *ATTR1 = data->descriptor.ATTR1;                                    \
    return( 0 );                                                        \
}

/*
 * Macro to generate a function for retrieving a single attribute from an
 * oid_descriptor_t wrapper.
 */
#define FN_OID_GET_ATTR1(FN_NAME, TYPE_T, TYPE_NAME, ATTR1_TYPE, ATTR1) \
int FN_NAME( const asn1_buf *oid, ATTR1_TYPE * ATTR1 )                  \
{                                                                       \
    const TYPE_T *data = oid_ ## TYPE_NAME ## _from_asn1( oid );        \
    if( data == NULL ) return ( POLARSSL_ERR_OID_NOT_FOUND );           \
    *ATTR1 = data->ATTR1;                                               \
    return( 0 );                                                        \
}

/*
 * Macro to generate a function for retrieving two attributes from an
 * oid_descriptor_t wrapper.
 */
#define FN_OID_GET_ATTR2(FN_NAME, TYPE_T, TYPE_NAME, ATTR1_TYPE, ATTR1,     \
                         ATTR2_TYPE, ATTR2)                                 \
int FN_NAME( const asn1_buf *oid, ATTR1_TYPE * ATTR1, ATTR2_TYPE * ATTR2 )  \
{                                                                           \
    const TYPE_T *data = oid_ ## TYPE_NAME ## _from_asn1( oid );            \
    if( data == NULL ) return ( POLARSSL_ERR_OID_NOT_FOUND );               \
    *ATTR1 = data->ATTR1;                                                   \
    *ATTR2 = data->ATTR2;                                                   \
    return( 0 );                                                            \
}

/*
 * Macro to generate a function for retrieving the OID based on a single
 * attribute from a oid_descriptor_t wrapper.
 */
#define FN_OID_GET_OID_BY_ATTR1(FN_NAME, TYPE_T, LIST, ATTR1_TYPE, ATTR1)   \
int FN_NAME( ATTR1_TYPE ATTR1, const char **oid_str )                       \
{                                                                           \
    const TYPE_T *cur = LIST;                                               \
    while( cur->descriptor.asn1 != NULL ) {                                 \
        if( cur->ATTR1 == ATTR1 ) {                                         \
            *oid_str = cur->descriptor.asn1;                                \
            return( 0 );                                                    \
        }                                                                   \
        cur++;                                                              \
    }                                                                       \
    return( POLARSSL_ERR_OID_NOT_FOUND );                                   \
}

/*
 * Macro to generate a function for retrieving the OID based on two
 * attributes from a oid_descriptor_t wrapper.
 */
#define FN_OID_GET_OID_BY_ATTR2(FN_NAME, TYPE_T, LIST, ATTR1_TYPE, ATTR1,   \
                                ATTR2_TYPE, ATTR2)                          \
int FN_NAME( ATTR1_TYPE ATTR1, ATTR2_TYPE ATTR2, const char **oid_str )     \
{                                                                           \
    const TYPE_T *cur = LIST;                                               \
    while( cur->descriptor.asn1 != NULL ) {                                 \
        if( cur->ATTR1 == ATTR1 && cur->ATTR2 == ATTR2 ) {                  \
            *oid_str = cur->descriptor.asn1;                                \
            return( 0 );                                                    \
        }                                                                   \
        cur++;                                                              \
    }                                                                       \
    return( POLARSSL_ERR_OID_NOT_FOUND );                                   \
}

/*
 * Core generic function
 */
static const oid_descriptor_t *oid_descriptor_from_buf( const void *struct_set,
                size_t struct_size, const unsigned char *oid, size_t len )
{
    const unsigned char *p = (const unsigned char *) struct_set;
    const oid_descriptor_t *cur;

    if( struct_set == NULL || oid == NULL )
        return( NULL );

    cur = (const oid_descriptor_t *) p;
    while( cur->asn1 != NULL )
    {
        if( strlen( cur->asn1 ) == len &&
            memcmp( cur->asn1, oid, len ) == 0 )
        {
            return( cur );
        }

        p += struct_size;
        cur = (const oid_descriptor_t *) p;
    }

    return( NULL );
}

/*
 * For X520 attribute types
 */
typedef struct {
    oid_descriptor_t    descriptor;
    const char          *short_name;
} oid_x520_attr_t;

static const oid_x520_attr_t oid_x520_attr_type[] =
{
    {
        { OID_AT_CN,          "id-at-commonName",               "Common Name" },
        "CN",
    },
    {
        { OID_AT_COUNTRY,     "id-at-countryName",              "Country" },
        "C",
    },
    {
        { OID_AT_LOCALITY,    "id-at-locality",                 "Locality" },
        "L",
    },
    {
        { OID_AT_STATE,       "id-at-state",                    "State" },
        "ST",
    },
    {
        { OID_AT_ORGANIZATION,"id-at-organizationName",         "Organization" },
        "O",
    },
    {
        { OID_AT_ORG_UNIT,    "id-at-organizationalUnitName",   "Org Unit" },
        "OU",
    },
    {
        { OID_PKCS9_EMAIL,    "emailAddress",                   "E-mail address" },
        "emailAddress",
    },
    {
        { NULL, NULL, NULL },
        NULL,
    }
};

FN_OID_TYPED_FROM_ASN1(oid_x520_attr_t, x520_attr, oid_x520_attr_type);
FN_OID_GET_ATTR1(oid_get_attr_short_name, oid_x520_attr_t, x520_attr, const char *, short_name);

#if defined(POLARSSL_X509_PARSE_C) || defined(POLARSSL_X509_WRITE_C)
/*
 * For X509 extensions
 */
typedef struct {
    oid_descriptor_t    descriptor;
    int                 ext_type;
} oid_x509_ext_t;

static const oid_x509_ext_t oid_x509_ext[] =
{
    {
        { OID_BASIC_CONSTRAINTS,    "id-ce-basicConstraints",   "Basic Constraints" },
        EXT_BASIC_CONSTRAINTS,
    },
    {
        { OID_KEY_USAGE,            "id-ce-keyUsage",           "Key Usage" },
        EXT_KEY_USAGE,
    },
    {
        { OID_EXTENDED_KEY_USAGE,   "id-ce-keyUsage",           "Extended Key Usage" },
        EXT_EXTENDED_KEY_USAGE,
    },
    {
        { OID_SUBJECT_ALT_NAME,     "id-ce-subjectAltName",     "Subject Alt Name" },
        EXT_SUBJECT_ALT_NAME,
    },
    {
        { OID_NS_CERT_TYPE,         "id-netscape-certtype",     "Netscape Certificate Type" },
        EXT_NS_CERT_TYPE,
    },
    {
        { NULL, NULL, NULL },
        0,
    },
};

FN_OID_TYPED_FROM_ASN1(oid_x509_ext_t, x509_ext, oid_x509_ext);
FN_OID_GET_ATTR1(oid_get_x509_ext_type, oid_x509_ext_t, x509_ext, int, ext_type);

static const oid_descriptor_t oid_ext_key_usage[] =
{
    { OID_SERVER_AUTH,      "id-kp-serverAuth",      "TLS Web Server Authentication" },
    { OID_CLIENT_AUTH,      "id-kp-clientAuth",      "TLS Web Client Authentication" },
    { OID_CODE_SIGNING,     "id-kp-codeSigning",     "Code Signing" },
    { OID_EMAIL_PROTECTION, "id-kp-emailProtection", "E-mail Protection" },
    { OID_TIME_STAMPING,    "id-kp-timeStamping",    "Time Stamping" },
    { OID_OCSP_SIGNING,     "id-kp-OCSPSigning",     "OCSP Signing" },
    { NULL, NULL, NULL },
};

FN_OID_TYPED_FROM_ASN1(oid_descriptor_t, ext_key_usage, oid_ext_key_usage);
FN_OID_GET_ATTR1(oid_get_extended_key_usage, oid_descriptor_t, ext_key_usage, const char *, description);

#endif /* POLARSSL_X509_PARSE_C || POLARSSL_X509_WRITE_C */

#if defined(POLARSSL_MD_C)
/*
 * For SignatureAlgorithmIdentifier
 */
typedef struct {
    oid_descriptor_t    descriptor;
    md_type_t           md_alg;
    pk_type_t           pk_alg;
} oid_sig_alg_t;

static const oid_sig_alg_t oid_sig_alg[] =
{
    {
        { OID_PKCS1_MD2,        "md2WithRSAEncryption",     "RSA with MD2" },
        POLARSSL_MD_MD2,      POLARSSL_PK_RSA,
    },
    {
        { OID_PKCS1_MD4,        "md4WithRSAEncryption",     "RSA with MD4" },
        POLARSSL_MD_MD4,      POLARSSL_PK_RSA,
    },
    {
        { OID_PKCS1_MD5,        "md5WithRSAEncryption",     "RSA with MD5" },
        POLARSSL_MD_MD5,      POLARSSL_PK_RSA,
    },
    {
        { OID_PKCS1_SHA1,       "sha-1WithRSAEncryption",   "RSA with SHA1" },
        POLARSSL_MD_SHA1,     POLARSSL_PK_RSA,
    },
    {
        { OID_PKCS1_SHA224,     "sha224WithRSAEncryption",  "RSA with SHA-224" },
        POLARSSL_MD_SHA224,   POLARSSL_PK_RSA,
    },
    {
        { OID_PKCS1_SHA256,     "sha256WithRSAEncryption",  "RSA with SHA-256" },
        POLARSSL_MD_SHA256,   POLARSSL_PK_RSA,
    },
    {
        { OID_PKCS1_SHA384,     "sha384WithRSAEncryption",  "RSA with SHA-384" },
        POLARSSL_MD_SHA384,   POLARSSL_PK_RSA,
    },
    {
        { OID_PKCS1_SHA512,     "sha512WithRSAEncryption",  "RSA with SHA-512" },
        POLARSSL_MD_SHA512,   POLARSSL_PK_RSA,
    },
    {
        { OID_RSA_SHA_OBS,      "sha-1WithRSAEncryption",   "RSA with SHA1" },
        POLARSSL_MD_SHA1,     POLARSSL_PK_RSA,
    },
    {
        { NULL, NULL, NULL },
        0, 0,
    },
};

FN_OID_TYPED_FROM_ASN1(oid_sig_alg_t, sig_alg, oid_sig_alg);
FN_OID_GET_DESCRIPTOR_ATTR1(oid_get_sig_alg_desc, oid_sig_alg_t, sig_alg, const char *, description);
FN_OID_GET_ATTR2(oid_get_sig_alg, oid_sig_alg_t, sig_alg, md_type_t, md_alg, pk_type_t, pk_alg);
FN_OID_GET_OID_BY_ATTR2(oid_get_oid_by_sig_alg, oid_sig_alg_t, oid_sig_alg, pk_type_t, pk_alg, md_type_t, md_alg);
#endif /* POLARSSL_MD_C */

/*
 * For PublicKeyInfo
 */
typedef struct {
    oid_descriptor_t    descriptor;
    pk_type_t           pk_alg;
} oid_pk_alg_t;

static const oid_pk_alg_t oid_pk_alg[] =
{
    {
        { OID_PKCS1_RSA,      "rsaEncryption",   "RSA" },
        POLARSSL_PK_RSA,
    },
    {
        { NULL, NULL, NULL },
        0,
    },
};

FN_OID_TYPED_FROM_ASN1(oid_pk_alg_t, pk_alg, oid_pk_alg);
FN_OID_GET_ATTR1(oid_get_pk_alg, oid_pk_alg_t, pk_alg, pk_type_t, pk_alg);

#if defined(POLARSSL_CIPHER_C)
/*
 * For PKCS#5 PBES2 encryption algorithm
 */
typedef struct {
    oid_descriptor_t    descriptor;
    cipher_type_t       cipher_alg;
} oid_cipher_alg_t;

static const oid_cipher_alg_t oid_cipher_alg[] =
{
    {
        { OID_DES_CBC,              "desCBC",       "DES-CBC" },
        POLARSSL_CIPHER_DES_CBC,
    },
    {
        { OID_DES_EDE3_CBC,         "des-ede3-cbc", "DES-EDE3-CBC" },
        POLARSSL_CIPHER_DES_EDE3_CBC,
    },
    {
        { NULL, NULL, NULL },
        0,
    },
};

FN_OID_TYPED_FROM_ASN1(oid_cipher_alg_t, cipher_alg, oid_cipher_alg);
FN_OID_GET_ATTR1(oid_get_cipher_alg, oid_cipher_alg_t, cipher_alg, cipher_type_t, cipher_alg);
#endif /* POLARSSL_CIPHER_C */

#if defined(POLARSSL_MD_C)
/*
 * For digestAlgorithm
 */
typedef struct {
    oid_descriptor_t    descriptor;
    md_type_t           md_alg;
} oid_md_alg_t;

static const oid_md_alg_t oid_md_alg[] =
{
    {
        { OID_DIGEST_ALG_MD2,       "id-md2",       "MD2" },
        POLARSSL_MD_MD2,
    },
    {
        { OID_DIGEST_ALG_MD4,       "id-md4",       "MD4" },
        POLARSSL_MD_MD4,
    },
    {
        { OID_DIGEST_ALG_MD5,       "id-md5",       "MD5" },
        POLARSSL_MD_MD5,
    },
    {
        { OID_DIGEST_ALG_SHA1,      "id-sha1",      "SHA-1" },
        POLARSSL_MD_SHA1,
    },
    {
        { OID_DIGEST_ALG_SHA1,      "id-sha1",      "SHA-1" },
        POLARSSL_MD_SHA1,
    },
    {
        { OID_DIGEST_ALG_SHA224,    "id-sha224",    "SHA-224" },
        POLARSSL_MD_SHA224,
    },
    {
        { OID_DIGEST_ALG_SHA256,    "id-sha256",    "SHA-256" },
        POLARSSL_MD_SHA256,
    },
    {
        { OID_DIGEST_ALG_SHA384,    "id-sha384",    "SHA-384" },
        POLARSSL_MD_SHA384,
    },
    {
        { OID_DIGEST_ALG_SHA512,    "id-sha512",    "SHA-512" },
        POLARSSL_MD_SHA512,
    },
    {
        { NULL, NULL, NULL },
        0,
    },
};

FN_OID_TYPED_FROM_ASN1(oid_md_alg_t, md_alg, oid_md_alg);
FN_OID_GET_ATTR1(oid_get_md_alg, oid_md_alg_t, md_alg, md_type_t, md_alg);
FN_OID_GET_OID_BY_ATTR1(oid_get_oid_by_md, oid_md_alg_t, oid_md_alg, md_type_t, md_alg);
#endif /* POLARSSL_MD_C */

#if defined(POLARSSL_PKCS12_C)
/*
 * For PKCS#12 PBEs
 */
typedef struct {
    oid_descriptor_t    descriptor;
    md_type_t           md_alg;
    cipher_type_t       cipher_alg;
} oid_pkcs12_pbe_alg_t;

static const oid_pkcs12_pbe_alg_t oid_pkcs12_pbe_alg[] =
{
    {
        { OID_PKCS12_PBE_SHA1_DES3_EDE_CBC, "pbeWithSHAAnd3-KeyTripleDES-CBC", "PBE with SHA1 and 3-Key 3DES" },
        POLARSSL_MD_SHA1,      POLARSSL_CIPHER_DES_EDE3_CBC,
    },
    {
        { OID_PKCS12_PBE_SHA1_DES2_EDE_CBC, "pbeWithSHAAnd2-KeyTripleDES-CBC", "PBE with SHA1 and 2-Key 3DES" },
        POLARSSL_MD_SHA1,      POLARSSL_CIPHER_DES_EDE_CBC,
    },
    {
        { NULL, NULL, NULL },
        0, 0,
    },
};

FN_OID_TYPED_FROM_ASN1(oid_pkcs12_pbe_alg_t, pkcs12_pbe_alg, oid_pkcs12_pbe_alg);
FN_OID_GET_ATTR2(oid_get_pkcs12_pbe_alg, oid_pkcs12_pbe_alg_t, pkcs12_pbe_alg, md_type_t, md_alg, cipher_type_t, cipher_alg);
#endif /* POLARSSL_PKCS12_C */

#if defined _MSC_VER && !defined snprintf
#include <stdarg.h>

#if !defined vsnprintf
#define vsnprintf _vsnprintf
#endif // vsnprintf

/*
 * Windows _snprintf and _vsnprintf are not compatible to linux versions.
 * Result value is not size of buffer needed, but -1 if no fit is possible.
 *
 * This fuction tries to 'fix' this by at least suggesting enlarging the
 * size by 20.
 */
static int compat_snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    int res = -1;

    va_start( ap, format );

    res = vsnprintf( str, size, format, ap );

    va_end( ap );

    // No quick fix possible
    if ( res < 0 )
        return( (int) size + 20 );

    return res;
}

#define snprintf compat_snprintf
#endif

#define POLARSSL_ERR_DEBUG_BUF_TOO_SMALL    -2

#define SAFE_SNPRINTF()                         \
{                                               \
    if( ret == -1 )                             \
        return( -1 );                           \
                                                \
    if ( (unsigned int) ret > n ) {             \
        p[n - 1] = '\0';                        \
        return POLARSSL_ERR_DEBUG_BUF_TOO_SMALL;\
    }                                           \
                                                \
    n -= (unsigned int) ret;                    \
    p += (unsigned int) ret;                    \
}

/* Return the x.y.z.... style numeric string for the given OID */
int oid_get_numeric_string( char *buf, size_t size,
                            const asn1_buf *oid )
{
    int ret;
    size_t i, n;
    unsigned int value;
    char *p;

    p = buf;
    n = size;

    /* First byte contains first two dots */
    if( oid->len > 0 )
    {
        ret = snprintf( p, n, "%d.%d", oid->p[0] / 40, oid->p[0] % 40 );
        SAFE_SNPRINTF();
    }

    /* Prevent overflow in value. */
    if( oid->len > sizeof(value) )
        return( POLARSSL_ERR_DEBUG_BUF_TOO_SMALL );

    value = 0;
    for( i = 1; i < oid->len; i++ )
    {
        value <<= 7;
        value += oid->p[i] & 0x7F;

        if( !( oid->p[i] & 0x80 ) )
        {
            /* Last byte */
            ret = snprintf( p, n, ".%d", value );
            SAFE_SNPRINTF();
            value = 0;
        }
    }

    return( (int) ( size - n ) );
}

#endif /* POLARSSL_OID_C */