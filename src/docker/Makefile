DOCKER_IMAGE=md_protoc:latest
DOCKER_FILE=./Dockerfile
DOCKER_BUILDER=.docker_builder

all: $(DOCKER_BUILDER) ## Default target: build docker image

help: ## Provide help message
	@echo "Available targets:"
	@awk -F ':.*?## ' '/^[a-zA-Z0-9_-]+:.*?##/ { printf "  %-20s %s\n", $$1, $$2 }' $(MAKEFILE_LIST)

scrub: force ## Remove docker image
	rm -rf $(DOCKER_BUILDER)
	docker rmi -f $(DOCKER_IMAGE) 2> /dev/null

$(DOCKER_BUILDER): $(DOCKER_FILE)
	docker build . -t $(DOCKER_IMAGE) -f $(DOCKER_FILE)
	touch $(DOCKER_BUILDER)

.PHONY: force