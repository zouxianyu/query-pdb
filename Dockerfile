FROM ubuntu:22.04 AS builder

MAINTAINER zouxianyu "2979121738@qq.com"

RUN apt-get update

RUN apt-get install -y \
    build-essential \
    cmake \
    libssl-dev

WORKDIR /app

COPY . .

RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    cmake --build . --target query_pdb_server -j

FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y \
    ca-certificates \
    libssl3 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/server/query_pdb_server /app/

ENTRYPOINT ["/app/query_pdb_server"]
