set(LIB build)

list(REMOVE_ITEM LIBS ${LIB})

add_library(${LIB} STATIC
	src/engine.c
	src/filesystem.c
	src/fixedPoint_math.c
	src/mathlib.c
	src/mmulti.c
	src/physics.c
	src/sprite.c
	src/tiles.c
	src/timer.c
)

target_include_directories(${LIB} PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(${LIB} PUBLIC "./include")
target_link_libraries(${LIB} PRIVATE ${LIBS} ${SDL2_ALL_LIBRARIES})
