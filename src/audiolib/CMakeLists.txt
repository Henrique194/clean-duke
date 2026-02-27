set(LIB audiolib)

list(REMOVE_ITEM LIBS ${LIB})

# Source contains loads of files that are not used (only DOS). Delete them?
add_library(${LIB} STATIC
	src/dpmi.c
	src/dsl.c
	src/fx_man.c
	src/ll_man.c
	src/multivoc.c
	src/mv_mix.c
	src/mvreverb.c
	src/pitch.c
	src/sdl_midi.c
	src/sounds.c
)

target_include_directories(${LIB} PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(${LIB} PUBLIC "./include")
target_link_libraries(${LIB} PRIVATE ${LIBS} ${SDL2_ALL_LIBRARIES})
