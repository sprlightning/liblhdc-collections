cmake_minimum_required(VERSION 3.15)

idf_component_register(SRCS src/lhdcv5_util_dec.c
							src/lhdcv5BT_dec.c
                       INCLUDE_DIRS inc
                       PRIV_INCLUDE_DIRS inc include src)

target_compile_options(${COMPONENT_LIB} PRIVATE -Werror=implicit-function-declaration)

