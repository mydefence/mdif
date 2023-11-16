all: pb ## Default target. Same as pb

DOCKER_IMAGE=md_protoc:latest
DOCKER_DIR=../docker
DOCKER_FILE=$(DOCKER_DIR)/Dockerfile
DOCKER_BUILDER=$(DOCKER_DIR)/.docker_builder

PB_CORE_SPEC_ROOT=../protobuf
PB_CORE_SPEC=$(PB_CORE_SPEC_ROOT)/mdif/core/core.proto

PB_TYPES_DIR=./ts_src/_generated
PB_CORE_TYPES=$(PB_TYPES_DIR)/mdif/core/core.ts

$(DOCKER_BUILDER): $(DOCKER_FILE)
	make -C $(DOCKER_DIR)

$(PB_TYPES_DIR):
	mkdir -p $(PB_TYPES_DIR)

$(PB_CORE_TYPES): CMD=protoc --plugin=./node_modules/.bin/protoc-gen-ts_proto --ts_proto_opt=esModuleInterop=true --ts_proto_opt=oneof=unions --ts_proto_out=$(PB_TYPES_DIR) -I$(PB_CORE_SPEC_ROOT) $(PB_CORE_SPEC)
$(PB_CORE_TYPES): $(DOCKER_BUILDER) $(PB_TYPES_DIR) $(PB_CORE_SPEC)
	docker run --rm --user $(shell id -u):$(shell id -g) -v$(CURDIR)/..:/work -w/work/$(notdir $(CURDIR)) $(DOCKER_IMAGE) $(CMD)

help: ## Provide help message
	@echo "Available targets:"
	@awk -F ':.*?## ' '/^[a-zA-Z0-9_-]+:.*?##/ { printf "  %-20s %s\n", $$1, $$2 }' $(MAKEFILE_LIST)

pb: node_modules $(PB_CORE_TYPES) ## Generate protobuf TS files

node_modules: package.json pnpm-lock.yaml ## Install node modules
	pnpm install

clean: ## Remove generated files
	rm -rf $(PB_TYPES_DIR)

scrub: clean ## Remove generated files, docker builder and node_modules
	rm -rf node_modules
	make -C $(DOCKER_DIR) scrub

.PHONY: all help pb clean scrub
