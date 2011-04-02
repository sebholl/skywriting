#pragma once

#include <openssl/sha.h>

char * sha1_hash_to_hex_digest( const unsigned char *sha_hash );
char * sha1_hex_digest_from_bytes( const char *bytes, size_t len, int shouldFreeInput );
void   sha1_hex_digest_to_hash( const char *hex_digest, unsigned char *hash_out );
