if(GRAPHICS_LIBRARY MATCHES "OpenGL")
	# To install OpenGL:
	#sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev
	set(GRAPHICS_SOURCES colors.cpp GraphicsOpenGL.cpp)
elseif(GRAPHICS_LIBRARY MATCHES "SDL")
	# To install SDL:
	#sudo apt-get install libsdl2-dev
	set(GRAPHICS_SOURCES colors.cpp GraphicsSDL.cpp)
else()
	message(STATUS "Unsupported renderer (${GRAPHICS_LIBRARY})")
endif()

# Generate the graphics library
add_library(GRAPHICS_OBJECTS OBJECT ${GRAPHICS_SOURCES})
add_library(GRAPHICS_LIB STATIC $<TARGET_OBJECTS:GRAPHICS_OBJECTS>)
