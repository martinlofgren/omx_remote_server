#include <stdlib.h> // malloc()
#include <string.h>

#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include "ws.h"

/*
 * Function: ws_accept_string
 * --------------------------
 * Calculates the Sec-WebSocket-Accept header value by taking the base64
 * encoded SHA-1 hash of the given Sec-WebSocket-Key (see RFC6455) using 
 * encoding functions from OpenSSL.
 *
 * key: the Sec-WebSocket-Key string.
 * 
 * Returns: pointer to the accept string. The string has been allocated
 *          with malloc and should be freed by the caller.
 *          returns NULL if not succesful.
 */
char* ws_accept_string(const char *key) {
  const char
    *magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  const int
    magic_len = 36,
    key_len = strlen(key);
  EVP_MD_CTX
    *evp_sha1;
  BIO
    *b64,
    *mem;
  BUF_MEM
    *bufferPtr;
  unsigned char
    *buf = malloc(SHA_DIGEST_LENGTH * sizeof(char));

  // Calculate SHA-1 hash
  evp_sha1 = EVP_MD_CTX_create();
  if (!EVP_DigestInit(evp_sha1, EVP_sha1())) {
    perror("EVP_DigestInit fail");
    return(NULL);
  }
  if (!EVP_DigestUpdate(evp_sha1, (const void *)key, key_len)) {
    perror("EVP_DigestUpdate fail");
    return(NULL);
  }
  if (!EVP_DigestUpdate(evp_sha1, magic, magic_len)) {
    perror("EVP_DigestUpdate fail");
    return(NULL);
  }
  if (!EVP_DigestFinal(evp_sha1, buf, NULL)) {
    perror("EVP_DigestFinal fail");
    return(NULL);
  }

  // Base64 encode the hash
  if (!(mem = BIO_new(BIO_s_mem()))) {
    perror("Couldn't allocate BIO_s_mem");
    return(NULL);
  }
  if (!(b64 = BIO_new(BIO_f_base64()))) {
    perror("Couldn't allocate BIO_f_base64");
    return(NULL);
  }
  BIO_push(b64, mem);
  if (BIO_write(b64, buf, SHA_DIGEST_LENGTH) < 1) {
    perror("BIO_write fail");
    return(NULL);
  }
  if (BIO_flush(b64) < 1) {
    perror("BIO_write fail");
    return(NULL);
  }

  // Get pointer to data and make sure buffer is freed when done
  BIO_get_mem_ptr(mem, &bufferPtr);
  BIO_set_close(mem, BIO_CLOSE);
  
  // Now get the data...
  char* ret = malloc(bufferPtr->length * sizeof(char));
  if (ret == NULL) {
    perror("Could not allocate memory");
    return(NULL);
  }
  strcpy(ret, bufferPtr->data);

  // And free all allocated resources.
  BIO_free_all(mem);

  return(ret);
}

