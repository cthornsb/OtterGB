add_executable(disassembler disassembler.cpp)
target_include_directories(
	disassembler
	PRIVATE
	../../include
)
install(TARGETS disassembler DESTINATION bin)

add_executable(dmgpalettes palettes.cpp)
target_include_directories(
	dmgpalettes
	PRIVATE
	../../graphics/include
	${OTTER_INCLUDE_DIRS}
)
target_link_libraries(
	dmgpalettes 
	GRAPHICS_LIB
	${OTTER_LIBRARIES}
	${GLFW3_LIBRARY}
	OpenGL::GL
)
install(TARGETS dmgpalettes DESTINATION bin)

# A mess of libraries, but it works for now!
ott_use_3d()
add_executable(gbimage gbimage.cpp)
target_include_directories(
	gbimage
	PRIVATE
	${OTTER_INCLUDE_DIRS}
)
target_link_libraries(
	gbimage
	GRAPHICS_LIB
	${OTTER_LIBRARIES}
	${GLFW3_LIBRARY}
	${SOIL_LIBRARY}
	OpenGL::GL
	OpenGL::GLU
	GLEW::GLEW
)
install(TARGETS gbimage DESTINATION bin)
