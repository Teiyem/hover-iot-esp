idf_component_register(SRCS "iot_security.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES "mbedtls"
                    PRIV_REQUIRES "iot_common" 
                    EMBED_TXTFILES keys/crypt_key)
