#include "iot_security.h"

/* A pointer to the start of the encryption and decryption key. */
extern const uint8_t crypt_key_start[] asm("_binary_crypt_key_start");

/* A pointer to the end of the encryption and decryption key. */
extern const uint8_t crypt_key_end[] asm("_binary_crypt_key_end");

/**
 * Encrypts data.
 *
 * @param[in] params A pointer to enc_dec_crypt_params containing the data to encrypt.
 * @return A pointer to the encrypted data on success, otherwise a nullptr.
 * @note Ensure to free the encrypted data after usage.
 */
char *IotSecurity::encrypt(enc_dec_crypt_params_t *params)
{
    if (iot_valid_str(params->input) != ESP_OK)
        return nullptr;

    uint8_t iv[BLOCK_SIZE];
    esp_fill_random(iv, BLOCK_SIZE);

    ESP_LOGI(TAG, "%s: Generated Random [iv: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]", __func__,
             iv[0], iv[1], iv[2], iv[3], iv[4], iv[5], iv[6], iv[7], iv[8], iv[9], iv[10], iv[11], iv[12], iv[13],
             iv[14], iv[15]);

    size_t pad_len = pad_length(params->len);

    ESP_LOGI(TAG, "%s: Input data [length %d], added padding [length: %d]", __func__, params->len, pad_len - params->len);

    if (pad_len == params->len)
        pad_len += BLOCK_SIZE; // in order to avoid a buffer overflow.

    auto *output = iot_allocate_mem<uint8_t>(pad_len);

    if (output == nullptr)
    {
        return nullptr;
    }

    mbedtls_cipher_context_t ctx;

    if (cipher_init(&ctx, MBEDTLS_ENCRYPT) != ESP_OK)
        return err_result(output, &ctx);

    size_t output_len = 0;

    int mbedtls_ret = cipher_cbc_crypt(&ctx, iv, reinterpret_cast<const uint8_t *>(params->input), params->len, output, &output_len);

    if (mbedtls_ret)
    {
        ESP_LOGE(TAG, "%s: Failed to encrypt the data [reason: -0x%04X]", __func__, mbedtls_ret);
        return err_result(output, &ctx);
    }

    free_ctx(&ctx);

    if (output_len > pad_len)
    {
        ESP_LOGI(TAG, "%s: Buffer overflow detected [output_len: %d > pad_len: %d]", __func__, output_len, pad_len);
        return err_result(output, nullptr);
    }

    ESP_LOGI(TAG, "%s: Encrypted [total: %d bytes ]. No buffer overflow detected", __func__, output_len);

    const size_t data_base_sf_len = calc_base64_enc_length(output_len);

    ESP_LOGI(TAG, "%s: Calculated required length of base64 enc buffer [to: %d]", __func__, data_base_sf_len);

    uint8_t *encoded_data = encode_to_base64(output, output_len, data_base_sf_len);

    if (encoded_data == nullptr)
    {
        return err_result(output, nullptr);
    }

    free(output);

    const size_t iv_base_sf_len = calc_base64_enc_length(BLOCK_SIZE);

    ESP_LOGI(TAG, "%s: Calculated required length of base64 iv buffer [to: %d]", __func__, iv_base_sf_len);

    uint8_t *encoded_iv = encode_to_base64(iv, BLOCK_SIZE, iv_base_sf_len);

    if (encoded_iv == nullptr)
    {
        return err_result(encoded_data, nullptr);
    }

    auto *iv_data = reinterpret_cast<uint8_t *>(
        iot_cat_with_delimiter(reinterpret_cast<char *>(encoded_iv), reinterpret_cast<char *>(encoded_data), DELIMITER));

    if (iv_data == nullptr)
    {
        free(encoded_data);
        return err_result(encoded_iv, nullptr);
    }

    free(encoded_data);
    free(encoded_iv);

    char *ret = reinterpret_cast<char *>(iv_data);

    ESP_LOGI(TAG, "%s: Done encrypting. Total data [size:  %d]", __func__, strlen(ret));

    return ret;
}

/**
 * Decrypts data.
 *
 * @param[in] params A pointer to enc_dec_crypt_params containing the data to decrypt.
 * @return A pointer to the decrypted data on success, otherwise a nullptr.
 * @note Ensure to free the decrypted data after usage.
 */
char *IotSecurity::decrypt(enc_dec_crypt_params_t *params)
{
    if (iot_valid_str(params->input) != ESP_OK)
    {
        return nullptr;
    }

    uint8_t *encoded_iv = nullptr;
    uint8_t *encoded_encrypted_data = nullptr;

    esp_err_t split_ret = iot_split_with_delimiter(params->input, DELIMITER, reinterpret_cast<char **>(&encoded_iv),
                                                   reinterpret_cast<char **>(&encoded_encrypted_data));

    if (split_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "%s: Failed to extract iv and encrypted data, [data: %s] could be invalid",
                 __func__, params->input);
        return nullptr;
    }

    const size_t iv_len = strlen(reinterpret_cast<char *>(encoded_iv));

    size_t decoded_iv_len = 0;

    uint8_t *iv = decode_from_base64(encoded_iv, iv_len, &decoded_iv_len);

    if (iv == nullptr)
    {
        free(encoded_iv);
        return err_result(encoded_encrypted_data, nullptr);
    }

    if (decoded_iv_len != BLOCK_SIZE)
    {
        ESP_LOGI(TAG, "%s: IV [size: %d is invalid]", __func__, decoded_iv_len);
        free(encoded_iv);
        return err_result(encoded_encrypted_data, nullptr);
    }

    free(encoded_iv);

    const size_t encrypted_len = strlen(reinterpret_cast<char *>(encoded_encrypted_data));

    size_t decoded_encrypted_data_len = 0;

    uint8_t *encrypted_data = decode_from_base64(encoded_encrypted_data, encrypted_len, &decoded_encrypted_data_len);

    if (encrypted_data == nullptr)
    {
        free(encoded_encrypted_data);
        return err_result(iv, nullptr);
    }

    free(encoded_encrypted_data);

    ESP_LOGI(TAG, "%s: Encryption [iv: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]", __func__,
             iv[0], iv[1], iv[2], iv[3], iv[4], iv[5], iv[6], iv[7], iv[8], iv[9], iv[10], iv[11], iv[12], iv[13],
             iv[14], iv[15]);

    ESP_LOGI(TAG, "%s: Encrypted data total [len: %d]", __func__, decoded_encrypted_data_len);

    auto *output =  iot_allocate_mem<uint8_t>(decoded_encrypted_data_len);

    if (output == nullptr)
    {
        free(iv);
        return err_result(encrypted_data, nullptr);
    }

    mbedtls_cipher_context_t ctx;

    if (cipher_init(&ctx, MBEDTLS_DECRYPT) != ESP_OK)
    {
        free(iv);
        free(encrypted_data);
        return err_result(output, &ctx);
    }

    size_t output_len = 0;

    int mbedtls_ret = cipher_cbc_crypt(&ctx, iv, encrypted_data, decoded_encrypted_data_len, output, &output_len);

    if (mbedtls_ret)
    {
        free(iv);
        free(encrypted_data);
        ESP_LOGE(TAG, "%s: Failed to decrypt the data [reason: -0x%04X]", __func__, -mbedtls_ret);
        return err_result(output, &ctx);
    }

    free_ctx(&ctx);

    output[output_len] = '\0';

    free(iv);
    free(encrypted_data);

    char *ret = reinterpret_cast<char *>(output);

    ESP_LOGI(TAG, "%s: Done decrypting. Total data [size: %d]", __func__, strlen(ret));

    return ret;
}

/**
 * Initializes the AES cipher context with a 256-bit key and the specified operation mode (encrypt or decrypt).
 *
 * @param[in] ctx The AES cipher context to be initialized.
 * @param[in] operation The operation mode (encrypt or decrypt).
 * @return Returns ESP_OK on success, otherwise an error code
 */
esp_err_t IotSecurity::cipher_init(mbedtls_cipher_context_t *ctx, const mbedtls_operation_t operation)
{
    esp_err_t err_ret = ESP_FAIL;

    const uint8_t *key = const_cast<uint8_t *>(crypt_key_start);

    const mbedtls_cipher_info_t *info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_256_CBC);

    mbedtls_cipher_init(ctx);

    int ret = mbedtls_cipher_setup(ctx, info);

    if (ret)
    {
        ESP_LOGI(TAG, "%s: Failed to setup cipher reason [reason: -0x%04X]", __func__, -ret);
        free_ctx(ctx);
        return err_ret;
    }

    ret = mbedtls_cipher_setkey(ctx, key, 256, operation);

    if (ret)
    {
        ESP_LOGI(TAG, "%s: Failed to set key reason [reason: -0x%04X]", __func__, -ret);
        free_ctx(ctx);
        return err_ret;
    }

    return ESP_OK;
}

/**
 * Encrypts or decrypts data using AES-CBC.
 *
 * @param[in] ctx A pointer to the mbedtls aes context.
 * @param[in] iv The initialization vector used in CBC mode.
 * @param[in] input The data to be encrypted or decrypted.
 * @param[in] input_len The length of the data.
 * @param[out] output The output buffer to store the encrypted or decrypted data.
 * @param[out] output_len The length of the output buffer.
 * @return 0 on success, otherwise an error code.
 */
int IotSecurity::cipher_cbc_crypt(mbedtls_cipher_context_t *ctx, uint8_t *iv, const uint8_t *input, size_t input_len, uint8_t *output, unsigned int *output_len)
{
    return mbedtls_cipher_crypt(ctx, iv, BLOCK_SIZE, input, input_len, output, output_len);
}

/**
 * Encodes a string to a base64-encoded string.
 *
 * @param[in] data The data to be encoded.
 * @param[in] data_len The length of the data.
 * @param[in] base64_len The length of the base64-encoded data.
 * @return A pointer to the base64-encoded data on success, otherwise a nullptr.
 */
uint8_t *IotSecurity::encode_to_base64(const uint8_t *data, const size_t data_len, const size_t base64_len)
{
    ESP_LOGI(TAG, "%s: Encoding data to base64...", __func__);

    auto *encoded_data = iot_allocate_mem<uint8_t>(base64_len);

    if (encoded_data == nullptr)
        return nullptr;

    size_t encoded_data_len = 0;

    int ret = mbedtls_base64_encode(encoded_data, base64_len, &encoded_data_len, data, data_len);

    if (ret)
    {
        ESP_LOGE(TAG, "%s: Failed to encode data [reason: -0x%04X]", __func__, -ret);
        free(encoded_data);
        return nullptr;
    }

    ESP_LOGI(TAG, "%s: Encoded data [total: %d]", __func__, encoded_data_len);

    return encoded_data;
}

/**
 * Decodes a base64-encoded string.
 *
 * @param[in] data The data to be decoded.
 * @param[in] data_len The length of the data.
 * @param[out] decoded_len The number of bytes decoded.
 * @return A pointer to the decoded data on success, otherwise a nullptr.
 */
uint8_t *IotSecurity::decode_from_base64(const uint8_t *data, const size_t data_len, size_t *decoded_len)
{
    ESP_LOGI(TAG, "%s: Decoding data from base64...", __func__);

    const size_t align_len = base64_align_len(data_len);

    ESP_LOGI(TAG, "%s: Calculated a total data length [alignment: %d]", __func__, align_len);

    auto *data_tmp = iot_allocate_mem<uint8_t>(align_len + 1);

    if (data_tmp == nullptr)
        return nullptr;

    memcpy(data_tmp, data, align_len);

    for (size_t i = data_len; i < align_len; i++)
    {
        data_tmp[i] = '=';
    }
    data_tmp[align_len] = '\0';

    const size_t base_sf_len = calc_base64_dec_length(data_len);

    ESP_LOGI(TAG, "%s: Calculated a total base64 buffer length of [%d ]", __func__, base_sf_len);

    auto *decoded_data = iot_allocate_mem<uint8_t>(base_sf_len);

    if (decoded_data == nullptr)
    {
        free(data_tmp);
        return nullptr;
    }

    size_t decoded_data_len = 0;

    int mbedtls_ret = mbedtls_base64_decode(decoded_data, base_sf_len, &decoded_data_len, (const uint8_t *)data_tmp, align_len);

    if (mbedtls_ret)
    {
        ESP_LOGE(TAG, "%s: Failed to decode data [reason: -0x%04X]", __func__, -mbedtls_ret);
        free(decoded_data);
        free(data_tmp);
        return nullptr;
    }

    free(data_tmp);

    *decoded_len = decoded_data_len;

    ESP_LOGI(TAG, "%s: Decoded data total len of [%d]", __func__, decoded_data_len);

    return decoded_data;
}

/**
 * Frees memory allocated for a buffer and a cipher context and returns a null pointer.
 *
 * @param[in] buffer A pointer to a buffer allocated with malloc.
 * @param[in] ctx A pointer to the mbedtls cipher context.
 * @return A null pointer.
 */
char *IotSecurity::err_result(uint8_t *buffer, mbedtls_cipher_context_t *ctx)
{
    if (buffer != nullptr)
        free(buffer);

    free_ctx(ctx);

    return nullptr;
}

/**
 * Frees the given cipher context.
 *
 * @param[in] ctx A pointer to the mbedtls cipher context.
 */
void IotSecurity::free_ctx(mbedtls_cipher_context_t *ctx)
{
    if (ctx != nullptr)
        mbedtls_cipher_free(ctx);
}

/**
 * Aligns the given length to a multiple of 4.
 *
 * @param[in] len The input length.to align.
 * @return The aligned length.
 */
constexpr size_t IotSecurity::base64_align_len(const size_t len)
{
    return ((len + 3) & ~3u);
}

/**
 * Calculates the length of the base64-encoded string for the given input length.
 *
 * @param[in] len The input length.
 * @return The length of the resulting base64-encoded string.
 */
constexpr size_t IotSecurity::calc_base64_enc_length(const size_t len)
{
    return (((len + 2) / 3) * 4 + 1);
}

/**
 * Calculates the maximum length of the decoded base64 string for the given input length.
 *
 * @param[in] len The input length.
 * @return The maximum length of the decoded base64 string.
 */
constexpr size_t IotSecurity::calc_base64_dec_length(const size_t len)
{
    return (base64_align_len(len) / 4 * 3 + 1);
}

/**
 * Calculates the number of padding bytes.
 *
 * @param[in] len The input length.
 * @return The padding length of the data buffer.
 */
constexpr size_t IotSecurity::calc_pad_length(const size_t len)
{
    return BLOCK_SIZE - (len % BLOCK_SIZE);
}

/**
 * Calculates the total length required for padding the input length to the next multiple of the block size.
 *
 * @param[in] len The input length.
 * @return The padded length if the.
 */
constexpr size_t IotSecurity::pad_length(const size_t len)
{
    size_t ret = len;

    if (ret % BLOCK_SIZE)
    {
        ret += calc_pad_length(len);
    }
    return ret;
}