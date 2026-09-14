FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
        cmake \
        g++ \
        make \
        git \
        libboost-json-dev \
        libboost-system-dev \
        libboost-log-dev \
        libboost-program-options-dev \
        libboost-thread-dev \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN rm -rf build && \
    mkdir -p build && \
    cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTING=OFF && \
    cmake --build . --target game_server -j"$(nproc)" && \
    cp game_server /app/game_server

EXPOSE 8080

CMD ["/app/game_server", "--config-file", "/app/data/config.json", "--www-root", "/app/static/"]
