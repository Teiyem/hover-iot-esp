#pragma once

#include <cstring>
#include <cstdlib>
#include "mbedtls/base64.h"
#include "mbedtls/cipher.h"
#include "esp_random.h"
#include "iot_security_defs.h"
#include "iot_common.h"

/**
 * A class for handling encryption and decryption.
 */
class IotSecurity
{
public:
    char *encrypt(enc_dec_crypt_params_t *params);
    char *decrypt(enc_dec_crypt_params_t *params);

private:
    static constexpr const char *TAG = "IotSecurity";  /**< A constant used to identify the source of the log message of this class. */
    static constexpr const size_t BLOCK_SIZE = 16;     /**< The cipher block size. */
    static constexpr const char *DELIMITER = ";";      /**< A constant used concatenate/split the iv and encrypted data. */
    esp_err_t cipher_init(mbedtls_cipher_context_t *ctx, mbedtls_operation_t operation);
    void free_ctx(mbedtls_cipher_context_t *ctx);
    int cipher_cbc_crypt(mbedtls_cipher_context_t *ctx, uint8_t *iv, const uint8_t *input, size_t input_len, uint8_t *output, unsigned int *output_len);
    uint8_t *encode_to_base64(const uint8_t *data, size_t data_len, size_t base64_len);
    uint8_t *decode_from_base64(const uint8_t *data, size_t data_len, size_t *decoded_len);
    char *err_result(uint8_t *temp, mbedtls_cipher_context_t *ctx);

    static constexpr size_t base64_align_len(size_t len);
    static constexpr size_t calc_base64_enc_length(size_t len);
    static constexpr size_t calc_base64_dec_length(size_t len);
    static constexpr size_t calc_pad_length(size_t len);
    static constexpr size_t pad_length(size_t len);
};
