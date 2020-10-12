set(CORE_SOURCES sdlWindow.cpp colors.cpp LR35902.cpp SystemComponent.cpp GPU.cpp Cartridge.cpp Sound.cpp Joystick.cpp WorkRam.cpp SystemRegisters.cpp SystemTimer.cpp SystemGBC.cpp Support.cpp)

#Add the sources to the library.
add_library(CORE_OBJECTS OBJECT ${CORE_SOURCES})

#Generate a static library.
add_library(CORE_LIB STATIC $<TARGET_OBJECTS:CORE_OBJECTS>)

# To install SDL:
#sudo apt-get install libsdl2-dev

#Build renderer executable.
add_executable(gbc gbc.cpp)
target_link_libraries(gbc CORE_LIB -lSDL2)
install(TARGETS gbc DESTINATION bin)
