
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
	${GLEW_INCLUDE_DIRS}
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

# Generate the system component library
add_library(
	COMPONENT_LIB 
	STATIC 
	$<TARGET_OBJECTS:COMPONENT_OBJECTS>
)

#Build renderer executable.
if(NOT ENABLE_DEBUGGER)
	add_executable( ottergb 
		gbc.cpp 
		SystemGBC.cpp
	)
else()
	# Create moc code
	QT4_WRAP_CPP( MOC_FILENAME
		${TOP_DIRECTORY}/qtfiles/mainwindow.h
		TARGET ottergb
	)

	# Create uic file
	QT4_WRAP_UI( UIC_FILENAME
		${TOP_DIRECTORY}/qtfiles/mainwindow.ui
		TARGET ottergb
	)
	
	add_executable( ottergb 
		gbc.cpp 
		SystemGBC.cpp 
		${TOP_DIRECTORY}/qtfiles/mainwindow.cpp 
		${UIC_FILENAME} 
		${MOC_FILENAME}
	)
endif()

# Bootstrap rom paths (optional)
if(NOT ${DMG_BOOTSTRAP_ROM} STREQUAL "")
	target_compile_definitions( ottergb 
		PRIVATE DMG_BOOTSTRAP_ROM="${DMG_BOOTSTRAP_ROM}"
	)
endif()
if(NOT ${CGB_BOOTSTRAP_ROM} STREQUAL "")
	target_compile_definitions( ottergb
		PRIVATE CGB_BOOTSTRAP_ROM="${CGB_BOOTSTRAP_ROM}"
	)
endif()

# Add include directories
target_include_directories(
	ottergb
	PRIVATE
	../include
	../core/include
	../graphics/include
	../audio/include
	${GLFW3_INCLUDE_DIRS}
	${GLEW_INCLUDE_DIRS}
	${OTTER_INCLUDE_DIRS}
	${PORT_AUDIO_INCLUDE_DIRS}
)
if(ENABLE_DEBUGGER)
	target_include_directories(
		ottergb
		PRIVATE
		../qtfiles
		${CMAKE_CURRENT_BINARY_DIR}
	)
endif()

# Add linker libraries
target_link_libraries(
	ottergb 
	COMPONENT_LIB 
	AUDIO_LIB 
	GRAPHICS_LIB 
	CORE_LIB 
	Ott::OtterCore
	Ott::OtterSystem
	Ott::OtterAudio
	Ott::OtterMath
	${GLFW3_LIBRARY}
	${SOIL_LIBRARY}
	OpenGL::GL
	OpenGL::GLU
	GLEW::GLEW
)
if(WIN32)
	target_link_libraries(
		ottergb
		Winmm
	)
endif()
if(ENABLE_AUDIO)
	target_link_libraries(
		ottergb
		${PORT_AUDIO_LIBRARY}
	)
endif()
if(ENABLE_DEBUGGER)
	target_link_libraries(
		ottergb 
		Qt4::QtGui 
		Qt4::QtCore 
		Qt4::QtOpenGL 
	)
endif()

# Install executable
install(
	TARGETS ottergb
	DESTINATION bin
)

# Install README
install(
	FILES  "${TOP_DIRECTORY}/README.md" 
	DESTINATION bin
)

# Install default configuration file
install(
	FILES "${TOP_DIRECTORY}/assets/default.cfg"
	DESTINATION bin
)

# Install needed mutlimedia assets
install(
	FILES 
	"${TOP_DIRECTORY}/assets/cmap.dat" 
	"${TOP_DIRECTORY}/assets/palettes.dat"
	"${TOP_DIRECTORY}/assets/OtterIcon.png"
	"${TOP_DIRECTORY}/assets/OtterLogo.png"
	DESTINATION bin/assets
)

# Install windows dlls
if(WIN32 AND INSTALL_DLLS)
	#Install required dlls
	install(
		FILES ${INSTALL_DLL_LIST} 
		DESTINATION bin
	)
endif(WIN32 AND INSTALL_DLLS)
