#ifndef __AES_H
#define __AES_H

#include <stdint.h>
#include <stddef.h>

/*
 * AES-CBC decryption, in place. keybits is 128, 192 or 256; len must be a
 * multiple of 16. Self-contained (no external crypto library).
 */
void aes_cbc_decrypt (const uint8_t *key, int keybits, const uint8_t iv[16],
                      uint8_t *buf, size_t len);

#endif /* __AES_H */
