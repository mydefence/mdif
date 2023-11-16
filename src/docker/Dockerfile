FROM ubuntu:22.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update \
    && apt-get install -y protobuf-c-compiler make gcc wget nodejs unzip

RUN wget https://github.com/protocolbuffers/protobuf/releases/download/v23.2/protoc-23.2-linux-x86_64.zip \
    && unzip protoc-23.2-linux-x86_64.zip -d protoc_dir \
    && cp protoc_dir/bin/protoc /usr/bin/ \
    && rm -rf protoc-23.2-linux-x86_64.zip protoc_dir
