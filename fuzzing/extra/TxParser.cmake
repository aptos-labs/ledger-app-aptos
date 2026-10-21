# project information
project(TxParser
        VERSION 1.0
        DESCRIPTION "Transaction parser of Aptos app"
        LANGUAGES C)

# specify C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_FLAGS_DEBUG} -Werror -Wall -Wextra -Wno-unused-function -DFUZZ -pedantic -g -O0"
)

set(BOLOS_SDK $ENV{BOLOS_SDK})

add_library(txparser STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/bcs/init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/bcs/decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/bcs/utf8.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/transaction/utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/transaction/deserialize.c
    ${BOLOS_SDK}/lib_standard_app/format.c
    ${BOLOS_SDK}/lib_standard_app/buffer.c
    ${BOLOS_SDK}/lib_standard_app/varint.c
    ${BOLOS_SDK}/lib_standard_app/read.c
    ${BOLOS_SDK}/lib_standard_app/write.c
    ${BOLOS_SDK}/lib_standard_app/bip32.c
)

set_target_properties(txparser PROPERTIES SOVERSION 1)

target_include_directories(txparser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${BOLOS_SDK}/lib_standard_app
)
