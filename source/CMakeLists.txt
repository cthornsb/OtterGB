
set(CORE_SOURCES ConfigFile.cpp
                 Opcode.cpp
                 Support.cpp)

if(NOT WIN32)
	set(CORE_SOURCES ${CORE_SOURCES} optionHandler.cpp)
endif(NOT WIN32)

set(SYSTEM_SOURCES AudioUnit.cpp
                   Bitmap.cpp
                   Cartridge.cpp
                   ColorGBC.cpp
                   Console.cpp
                   DmaController.cpp
                   FrequencySweep.cpp
                   GPU.cpp
                   Joystick.cpp
                   LengthCounter.cpp
                   LR35902.cpp
                   SerialController.cpp
                   ShiftRegister.cpp
                   Sound.cpp
                   SquareWave.cpp
                   SystemComponent.cpp
                   SystemGBC.cpp
                   SystemRegisters.cpp
                   SystemTimer.cpp
                   TextParser.cpp
                   UnitTimer.cpp
                   VolumeEnvelope.cpp
                   WaveTable.cpp
                   WorkRam.cpp)

if(NOT ${GB_BOOT_ROM} STREQUAL "")
	add_definitions(-DGB_BOOT_ROM="${GB_BOOT_ROM}")
endif()

if(NOT ${GBC_BOOT_ROM} STREQUAL "")
	add_definitions(-DGBC_BOOT_ROM="${GBC_BOOT_ROM}")
endif()

# Generate the core library
add_library(CORE_OBJECTS OBJECT ${CORE_SOURCES})
add_library(CORE_LIB STATIC $<TARGET_OBJECTS:CORE_OBJECTS>)

# Generate the system component library
add_library(SYSTEM_OBJECTS OBJECT ${SYSTEM_SOURCES})
add_library(SYSTEM_LIB STATIC $<TARGET_OBJECTS:SYSTEM_OBJECTS>)

#Build renderer executable.
add_executable(gbc gbc.cpp)
if(NOT BUILD_DEBUG)
	target_link_libraries(gbc SYSTEM_LIB CORE_LIB GRAPHICS_LIB AUDIO_LIB ${EXTERNAL_GRAPHICS_LIBS} ${EXTERNAL_AUDIO_LIBS})
else()
	target_link_libraries(gbc SYSTEM_LIB CORE_LIB GRAPHICS_LIB AUDIO_LIB QTDEBUG_LIB ${QT_GUI_LIB} ${QT_CORE_LIB} ${QT_OPENGL_LIB} ${EXTERNAL_GRAPHICS_LIBS} ${EXTERNAL_AUDIO_LIBS})
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
