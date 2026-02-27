set(LIB console)

list(REMOVE_ITEM LIBS ${LIB})

add_library(${LIB} STATIC
	src/console.c
	src/cvar_defs.c
	src/cvars.c
)

target_include_directories(${LIB} PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(${LIB} PUBLIC "./include")
target_link_libraries(${LIB} PRIVATE ${LIBS} ${SDL2_ALL_LIBRARIES})
