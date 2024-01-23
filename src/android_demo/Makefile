all: android_demo ## Default target. Same as android_demo

DOCKER_IMAGE=md_protoc:latest
DOCKER_DIR=../docker
DOCKER_FILE=$(DOCKER_DIR)/Dockerfile
DOCKER_BUILDER=$(DOCKER_DIR)/.docker_builder

PB_MDIF_SPEC_ROOT=../protobuf
JAVA_OUT_DIR=app/src/main/java
JAVA_PACKAGE=dk/mydefence
JAVA_TARGET_FILES=mdif/common/Common.java mdif/rfs/Rfs.java mdif/fwu/Fwu.java mdif/core/Core.java

# Search for all proto files within PB_MDIF_SPEC_ROOT
SOURCES := $(shell find $(PB_MDIF_SPEC_ROOT) -name '*.proto')

# Prepend path to targets
TARGETS=$(JAVA_TARGET_FILES:%=./$(JAVA_OUT_DIR)/$(JAVA_PACKAGE)/%)

$(DOCKER_BUILDER): $(DOCKER_FILE)
	make -C $(DOCKER_DIR)

$(TARGETS): $(SOURCES) $(DOCKER_BUILDER)
	docker run --rm --user $(shell id -u):$(shell id -g) -v$(PWD)/..:/work -w/work/$(notdir $(CURDIR)) $(DOCKER_IMAGE) protoc --java_out=$(JAVA_OUT_DIR) -I$(PB_MDIF_SPEC_ROOT) $(SOURCES)

help: ## Provide help message
	@echo "Available targets:"
	@awk -F ':.*?## ' '/^[a-zA-Z0-9_-]+:.*?##/ { printf "  %-20s %s\n", $$1, $$2 }' $(MAKEFILE_LIST)

pb: $(TARGETS) ## Generate protobuf java files

android_demo: $(TARGETS) ## Build demo app
	./gradlew assemble

clean: ## Remove generated files
	rm -rf $(JAVA_OUT_DIR)/$(JAVA_PACKAGE)/mdif
	./gradlew clean

scrub: clean ## Remove generated files and docker builder
	make -C $(DOCKER_DIR) scrub

echo:
	@echo $(SOURCES)
	@echo $(TARGETS)


.PHONY: all help pb android_demo clean scrub

