set(LIB config)

list(REMOVE_ITEM LIBS ${LIB})

add_library(${LIB} STATIC
	src/config.c
	src/scriplib.c
	src/scriplib.h
)

target_include_directories(${LIB} PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(${LIB} PUBLIC "./include")
target_link_libraries(${LIB} PRIVATE ${LIBS} ${SDL2_ALL_LIBRARIES})
