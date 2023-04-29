#pragma once

#include <string.h>
#include <stdlib.h>
#include "mbedtls/base64.h"
#include "mbedtls/cipher.h"
#include "esp_random.h"
#include "iot_util.h"

/**
 * A class for handling encryption and decryption.
 */
class iot_security
{
public:
    char *encrypt(encryption_params_t *params);
    char *decrypt(encryption_params_t *params);

private:
    static constexpr const size_t block_size = 16;     /** The cipher block size. */
    static constexpr const char *tag = "iot_security"; /** A constant used to identify the source of the log message of this class. */
    static constexpr const char *delimiter = ";";      /** A constant used concatenate/split the iv and encrypted data. */
    esp_err_t cipher_init(mbedtls_cipher_context_t *ctx, const mbedtls_operation_t operation);
    void cipher_deinit(mbedtls_cipher_context_t *ctx);
    int cipher_cbc_crypt(mbedtls_cipher_context_t *ctx, uint8_t *iv, const uint8_t *input, size_t input_len, uint8_t *output, unsigned int *output_len);
    uint8_t *encode_to_base64(const uint8_t *data, const size_t data_len, const size_t base64_len);
    uint8_t *decode_from_base64(const uint8_t *data, const size_t data_len, size_t *decoded_len);
    char *err_result(uint8_t *temp, mbedtls_cipher_context_t *ctx);
    constexpr size_t base64_align_len(const size_t len);
    constexpr size_t calc_base64_enc_length(const size_t len);
    constexpr size_t calc_base64_dec_length(const size_t len);
    constexpr size_t calc_pad_length(const size_t len);
    constexpr size_t pad_length(const size_t len);
};
