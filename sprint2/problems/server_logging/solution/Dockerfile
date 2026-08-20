FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    python3-pip \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Скачиваем и устанавливаем Boost.JSON
RUN wget https://github.com/boostorg/json/archive/refs/tags/boost-1.78.0.tar.gz \
    && tar -xzf boost-1.78.0.tar.gz \
    && cp -r json-boost-1.78.0/include/boost/json /usr/include/boost/ \
    && rm -rf boost-1.78.0.tar.gz json-boost-1.78.0

WORKDIR /workspace

COPY . .

RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build .

CMD ["./build/game_server", "data/config.json", "static"]
