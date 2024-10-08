all: rfs_demo ## Default target. Same as rfs_demo

DOCKER_IMAGE=md_protoc:latest
DOCKER_DIR=../docker
DOCKER_FILE=$(DOCKER_DIR)/Dockerfile
DOCKER_BUILDER=$(DOCKER_DIR)/.docker_builder

PB_MDIF_SPEC_ROOT=../protobuf
PB_COMMON_SPEC=$(PB_MDIF_SPEC_ROOT)/mdif/common.proto
PB_CORE_SPEC=$(PB_MDIF_SPEC_ROOT)/mdif/core/core.proto
RFS_SPEC=$(PB_MDIF_SPEC_ROOT)/mdif/rfs/rfs.proto

PB_GEN_DIR=./_generated
PB_H_FILES=$(PB_GEN_DIR)/mdif/core/core.pb-c.h $(PB_GEN_DIR)/mdif/common.pb-c.h $(PB_GEN_DIR)/mdif/rfs/rfs.pb-c.h
PB_C_FILES=$(PB_GEN_DIR)/mdif/core/core.pb-c.c $(PB_GEN_DIR)/mdif/common.pb-c.c $(PB_GEN_DIR)/mdif/rfs/rfs.pb-c.c

HDLC_SRC=../hdlc/dlc/dlc.c ../hdlc/ports/linux/linux_port.c ../hdlc/yahdlc/yahdlc.c ../hdlc/yahdlc/fcs.c ../hdlc/ports/linux/log/log.c
CORE_CODEC_SRC=../linux_core_codec/core_codec.c
MDIF_SOCKET_SRC=../linux_mdif_socket/mdif_socket.c
CFILES=$(HDLC_SRC) $(PB_C_FILES) $(CORE_CODEC_SRC) $(MDIF_SOCKET_SRC) main.c codec.c
COPT=-Wall -I. -I.. -I../hdlc/ports/linux -g -I$(PB_GEN_DIR)

$(DOCKER_BUILDER): $(DOCKER_FILE)
	make -C $(DOCKER_DIR)

$(PB_GEN_DIR):
	mkdir -p $(PB_GEN_DIR)

$(PB_H_FILES): CMD=protoc-c --c_out $(PB_GEN_DIR) --proto_path=$(PB_MDIF_SPEC_ROOT) $(PB_COMMON_SPEC) $(PB_CORE_SPEC) $(RFS_SPEC)
$(PB_H_FILES)&: $(DOCKER_BUILDER) $(PB_GEN_DIR) $(PB_COMMON_SPEC) $(PB_CORE_SPEC) $(RFS_SPEC)
	docker run --rm --user $(shell id -u):$(shell id -g) -v$(CURDIR)/..:/work -w/work/$(notdir $(CURDIR)) $(DOCKER_IMAGE) $(CMD)

help: ## Provide help message
	@echo "Available targets:"
	@awk -F ':.*?## ' '/^[a-zA-Z0-9_-]+:.*?##/ { printf "  %-20s %s\n", $$1, $$2 }' $(MAKEFILE_LIST)

pb: $(PB_H_FILES) ## Generate protobuf C files

rfs_demo: $(PB_H_FILES) $(CFILES) ## Build demo app
	gcc -o $@ $(COPT) $(CFILES) -l:libprotobuf-c.a -lpthread

clean: ## Remove generated files
	rm -rf rfs_demo $(PB_GEN_DIR)

scrub: clean ## Remove generated files and docker builder
	make -C $(DOCKER_DIR) scrub

.PHONY: all help pb rfs_demo clean scrub
