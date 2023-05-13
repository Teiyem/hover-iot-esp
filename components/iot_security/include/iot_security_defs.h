#pragma once

/**
 * A struct that represents encrytion and decryption function input params.
 *
 * @param input A pointer to the input string.
 * @param len The len of the input.
 */
typedef struct enc_dec_crypt_params
{
    const char *input; /**< A pointer to the input string. */
    const size_t len;  /**< The len of the input. */
} enc_dec_crypt_params_t;