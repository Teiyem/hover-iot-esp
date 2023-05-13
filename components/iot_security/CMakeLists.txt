idf_build_get_property(project_dir PROJECT_DIR)

idf_component_register(SRCS "iot_security.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES "mbedtls"
                    PRIV_REQUIRES "iot_common" 
                    EMBED_TXTFILES ${project_dir}/keys/crypt_key)
