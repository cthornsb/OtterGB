add_library(
	GRAPHICS_OBJECTS 
	OBJECT
	Bitmap.cpp
	ColorGBC.cpp 
	ColorPalette.cpp
	ComponentThread.cpp
)

# Set OtterEngine include directories
target_include_directories(GRAPHICS_OBJECTS PRIVATE ../include ${OTTER_DIRECTORY}/graphics/include)

# Generate the graphics library
add_library(GRAPHICS_LIB STATIC $<TARGET_OBJECTS:GRAPHICS_OBJECTS>)

