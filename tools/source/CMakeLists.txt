#add_executable(disassembler disassembler.cpp)
#install(TARGETS disassembler DESTINATION bin)

add_executable(disassembler disassembler.cpp)
target_include_directories(
	disassembler
	PRIVATE
	../../include
#	${OTTER_INCLUDE_CORE}
)
target_link_libraries(
	disassembler 
#	GRAPHICS_LIB
#	${OTTER_LIBRARIES}
#	${GLFW3_LIBRARY}
#	OpenGL::GL
)
install(TARGETS disassembler DESTINATION bin)

add_executable(dmgpalettes palettes.cpp)
target_include_directories(
	dmgpalettes
	PRIVATE
	../../graphics/include
	${OTTER_INCLUDE_CORE}
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
	${OTTER_INCLUDE_CORE}
	${OTTER_INCLUDE_3D}
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
