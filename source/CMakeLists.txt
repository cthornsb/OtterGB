set(CORE_SOURCES LR35902.cpp SystemComponent.cpp GPU.cpp Cartridge.cpp Sound.cpp Joystick.cpp WorkRam.cpp DmaController.cpp SerialController.cpp SystemRegisters.cpp SystemTimer.cpp SystemGBC.cpp Support.cpp ConfigFile.cpp Bitmap.cpp)

if(NOT WIN32)
	append(CORE_SOURCES optionHandler.cpp)
endif(NOT WIN32)

if(NOT ${GB_BOOT_ROM} STREQUAL "")
	add_definitions(-DGB_BOOT_ROM="${GB_BOOT_ROM}")
endif()

if(NOT ${GBC_BOOT_ROM} STREQUAL "")
	add_definitions(-DGBC_BOOT_ROM="${GBC_BOOT_ROM}")
endif()

# Generate the core library
add_library(CORE_OBJECTS OBJECT ${CORE_SOURCES})
add_library(CORE_LIB STATIC $<TARGET_OBJECTS:CORE_OBJECTS>)

#Build renderer executable.
add_executable(gbc gbc.cpp)
if(NOT BUILD_DEBUG)
	target_link_libraries(gbc CORE_LIB GRAPHICS_LIB ${GRAPHICS_LIBS})
else()
	target_link_libraries(gbc CORE_LIB GRAPHICS_LIB QTDEBUG_LIB ${QT_GUI_LIB} ${QT_CORE_LIB} ${QT_OPENGL_LIB} ${GRAPHICS_LIBS})
endif()
install(TARGETS gbc DESTINATION bin)
#Copy default configuration file (if it doesn't already exist)
if(NOT EXISTS ${CMAKE_INSTALL_PREFIX}/bin/default.cfg)
	install(FILES ${TOP_DIRECTORY}/assets/default.cfg DESTINATION bin)
endif()
if(WIN32 AND INSTALL_DLLS)
	#Install required dlls
	install(FILES ${FREEGLUT_DLL_LIST} DESTINATION bin)
	if(BUILD_DEBUG)
		install(FILES ${QT_DLL_LIST} DESTINATION bin)
	endif(BUILD_DEBUG)
endif(WIN32 AND INSTALL_DLLS)