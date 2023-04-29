idf_build_get_property(project_dir PROJECT_DIR)

idf_component_register(SRCS "iot_security.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES "esp_common" "mbedtls" "iot_util"
                    EMBED_TXTFILES ${project_dir}/keys/crypt_key)
