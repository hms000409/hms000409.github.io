cmake_minimum_required(VERSION 3.20)
project(pacman C)

set(CMAKE_C_STANDARD 11)

include_directories(sokol)

add_executable(pacman
        pacman.c
        graphic.c
        rom_data.c
        sokol/sokol.c
)

if(WIN32)
    target_link_libraries(pacman
            opengl32      # D3D11 ���� OpenGL32
            kernel32
            user32
            gdi32
            shell32
    )
endif()