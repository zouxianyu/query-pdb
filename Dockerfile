FROM alpine:3.14

MAINTAINER zouxianyu "2979121738@qq.com"

RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.aliyun.com/g' /etc/apk/repositories

RUN apk add --no-cache make
RUN apk add --no-cache gcc
RUN apk add --no-cache g++
RUN apk add --no-cache git
RUN apk add --no-cache cmake
RUN apk add --no-cache supervisor

RUN addgroup --gid 10001 --system nonroot \
 && adduser  --uid 10000 --system --ingroup nonroot --home /home/nonroot nonroot

COPY * /home/nonroot/query-pdb

COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf

RUN cd /home/nonroot/query-pdb && \
    mkdir -p build && \
    cd build && \
    cmake .. && \
    cmake --build . --target query_pdb_server

ENTRYPOINT ["/usr/bin/supervisord"]

