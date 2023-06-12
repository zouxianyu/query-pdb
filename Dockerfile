FROM ubuntu:22.04

MAINTAINER zouxianyu "2979121738@qq.com"

RUN apt-get update

RUN apt-get install -y \
    build-essential \
    cmake \
    supervisor \
    libssl-dev

COPY . /query-pdb/

RUN cd /query-pdb && \
    mkdir -p build && \
    cd build && \
    cmake .. && \
    cmake --build . --target query_pdb_server

ENTRYPOINT ["/usr/bin/supervisord", "-c", "/query-pdb/supervisord.conf"]
