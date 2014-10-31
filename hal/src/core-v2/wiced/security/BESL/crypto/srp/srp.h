/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
/*
 * Copyright (c) 1997-2007  The Stanford SRP Authentication Project
 * All Rights Reserved.
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
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL STANFORD BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF
 * THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Redistributions in source or binary form must retain an intact copy
 * of this copyright notice.
 */
#ifndef _SRP_H_
#define _SRP_H_

#include "cstr.h"
#include "srp_aux.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SRP library version identification */
#define SRP_VERSION_MAJOR 2
#define SRP_VERSION_MINOR 0
#define SRP_VERSION_PATCHLEVEL 1

typedef int SRP_RESULT;
/* Returned codes for SRP API functions */
#define SRP_OK(v) ((v) == SRP_SUCCESS)
#define SRP_SUCCESS 0
#define SRP_ERROR -1

/* Set the minimum number of bits acceptable in an SRP modulus */
#define SRP_DEFAULT_MIN_BITS 512
 SRP_RESULT SRP_set_modulus_min_bits P((int minbits));
 int SRP_get_modulus_min_bits P((void));

/*
 * Sets the "secret size callback" function.
 * This function is called with the modulus size in bits,
 * and returns the size of the secret exponent in bits.
 * The default function always returns 256 bits.
 */
typedef int ( * SRP_SECRET_BITS_CB)(int modsize);
 SRP_RESULT SRP_set_secret_bits_cb P((SRP_SECRET_BITS_CB cb));
 int SRP_get_secret_bits P((int modsize));

typedef struct srp_st SRP;

/* Server Lookup API */
typedef struct srp_server_lu_st SRP_SERVER_LOOKUP;

typedef struct srp_s_lu_meth_st {
  const char * name;

  SRP_RESULT ( * init)(SRP_SERVER_LOOKUP * slu);
  SRP_RESULT ( * finish)(SRP_SERVER_LOOKUP * slu);

  SRP_RESULT ( * lookup)(SRP_SERVER_LOOKUP * slu, SRP * srp, cstr * username);

  void * meth_data;
} SRP_SERVER_LOOKUP_METHOD;

struct srp_server_lu_st {
  SRP_SERVER_LOOKUP_METHOD * meth;
  void * data;
};

/*
 * The Server Lookup API deals with the server-side issue of
 * mapping usernames to verifiers.  Given a username, a lookup
 * mechanism needs to provide parameters (N, g), salt (s), and
 * password verifier (v) for that user.
 *
 * A SRP_SERVER_LOOKUP_METHOD describes the general mechanism
 * for performing lookups (e.g. files, LDAP, database, etc.)
 * A SRP_SERVER_LOOKUP is an active "object" that is actually
 * called to do lookups.
 */
 SRP_SERVER_LOOKUP *
     SRP_SERVER_LOOKUP_new P((SRP_SERVER_LOOKUP_METHOD * meth));
 SRP_RESULT SRP_SERVER_LOOKUP_free P((SRP_SERVER_LOOKUP * slu));
 SRP_RESULT SRP_SERVER_do_lookup P((SRP_SERVER_LOOKUP * slu,
                        SRP * srp, cstr * username));

/*
 * SRP_SERVER_system_lookup supercedes SRP_server_init_user.
 */
 SRP_SERVER_LOOKUP * SRP_SERVER_system_lookup P((void));

/*
 * Client Parameter Verification API
 *
 * This callback is called from the SRP client when the
 * parameters (modulus and generator) are set.  The callback
 * should return SRP_SUCCESS if the parameters are okay,
 * otherwise some error code to indicate that the parameters
 * should be rejected.
 */
typedef SRP_RESULT ( * SRP_CLIENT_PARAM_VERIFY_CB)(SRP * srp, const unsigned char * mod, int modlen, const unsigned char * gen, int genlen);

/* The default parameter verifier function */
 SRP_RESULT SRP_CLIENT_default_param_verify_cb(SRP * srp, const unsigned char * mod, int modlen, const unsigned char * gen, int genlen);
/* A parameter verifier that only accepts builtin params (no prime test) */
 SRP_RESULT SRP_CLIENT_builtin_param_verify_cb(SRP * srp, const unsigned char * mod, int modlen, const unsigned char * gen, int genlen);
/* The "classic" parameter verifier that accepts either builtin params
 * immediately, and performs safe-prime tests on N and primitive-root
 * tests on g otherwise.  SECURITY WARNING: This may allow for certain
 * attacks based on "trapdoor" moduli, so this is not recommended. */
 SRP_RESULT SRP_CLIENT_compat_param_verify_cb(SRP * srp, const unsigned char * mod, int modlen, const unsigned char * gen, int genlen);

/*
 * Main SRP API - SRP and SRP_METHOD
 */

/* SRP method definitions */
typedef struct srp_meth_st {
  const char * name;

  SRP_RESULT ( * init)(SRP * srp);
  SRP_RESULT ( * finish)(SRP * srp);

  SRP_RESULT ( * params)(SRP * srp,
                   const unsigned char * modulus, int modlen,
                   const unsigned char * generator, int genlen,
                   const unsigned char * salt, int saltlen);
  SRP_RESULT ( * auth)(SRP * srp, const unsigned char * a, int alen);
  SRP_RESULT ( * passwd)(SRP * srp,
                   const unsigned char * pass, int passlen);
  SRP_RESULT ( * genpub)(SRP * srp, cstr ** result);
  SRP_RESULT ( * key)(SRP * srp, cstr ** result,
                const unsigned char * pubkey, int pubkeylen);
  SRP_RESULT ( * verify)(SRP * srp,
                   const unsigned char * proof, int prooflen);
  SRP_RESULT ( * respond)(SRP * srp, cstr ** proof);

  void * data;
} SRP_METHOD;

/* Magic numbers for the SRP context header */
#define SRP_MAGIC_CLIENT 12
#define SRP_MAGIC_SERVER 28

/* Flag bits for SRP struct */
#define SRP_FLAG_LEFT_PAD  0x2    /* left-pad to length-of-N inside hashes */

/*
 * A hybrid structure that represents either client or server state.
 */
struct srp_st {
  int magic;    /* To distinguish client from server (and for sanity) */
  int flags;
  cstr* username;
  cstr* salt;
  cstr* ex_data;
  BigInteger modulus;
  BigInteger generator;
  BigInteger verifier;
  BigInteger password;
  BigInteger pubkey;
  BigInteger secret;
  BigInteger u;
  BigInteger key;
  SRP_METHOD* meth;
  void* meth_data;
  SRP_CLIENT_PARAM_VERIFY_CB param_cb;    /* to verify params */
  SRP_SERVER_LOOKUP* slu;   /* to look up users */
};

/*
 * Global initialization/de-initialization functions.
 * Call SRP_initialize_library before using the library,
 * and SRP_finalize_library when done.
 */
 SRP_RESULT SRP_initialize_library( void );
 SRP_RESULT SRP_finalize_library( void );

/*
 * SRP_new() creates a new SRP context object -
 * the method determines which "sense" (client or server)
 * the object operates in.  SRP_free() frees it.
 * (See RFC2945 method definitions below.)
 */
 SRP *      SRP_new P((SRP_METHOD * meth));
 SRP_RESULT SRP_free P((SRP * srp));

/*
 * Use the supplied lookup object to look up user parameters and
 * password verifier.  The lookup function gets called during
 * SRP_set_username/SRP_set_user_raw below.  Using this function
 * means that the server can avoid calling SRP_set_params and
 * SRP_set_authenticator, since the lookup function handles that
 * internally.
 */
 SRP_RESULT SRP_set_server_lookup P((SRP * srp,
                         SRP_SERVER_LOOKUP * lookup));

/*
 * Use the supplied callback function to verify parameters
 * (modulus, generator) given to the client.
 */
 SRP_RESULT
     SRP_set_client_param_verify_cb P((SRP * srp,
                       SRP_CLIENT_PARAM_VERIFY_CB cb));

/*
 * Both client and server must call both SRP_set_username and
 * SRP_set_params, in that order, before calling anything else.
 * SRP_set_user_raw is an alternative to SRP_set_username that
 * accepts an arbitrary length-bounded octet string as input.
 */
 SRP_RESULT SRP_set_username P((SRP * srp, const char * username));
 SRP_RESULT SRP_set_user_raw P((SRP * srp, const unsigned char * user,
                    int userlen));
 SRP_RESULT
     SRP_set_params P((SRP * srp,
               const unsigned char * modulus, int modlen,
               const unsigned char * generator, int genlen,
               const unsigned char * salt, int saltlen));

/*
 * On the client, SRP_set_authenticator, SRP_gen_exp, and
 * SRP_add_ex_data can be called in any order.
 * On the server, SRP_set_authenticator must come first,
 * followed by SRP_gen_exp and SRP_add_ex_data in either order.
 */
/*
 * The authenticator is the secret possessed by either side.
 * For the server, this is the bigendian verifier, as an octet string.
 * For the client, this is the bigendian raw secret, as an octet string.
 * The server's authenticator must be the generator raised to the power
 * of the client's raw secret modulo the common modulus for authentication
 * to succeed.
 *
 * SRP_set_auth_password computes the authenticator from a plaintext
 * password and then calls SRP_set_authenticator automatically.  This is
 * usually used on the client side, while the server usually uses
 * SRP_set_authenticator (since it doesn't know the plaintext password).
 */
 SRP_RESULT
     SRP_set_authenticator P((SRP * srp, const unsigned char * a, int alen));
 SRP_RESULT
     SRP_set_auth_password P((SRP * srp, const char * password));
 SRP_RESULT
     SRP_set_auth_password_raw P((SRP * srp,
                  const unsigned char * password,
                  int passlen));

/*
 * SRP_gen_pub generates the random exponential residue to send
 * to the other side.  If using SRP-3/RFC2945, the server must
 * withhold its result until it receives the client's number.
 * If using SRP-6, the server can send its value immediately
 * without waiting for the client.
 *
 * If "result" points to a NULL pointer, a new cstr object will be
 * created to hold the result, and "result" will point to it.
 * If "result" points to a non-NULL cstr pointer, the result will be
 * placed there.
 * If "result" itself is NULL, no result will be returned,
 * although the big integer value will still be available
 * through srp->pubkey in the SRP struct.
 */
 SRP_RESULT SRP_gen_pub P((SRP * srp, cstr ** result));
/*
 * Append the data to the extra data segment.  Authentication will
 * not succeed unless both sides add precisely the same data in
 * the same order.
 */
 SRP_RESULT SRP_add_ex_data P((SRP * srp, const unsigned char * data,
                       int datalen));

/*
 * SRP_compute_key must be called after the previous three methods.
 */
 SRP_RESULT SRP_compute_key P((SRP * srp, cstr ** result,
                       const unsigned char * pubkey,
                       int pubkeylen));

/*
 * On the client, call SRP_respond first to get the response to send
 * to the server, and call SRP_verify to verify the server's response.
 * On the server, call SRP_verify first to verify the client's response,
 * and call SRP_respond ONLY if verification succeeds.
 *
 * It is an error to call SRP_respond with a NULL pointer.
 */
 SRP_RESULT SRP_verify P((SRP * srp,
                  const unsigned char * proof, int prooflen));
 SRP_RESULT SRP_respond P((SRP * srp, cstr ** response));

/* RFC2945-style SRP authentication */
#if defined(USE_SRP_SHA_512)
 #define RFC2945_KEY_LEN  64    /* length of session key (bytes) */
 #define RFC2945_RESP_LEN 64    /* length of proof hashes (bytes) */
#else
 #define RFC2945_KEY_LEN  40    /* length of session key (bytes) */
 #define RFC2945_RESP_LEN 20    /* length of proof hashes (bytes) */
#endif
/*
 * RFC2945-style SRP authentication methods.  Use these like:
 * SRP * srp = SRP_new(SRP_RFC2945_client_method());
 */
 SRP_METHOD * SRP_RFC2945_client_method P((void));
 SRP_METHOD * SRP_RFC2945_server_method P((void));

/*
 * SRP-6 and SRP-6a authentication methods.
 * SRP-6a is recommended for better resistance to 2-for-1 attacks.
 */
 SRP_METHOD * SRP6_client_method P((void));
 SRP_METHOD * SRP6_server_method P((void));
 SRP_METHOD * SRP6a_client_method P((void));
 SRP_METHOD * SRP6a_server_method P((void));


#ifdef __cplusplus
}
#endif

#endif /* _SRP_H_ */
