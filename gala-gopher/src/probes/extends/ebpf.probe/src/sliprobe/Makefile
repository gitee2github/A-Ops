include ../mk/var.mk
INCLUDES = $(BASE_INC)

ifndef REDIS_VERSION
REDIS_VERSION = 6.2.1
endif
RDS_VER_MAJOR = $(shell echo $(REDIS_VERSION) | awk -F'.' '{print $$1}')
RDS_VER_MINOR = $(shell echo $(REDIS_VERSION) | awk -F'.' '{print $$2}')
RDS_VER_PATCH = $(shell echo $(REDIS_VERSION) | awk -F'.' '{print $$3}')
CFLAGS += -DRDS_VER_MAJOR=$(RDS_VER_MAJOR) -DRDS_VER_MINOR=$(RDS_VER_MINOR) -DRDS_VER_PATCH=$(RDS_VER_PATCH)

APP := redissli_$(REDIS_VERSION)

BPF_C := $(wildcard *.bpf.c)
DEPS := $(patsubst %.bpf.c, %.bpf.o, $(BPF_C))
DEPS += $(patsubst %.bpf.c, %.skel.h, $(BPF_C))

SRC_C := $(filter-out $(BPF_C), $(wildcard *.c))
SRC_C += $(UTIL_SRC)

.PHONY: all clean install

all: pre deps app
pre: $(OUTPUT)
deps: $(DEPS)
# build bpf code
%.bpf.o: %.bpf.c
	$(CLANG) $(CFLAGS) -target bpf $(INCLUDES) -c $(filter %.c,$^) -o $@
	$(LLVM_STRIP) -g $@

# build skel.h
%.skel.h: %.bpf.o
	$(BPFTOOL) gen skeleton $< > $@

app: $(APP)
%_$(REDIS_VERSION): %.c $(SRC_C)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -lbpf -lelf -lz -lpthread -o $@

clean:
	rm -rf $(DEPS)
	rm -rf $(APP)

install:
	mkdir -p $(INSTALL_DIR)
	cp $(APP) $(INSTALL_DIR)

