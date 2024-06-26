add_executable(c8
    audio.c
    cpu.c
    keyboard.c
    main.c
    memory.c
)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)

if(TARGET SDL2::SDL2main)
    target_link_libraries(c8 PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(c8 PRIVATE SDL2::SDL2 m)
