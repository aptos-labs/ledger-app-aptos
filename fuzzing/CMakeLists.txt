cmake_minimum_required(VERSION 3.10)

if(${CMAKE_VERSION} VERSION_LESS 3.10)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# project information
project(FuzzTxParser
        VERSION 1.0
	      DESCRIPTION "Fuzzing of transaction parser"
        LANGUAGES C CXX)

# guard against bad build-type strings
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build with code coverage generation
message("${CMAKE_CXX_COMPILER_ID}")

# compatible with ClusterFuzzLite
if (NOT CODE_COVERAGE AND NOT DEFINED ENV{LIB_FUZZING_ENGINE})
	set(COMPILATION_FLAGS ${CUSTOM_C_FLAGS} -g -O0 -Wall -Wextra -fprofile-instr-generate -fcoverage-mapping)
  if (SANITIZER MATCHES "address")
    set(COMPILATION_FLAGS ${COMPILATION_FLAGS} -fsanitize=fuzzer,address,undefined)
  elseif (SANITIZER MATCHES "memory")
    set(COMPILATION_FLAGS ${COMPILATION_FLAGS} -fsanitize=fuzzer,memory,undefined -fsanitize-memory-track-origins -fsanitize=fuzzer-no-link)
  else()
    message(FATAL_ERROR "Unknown sanitizer type. It must be set to `address` or `memory`.")
  endif()
else()
	set(COMPILATION_FLAGS "$ENV{LIB_FUZZING_ENGINE} $ENV{CFLAGS} ${CUSTOM_C_FLAGS}")
  separate_arguments(COMPILATION_FLAGS)
endif()

if(CODE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "(Apple)?[Cc]lang")
        add_compile_options(-O1 -g -fsanitize=fuzzer -fprofile-instr-generate -fcoverage-mapping)
        add_link_options(-fsanitize=fuzzer -fprofile-instr-generate -fcoverage-mapping)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-O1 -g -fprofile-arcs -ftest-coverage)
        add_link_options(-fprofile-arcs -ftest-coverage)
        link_libraries(gcov)
    else()
        message(FATAL_ERROR "Unsupported compiler used with code coverage generation")
    endif()
endif()

include(extra/TxParser.cmake)

add_executable(fuzz_tx_parser fuzz_tx_parser.c)
set_target_properties(fuzz_tx_parser PROPERTIES LINKER_LANGUAGE CXX)

target_compile_options(fuzz_tx_parser
    PRIVATE ${COMPILATION_FLAGS}
)

target_link_libraries(fuzz_tx_parser
    PRIVATE ${COMPILATION_FLAGS}
    PUBLIC txparser
)
