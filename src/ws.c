#include <unistd.h> // write()
#include <stdlib.h> // malloc()
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <ev.h>

#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include "ev_sock.h"
#include "ws.h"
#include "player.h"

#define DEBUG

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
  strcpy(ret, bufferPtr->data);  // <<<--- REWRITE USING memcpy() ?

  // And free all allocated resources.
  BIO_free_all(mem);

  return(ret);
}

/*
 * Function: ws_parse
 * ------------------
 * Parses a websocket maessage by twisting bits. 
 *
 * TODO: Introduce some proper error checking.
 *       Should return just the decoded msg instead of structure?
 *
 * wm: the websocket message structure to be populated
 * msg: the message to be parsed
 */
void ws_parse(ws_msg* wm, const char *msg) {
  char len, *ptr = (char*) msg;
  char *ptr2;
  int i, mask_offset;
  
  wm->fin = (*ptr >> 7) & 1;
  wm->opcode = *ptr & 0x0F;
  ptr++;
  wm->mask = (*ptr >> 7) & 1;
  len = *ptr & 0x7F;
  if (len < 126) {
    wm->payload_len = (unsigned int) len;
    mask_offset = 2;
  }
  else if (len == 126) {
    // do other stuff
  }
  else if (len == 127) {
    // do yet other stuff
  }
  ptr = (char*) msg + mask_offset;
  for (i = 0; i < 4; i++) {
    wm->mask_key[i] = *(ptr++);
  }
  ptr = (char*) msg + mask_offset + 4;
  wm->payload_data = malloc(sizeof(char) * len + 1);
  ptr2 = wm->payload_data;
  for (i = 0; i < wm->payload_len; i++) {
    *(ptr2++) = (*(ptr++)) ^ wm->mask_key[i % 4];
  }
  *ptr2 = 0;
}

/*
 * Function: ws_encode
 * -------------------
 * Encode a websocket maessage by twisting bits. 
 *
 * msg: the message to be encoded
 * len: length of msg
 * enc_msg: char pointer which, when the function returns, will contain the
 *          adress to the encoded websocket frame. The string has been 
 *          allocated with malloc, and it is the responsibility of the caller
 *          to free it after use
 *
 * Returns: length of encoded message
 */
#define MASK_LENGTH 4
int ws_encode(const char *msg, const int len, char **enc_msg) {
  // Decide on total length of message
  int ctrl_len;
  
  if (len < 126) {
    ctrl_len = 2;
  }
  else if (len == 126) {
    ctrl_len = 8;
    // And some more things
  }
  else if (len == 127) {
    ctrl_len = 14;
    // And some more things
  }

  // Allocate return string and build encoded websocket frame
  *enc_msg = malloc((len + ctrl_len) * sizeof(char));
  if (enc_msg == NULL)
    perror ("malloc() failed");
  char *cp = *enc_msg;
  
  *(cp++) = 0x80 | 0x01;            // (FIN = 1)  | (opcode = 0x01
  *(cp++) = 0x00 | len;             // (MASK = 0) | (payload length)
  memcpy(cp, msg, len);
  return (len + ctrl_len);
}

/*
 * Function: ws_client_consumer
 * ----------------------------
 *  
 *
 * w: the sock structure called by the event loop, containing relevant file
 *    descriptor for communication.
 * msg: the message read from the socket
 * len: length of msg
 */
void ws_client_consumer(ev_sock *w, const char *msg, const int len) {
#ifdef DEBUG
  puts("++++++[ enter ws_client ]++++++");
#endif
  
  ws_msg parsed_msg;
  ws_parse(&parsed_msg, msg);

  player_control(w, parsed_msg.payload_data, parsed_msg.payload_len);
  
  free(parsed_msg.payload_data);
#ifdef DEBUG
  puts("++++++[ exit ws_client ]++++++");
#endif
}

/*
 * Function: ws_client_producer
 * ----------------------------
 * 
 *
 * w: the sock structure called by the event loop, containing relevant file
 *    descriptor for communication.
 * msg: the message read from the socket
 * len: length of msg
 */
void ws_client_producer(ev_sock *w, const char *msg, const int len) {
#ifdef DEBUG
  char tmp[1024];
  snprintf(tmp, len, msg);
  printf("ws_client_produce event: %s\n", tmp);
#endif
  
  char* encoded = NULL;
  int enc_len;
  enc_len = ws_encode(msg, len, &encoded);

  if (send(w->io.fd, encoded, (size_t) enc_len, MSG_DONTWAIT) < 0) {
    perror("send() fail");
  }
  
  free(encoded);
}
