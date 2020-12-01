set(CORE_SOURCES LR35902.cpp SystemComponent.cpp GPU.cpp Cartridge.cpp Sound.cpp Joystick.cpp WorkRam.cpp DmaController.cpp SerialController.cpp SystemRegisters.cpp SystemTimer.cpp SystemGBC.cpp Support.cpp optionHandler.cpp Bitmap.cpp)

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
