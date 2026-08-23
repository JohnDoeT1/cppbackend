FROM ubuntu:22.04

# Установка необходимых пакетов
RUN apt-get update && \
    apt-get install -y \
    python3 \
    python3-pip \
    cmake \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

# Установка Conan 2.x
RUN pip3 install conan

# Установка профиля Conan
RUN conan profile detect --force

# Создание рабочей директории
WORKDIR /app

# Копирование исходников
COPY . .

# Сборка проекта
RUN rm -rf build && \
    mkdir -p build && \
    cd build && \
    conan install .. --build=missing -s build_type=Release && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . && \
    cp game_server /app/game_server

# Порт для HTTP-сервера
EXPOSE 8080

# Запуск сервера
CMD ["./game_server", "--config-file", "./data/config.json", "--www-root", "./static"]
