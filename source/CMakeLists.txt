
# System components
add_library(
	COMPONENT_OBJECTS 
	OBJECT
	Cartridge.cpp
	Console.cpp
	DmaController.cpp
	GPU.cpp
	Joystick.cpp
	LR35902.cpp
	OpcodeLR35902.cpp
	Serial.cpp
	Sound.cpp
	SpriteAttributes.cpp
	SystemClock.cpp	
	SystemGBC.cpp
	SystemRegisters.cpp
	SystemTimer.cpp
	WorkRam.cpp
)

target_include_directories(
	COMPONENT_OBJECTS 
	PRIVATE 
	../include 
	../core/include
	../graphics/include
	../audio/include
	${OTTER_INCLUDE_CORE}
	${OTTER_INCLUDE_AUDIO}
)

# Bootstrap rom paths (optional)
if(NOT ${GB_BOOT_ROM} STREQUAL "")
	target_compile_definitions(COMPONENT_OBJECTS PRIVATE GB_BOOT_ROM=${GB_BOOT_ROM})
endif()
if(NOT ${GBC_BOOT_ROM} STREQUAL "")
	target_compile_definitions(COMPONENT_OBJECTS PRIVATE GBC_BOOT_ROM=${GBC_BOOT_ROM})
endif()

# Generate the system component library
add_library(
	COMPONENT_LIB 
	STATIC 
	$<TARGET_OBJECTS:COMPONENT_OBJECTS>
)

#Build renderer executable.
add_executable(gbc gbc.cpp)

target_include_directories(
	gbc
	PRIVATE
	../include
	../core/include
	../graphics/include
	${OTTER_INCLUDE_CORE}
	${OTTER_INCLUDE_AUDIO}
)

target_link_libraries(
	gbc 
	COMPONENT_LIB 
	AUDIO_LIB 
	GRAPHICS_LIB 
	CORE_LIB 
	Threads::Threads
	${OTTER_LIBRARIES}
	${GLFW3_LIBRARY}
	OpenGL::GL
	OpenGL::GLU
)
if(ENABLE_AUDIO)
	target_link_libraries(
		gbc
		${PORT_AUDIO_LIBRARY}
	)
endif()
if(ENABLE_DEBUGGER)
	target_link_libraries(
		gbc 
		Qt4::QtGui 
		Qt4::QtCore 
		Qt4::QtOpenGL 
	)
endif()
install(TARGETS gbc DESTINATION bin)

# Copy default configuration file (if it doesn't already exist)
if(NOT EXISTS ${CMAKE_INSTALL_PREFIX}/bin/default.cfg)
	install(FILES ${TOP_DIRECTORY}/assets/default.cfg DESTINATION bin)
endif()

# Install windows dlls
if(WIN32 AND INSTALL_DLLS)
	#Install required dlls
	install(FILES ${INSTALL_DLL_LIST} DESTINATION bin)
endif(WIN32 AND INSTALL_DLLS)
