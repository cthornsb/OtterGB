set(CORE_SOURCES LR35902.cpp SystemComponent.cpp GPU.cpp Cartridge.cpp Sound.cpp Joystick.cpp WorkRam.cpp SystemRegisters.cpp SystemTimer.cpp SystemGBC.cpp Support.cpp optionHandler.cpp)

#if(BACKGROUND_DEBUG)
#	add_definitions(-DBACKGROUND_DEBUG)
#endif()

if(NOT ${GB_BOOT_ROM} STREQUAL "")
	add_definitions(-DGB_BOOT_ROM="${GB_BOOT_ROM}")
endif()

if(NOT ${GBC_BOOT_ROM} STREQUAL "")
	add_definitions(-DGBC_BOOT_ROM="${GBC_BOOT_ROM}")
endif()

if(GRAPHICS_SOURCES MATCHES "SDL")
	# To install SDL:
	#sudo apt-get install libsdl2-dev
	set(GRAPHICS_SOURCES colors.cpp Graphics.cpp)
	set(GRAPHICS_LIBS -lSDL2)
elseif(GRAPHICS_LIBRARY MATCHES "OpenGL")
	# To install OpenGL:
	#sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev
	add_definitions(-DUSE_OPENGL)
	set(GRAPHICS_SOURCES colors.cpp vector3.cpp Graphics.cpp)
	set(GRAPHICS_LIBS -lglut -lGLU -lGL)
else()
	message(STATUS "Unsupported renderer (${GRAPHICS_LIBRARY})")
endif()

# Generate the core library
add_library(CORE_OBJECTS OBJECT ${CORE_SOURCES})
add_library(CORE_LIB STATIC $<TARGET_OBJECTS:CORE_OBJECTS>)

# Generate the graphics library
add_library(GRAPHICS_OBJECTS OBJECT ${GRAPHICS_SOURCES})
add_library(GRAPHICS_LIB STATIC $<TARGET_OBJECTS:GRAPHICS_OBJECTS>)

#Build renderer executable.
add_executable(gbc gbc.cpp)
target_link_libraries(gbc CORE_LIB GRAPHICS_LIB ${GRAPHICS_LIBS})
install(TARGETS gbc DESTINATION bin)
