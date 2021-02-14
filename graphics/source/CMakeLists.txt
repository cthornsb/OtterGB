set(GRAPHICS_SOURCES 
	Bitmap.cpp
	colors.cpp
	ColorGBC.cpp 
)
if(GRAPHICS_LIBRARY MATCHES "OpenGL")
	# To install OpenGL:
	#sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev
	set(GRAPHICS_SOURCES ${GRAPHICS_SOURCES} GraphicsOpenGL.cpp)
elseif(GRAPHICS_LIBRARY MATCHES "SDL")
	# To install SDL:
	#sudo apt-get install libsdl2-dev
	set(GRAPHICS_SOURCES ${GRAPHICS_SOURCES} GraphicsSDL.cpp)
else()
	message(FATAL_ERROR "Unsupported renderer (${GRAPHICS_LIBRARY})")
endif()

# Generate the graphics library
add_library(GRAPHICS_OBJECTS OBJECT ${GRAPHICS_SOURCES})
add_library(GRAPHICS_LIB STATIC $<TARGET_OBJECTS:GRAPHICS_OBJECTS>)
