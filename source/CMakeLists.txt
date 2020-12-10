set(CORE_SOURCES Bitmap.cpp
                 Cartridge.cpp 
                 ConfigFile.cpp
                 Console.cpp
                 DmaController.cpp 
                 GPU.cpp 
                 Joystick.cpp
                 LR35902.cpp 
                 SerialController.cpp 
                 Sound.cpp 
                 Support.cpp
                 SystemComponent.cpp
                 SystemGBC.cpp
                 SystemRegisters.cpp 
                 SystemTimer.cpp 
                 WorkRam.cpp)

if(NOT WIN32)
	set(CORE_SOURCES ${CORE_SOURCES} optionHandler.cpp)
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
