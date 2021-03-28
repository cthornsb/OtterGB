
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
	${GLFW3_INCLUDE_DIRS}
	${OTTER_INCLUDE_DIRS}
	${PORT_AUDIO_INCLUDE_DIRS}
)

if(ENABLE_DEBUGGER)
	target_include_directories(
	COMPONENT_OBJECTS 
	PRIVATE 
	../qtfiles 
	)
endif()

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
if(NOT ENABLE_DEBUGGER)
	add_executable(gbc gbc.cpp SystemGBC.cpp)
else()
	# Create moc code
	QT4_WRAP_CPP(
		MOC_FILENAME
		${TOP_DIRECTORY}/qtfiles/mainwindow.h
		TARGET
		gbc
	)

	# Create uic file
	QT4_WRAP_UI(
		UIC_FILENAME
		${TOP_DIRECTORY}/qtfiles/mainwindow.ui
		TARGET
		gbc
	)
	
	add_executable(gbc gbc.cpp SystemGBC.cpp ${TOP_DIRECTORY}/qtfiles/mainwindow.cpp ${UIC_FILENAME} ${MOC_FILENAME})
endif()

# Add include directories
target_include_directories(
	gbc
	PRIVATE
	../include
	../core/include
	../graphics/include
	../audio/include
	${GLFW3_INCLUDE_DIRS}
	${OTTER_INCLUDE_DIRS}
	${PORT_AUDIO_INCLUDE_DIRS}
)
if(ENABLE_DEBUGGER)
	target_include_directories(
		gbc
		PRIVATE
		../qtfiles
		${CMAKE_CURRENT_BINARY_DIR}
	)
endif()

# Add linker libraries
target_link_libraries(
	gbc 
	COMPONENT_LIB 
	AUDIO_LIB 
	GRAPHICS_LIB 
	CORE_LIB 
	Threads::Threads
	Ott::OtterCore
	Ott::OtterAudio
	Ott::OtterMath
	${GLFW3_LIBRARY}
	${SOIL_LIBRARY}
	OpenGL::GL
	OpenGL::GLU
)
if(WIN32)
	target_link_libraries(
		gbc
		Winmm
	)
endif()
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

# Install executable
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
