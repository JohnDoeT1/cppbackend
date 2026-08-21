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

# Копирование исходников (build исключен через .dockerignore)
COPY . .

# Сборка проекта
RUN rm -rf build && \
    mkdir -p build && \
    cd build && \
    conan install .. --build=missing -s build_type=Release && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release && \
    cmake --build .

# Порт для HTTP-сервера
EXPOSE 8080

# Запуск сервера
CMD ["./build/game_server", "./data/config.json", "./static"]
