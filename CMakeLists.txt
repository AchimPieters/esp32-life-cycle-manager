idf_component_register(SRCS "main.c" "lcm_ota.c" ...
                       INCLUDE_DIRS ".")

target_add_binary_data(${COMPONENT_TARGET} "certs.pem" TEXT)
