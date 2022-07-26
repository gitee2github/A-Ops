ROOT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

Q = @

CLANG ?= clang
LLVM_STRIP ?= llvm-strip
BPFTOOL ?= $(ROOT_DIR)/../../tools/bpftool
LIBBPF_DIR = $(ROOT_DIR)/../.output
GOPHER_COMMON_DIR = $(ROOT_DIR)/../../../../../common

LIB_DIR ?= $(ROOT_DIR)../lib
CFILES ?= $(wildcard $(LIB_DIR)/*.c)
CFILES += $(wildcard $(GOPHER_COMMON_DIR)/*.c)

CPLUSFILES += $(wildcard $(GOPHER_COMMON_DIR)/*.cpp)

INSTALL_DIR=/usr/bin/extends/ebpf.probe

ARCH = $(shell uname -m)
ifeq ($(ARCH), x86_64)
	ARCH = x86
else ifeq ($(ARCH), aarch64)
	ARCH = arm64
endif

KER_VER = $(shell uname -r | awk -F'-' '{print $$1}')
KER_VER_MAJOR = $(shell echo $(KER_VER) | awk -F'.' '{print $$1}')
KER_VER_MINOR = $(shell echo $(KER_VER) | awk -F'.' '{print $$2}')
KER_VER_PATCH = $(shell echo $(KER_VER) | awk -F'.' '{print $$3}')

LINK_TARGET ?= -lpthread -lbpf -lelf -llog4cplus -lz
EXTRA_CFLAGS ?= -g -O2 -Wall -fPIC
EXTRA_CDEFINE ?= -D__TARGET_ARCH_$(ARCH)
CFLAGS := $(EXTRA_CFLAGS) $(EXTRA_CDEFINE)
CFLAGS += -DKER_VER_MAJOR=$(KER_VER_MAJOR) -DKER_VER_MINOR=$(KER_VER_MINOR) -DKER_VER_PATCH=$(KER_VER_PATCH)
LDFLAGS += -Wl,--copy-dt-needed-entries

CXXFLAGS += -std=c++11 -g -O2 -Wall -fPIC
C++ = g++
CC = gcc

BASE_INC := -I/usr/include \
            -I$(ROOT_DIR)../include \
            -I$(GOPHER_COMMON_DIR) \
             -I$(LIBBPF_DIR)
