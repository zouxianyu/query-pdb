FROM alpine:3.14

MAINTAINER zouxianyu "2979121738@qq.com"

RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.aliyun.com/g' /etc/apk/repositories

RUN apk add --no-cache \
    make \
    gcc \
    g++ \
    git \
    cmake \
    supervisor

COPY . /query-pdb/

RUN cd /query-pdb && \
    mkdir -p build && \
    cd build && \
    cmake .. && \
    cmake --build . --target query_pdb_server

ENTRYPOINT ["/usr/bin/supervisord", "-c", "/query-pdb/supervisord.conf"]

