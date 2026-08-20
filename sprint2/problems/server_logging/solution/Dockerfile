FROM ubuntu:22.04

# Установка необходимых пакетов
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Установка Conan
RUN pip3 install conan==1.59.0

# Настройка Conan
RUN conan profile new default --detect || true
RUN conan profile update settings.compiler.libcxx=libstdc++11 default || true

WORKDIR /workspace

# Копируем исходники
COPY . .

# Сборка проекта
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build .

# Команда по умолчанию
CMD ["./build/game_server", "0.0.0.0", "8080"]
