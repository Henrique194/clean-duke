set(LIB video)

list(REMOVE_ITEM LIBS ${LIB})

add_library(${LIB} STATIC
    src/vid_display.c
    src/vid_mode.c
    src/vid_palette.c
)

target_include_directories(${LIB} PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(${LIB} PUBLIC "./include")
target_link_libraries(${LIB} PRIVATE ${LIBS} ${SDL2_ALL_LIBRARIES})
