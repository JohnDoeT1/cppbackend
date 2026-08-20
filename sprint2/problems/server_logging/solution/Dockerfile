FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    python3-pip \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Скачиваем Boost.JSON в /usr/local/src
RUN cd /tmp \
    && wget https://github.com/boostorg/json/archive/refs/tags/boost-1.78.0.tar.gz \
    && tar -xzf boost-1.78.0.tar.gz \
    && mv json-boost-1.78.0 /usr/local/src/ \
    && rm -rf boost-1.78.0.tar.gz

WORKDIR /workspace

COPY . .

# Создаём минимальный конфиг и статическую папку, если их нет
RUN mkdir -p /workspace/data /workspace/static \
    && echo '{"maps": []}' > /workspace/data/config.json

RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build .

CMD ["./build/game_server", "data/config.json", "static"]
