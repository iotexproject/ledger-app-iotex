cmake_minimum_required(VERSION 3.10)

project(IotxFuzzer VERSION 0.1.9 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

# BOLOS SDK
set(BOLOS_SDK $ENV{BOLOS_SDK})
add_compile_definitions(IO_HID_EP_LENGTH=64)

add_compile_definitions(HAVE_ECC HAVE_HASH)
include_directories(.
        ../src/lib
        "${BOLOS_SDK}/include"
        "${BOLOS_SDK}/lib_cxng/include"
)

# Nanopb
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../nanopb/extra)
find_package(Nanopb REQUIRED)
include_directories(${NANOPB_INCLUDE_DIRS})

nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS ../proto/action.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS}
    PROPERTIES GENERATED TRUE)

add_compile_options(-g -ggdb2 -O3)

# Build with code coverage generation
if(CODE_COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "(Apple)?[Cc]lang")
        add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
        add_link_options(-fprofile-instr-generate -fcoverage-mapping)
    elseif(CMAKE_C_COMPILER_ID MATCHES "GNU")
        add_compile_options(-fprofile-arcs -ftest-coverage)
        link_libraries(gcov)
    else()
        message(FATAL_ERROR "Unsupported compiler used with code coverage generation")
    endif()
endif()

# Fuzzer target
set(APP_SRC_DIR "..")

set(APP_SOURCES
        ${APP_SRC_DIR}/src/lib/pb_parser.c
        ${APP_SRC_DIR}/src/lib/pb_parser.h
        ${APP_SRC_DIR}/src/lib/tx_parser.c
        ${PROTO_SRCS} ${PROTO_HDRS}
)

add_executable(fuzz
        fuzz_iotex.c
        ${APP_SOURCES}
)

target_compile_options(fuzz PUBLIC -fsanitize=fuzzer,address,undefined)
target_link_options(fuzz PUBLIC -fsanitize=fuzzer,address,undefined)
