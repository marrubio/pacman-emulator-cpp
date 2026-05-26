cmake_minimum_required(VERSION 3.12)
project(pacman_native LANGUAGES CXX)

# Forzar C++20 estándar para el compilador
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Opciones de compilación rápidas para optimizar Raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "Don't build raylib examples")
set(BUILD_GAMES OFF CACHE BOOL "Don't build raylib games")
set(SUPPORT_MODULE_RAUDIO ON CACHE BOOL "Enable audio module in raylib")

# Incluir FetchContent para descargar dependencias dinámicamente
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.0
)
FetchContent_MakeAvailable(raylib)

# Definir los archivos fuentes de nuestro proyecto
set(SOURCES
    src/main.cpp
    src/memory.hpp
    src/memory.cpp
    src/renderer.hpp
    src/renderer.cpp
    src/game.hpp
    src/game.cpp
)

# Definir el ejecutable nativo
add_executable(pacman_native ${SOURCES})

# Enlazar con raylib y añadir sus directorios de inclusión
target_link_libraries(pacman_native PRIVATE raylib)
target_include_directories(pacman_native PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Configuraciones de compilación avanzadas para optimizaciones
if(MSVC)
    target_compile_options(pacman_native PRIVATE /W4 /WX- /O2)
else()
    target_compile_options(pacman_native PRIVATE -Wall -Wextra -O3)
endif()
