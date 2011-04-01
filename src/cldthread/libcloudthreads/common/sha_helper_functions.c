#include <stdlib.h>
#include <stdio.h>

#include "sha_helper_functions.h"

char * sha1_hash_to_hex_digest( const unsigned char *const sha_hash ){

    size_t i;
    char *str = malloc( ((SHA_DIGEST_LENGTH << 1) +1 )*sizeof(char) );

    for (i = 0; i < SHA_DIGEST_LENGTH; i++) sprintf( &str[i<<1], "%02hhx", sha_hash[i]);

    return str;

}

void sha1_hex_digest_to_hash( const char *const hex_digest, unsigned char *const hash_out ){

    size_t i;

    for (i = 0; i < SHA_DIGEST_LENGTH; i++) sscanf( &hex_digest[i<<1], "%02hhx", &hash_out[i]);

}

char * sha1_hex_digest_from_bytes( const char *const bytes, size_t const len, int const shouldFreeInput ){

    unsigned char hash[SHA_DIGEST_LENGTH];

    SHA1( (unsigned char *)bytes, len, hash );

    if(shouldFreeInput) free( (char *)bytes );

    return sha1_hash_to_hex_digest( hash );

}
