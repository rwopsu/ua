# Compiler and flags
CXX = g++
CC = gcc
CXXFLAGS = -O3 -I src -Wall -std=c++98
CFLAGS = -O3 -I src -Wall
LDFLAGS = -lssl -lcrypto

# Directories
SRC_DIR = src
BUILD_DIR = build

# SIMD flags for BLAKE3
SSE2_FLAGS = -msse2
SSE41_FLAGS = -msse4.1
AVX2_FLAGS = -mavx2
AVX512_FLAGS = -mavx512f -mavx512vl

# Source files
UA_SRC = $(SRC_DIR)/ua.cc $(SRC_DIR)/filei.cc \
  $(SRC_DIR)/blake3.c $(SRC_DIR)/blake3_dispatch.c $(SRC_DIR)/blake3_portable.c \
  $(SRC_DIR)/blake3_sse2.c $(SRC_DIR)/blake3_sse41.c $(SRC_DIR)/blake3_avx2.c $(SRC_DIR)/blake3_avx512.c $(SRC_DIR)/xxhash.c
KUA_SRC = $(SRC_DIR)/kua.cc $(SRC_DIR)/filei.cc \
  $(SRC_DIR)/blake3.c $(SRC_DIR)/blake3_dispatch.c $(SRC_DIR)/blake3_portable.c \
  $(SRC_DIR)/blake3_sse2.c $(SRC_DIR)/blake3_sse41.c $(SRC_DIR)/blake3_avx2.c $(SRC_DIR)/blake3_avx512.c $(SRC_DIR)/xxhash.c

# Object files
UA_OBJS = $(UA_SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
UA_OBJS := $(UA_OBJS:$(SRC_DIR)/%.cc=$(BUILD_DIR)/%.o)
KUA_OBJS = $(KUA_SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
KUA_OBJS := $(KUA_OBJS:$(SRC_DIR)/%.cc=$(BUILD_DIR)/%.o)

# Binaries
UA_BIN = $(BUILD_DIR)/ua
KUA_BIN = $(BUILD_DIR)/kua

# Default target
all: $(UA_BIN) $(KUA_BIN)

# Build rules
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# SIMD-specific build rules
$(BUILD_DIR)/blake3_sse2.o: $(SRC_DIR)/blake3_sse2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SSE2_FLAGS) -c $< -o $@

$(BUILD_DIR)/blake3_sse41.o: $(SRC_DIR)/blake3_sse41.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SSE41_FLAGS) -c $< -o $@

$(BUILD_DIR)/blake3_avx2.o: $(SRC_DIR)/blake3_avx2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(AVX2_FLAGS) -c $< -o $@

$(BUILD_DIR)/blake3_avx512.o: $(SRC_DIR)/blake3_avx512.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(AVX512_FLAGS) -c $< -o $@

$(UA_BIN): $(UA_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(KUA_BIN): $(KUA_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -f $(BUILD_DIR)/*.o $(UA_BIN) $(KUA_BIN)

dist-clean: clean
	rm -rf $(BUILD_DIR)
	rm -f configure Makefile.in aclocal.m4 config.h.in
	rm -rf autom4te.cache
	rm -f ltmain.sh compile install-sh depcomp missing
	rm -f ua kua
	rm -f config.log Makefile
	@if [ -L COPYING ]; then rm -f COPYING; fi
	@if [ -L INSTALL ]; then rm -f INSTALL; fi

.PHONY: all clean dist-clean 