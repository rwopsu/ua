cmake_minimum_required(VERSION 3.10)
project(ua VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall")

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Source files
set(UA_SOURCES
    src/ua.cc
    src/filei.cc
)

set(KUA_SOURCES
    src/kua.cc
    src/filei.cc
)

# BLAKE3 source files
set(BLAKE3_SOURCES
    src/blake3.c
    src/blake3_dispatch.c
    src/blake3_portable.c
    src/blake3_sse2.c
    src/blake3_sse41.c
    src/blake3_avx2.c
    src/blake3_avx512.c
)

# xxHash source files
set(XXHASH_SOURCES
    src/xxhash.c
)

# SIMD compilation flags for BLAKE3
set_source_files_properties(src/blake3_sse2.c PROPERTIES COMPILE_FLAGS "-msse2")
set_source_files_properties(src/blake3_sse41.c PROPERTIES COMPILE_FLAGS "-msse4.1")
set_source_files_properties(src/blake3_avx2.c PROPERTIES COMPILE_FLAGS "-mavx2")
set_source_files_properties(src/blake3_avx512.c PROPERTIES COMPILE_FLAGS "-mavx512f -mavx512vl")

# Create executables
add_executable(ua ${UA_SOURCES} ${BLAKE3_SOURCES} ${XXHASH_SOURCES})
add_executable(kua ${KUA_SOURCES} ${BLAKE3_SOURCES} ${XXHASH_SOURCES})

# Include directories
target_include_directories(ua PRIVATE src)
target_include_directories(kua PRIVATE src)

# Link libraries
target_link_libraries(ua PRIVATE OpenSSL::SSL OpenSSL::Crypto pthread)
target_link_libraries(kua PRIVATE OpenSSL::SSL OpenSSL::Crypto pthread)

# Install targets
install(TARGETS ua kua
    RUNTIME DESTINATION bin
)

# Install man pages
install(FILES 
    man/man1/ua.1
    man/man1/kua.1
    DESTINATION share/man/man1
)

# Custom targets for convenience
add_custom_target(build-all
    DEPENDS ua kua
    COMMENT "Building ua and kua"
)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMENT "Removing build directory"
)

# Print configuration info
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "OpenSSL found: ${OPENSSL_FOUND}")
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}") 