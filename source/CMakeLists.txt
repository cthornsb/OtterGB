#System components
set(COMPONENT_SOURCES
	Cartridge.cpp
	Console.cpp
	DmaController.cpp
	GPU.cpp
	Joystick.cpp
	LR35902.cpp
	Serial.cpp
	Sound.cpp
	SystemClock.cpp	
	SystemGBC.cpp
	SystemRegisters.cpp
	SystemTimer.cpp
	WorkRam.cpp
)

if(NOT ${GB_BOOT_ROM} STREQUAL "")
	add_definitions(-DGB_BOOT_ROM="${GB_BOOT_ROM}")
endif()

if(NOT ${GBC_BOOT_ROM} STREQUAL "")
	add_definitions(-DGBC_BOOT_ROM="${GBC_BOOT_ROM}")
endif()

# Generate the system component library
add_library(COMPONENT_OBJECTS OBJECT ${COMPONENT_SOURCES})
add_library(COMPONENT_LIB STATIC $<TARGET_OBJECTS:COMPONENT_OBJECTS>)

#Build renderer executable.
add_executable(gbc gbc.cpp)
if(NOT BUILD_DEBUG)
	target_link_libraries(gbc COMPONENT_LIB AUDIO_LIB GRAPHICS_LIB CORE_LIB ${EXTERNAL_GRAPHICS_LIBS} ${EXTERNAL_AUDIO_LIBS})
else()
	target_link_libraries(gbc COMPONENT_LIB AUDIO_LIB GRAPHICS_LIB CORE_LIB QTDEBUG_LIB ${QT_GUI_LIB} ${QT_CORE_LIB} ${QT_OPENGL_LIB} ${EXTERNAL_GRAPHICS_LIBS} ${EXTERNAL_AUDIO_LIBS})
endif()
install(TARGETS gbc DESTINATION bin)
#Copy default configuration file (if it doesn't already exist)
if(NOT EXISTS ${CMAKE_INSTALL_PREFIX}/bin/default.cfg)
	install(FILES ${TOP_DIRECTORY}/assets/default.cfg DESTINATION bin)
endif()
if(WIN32 AND INSTALL_DLLS)
	#Install required dlls
	install(FILES ${INSTALL_DLL_LIST} DESTINATION bin)
endif(WIN32 AND INSTALL_DLLS)
