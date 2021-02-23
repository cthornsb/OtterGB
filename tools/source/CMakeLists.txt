#add_executable(disassembler disassembler.cpp)
#install(TARGETS disassembler DESTINATION bin)

add_executable(dmgpalettes palettes.cpp)
target_link_libraries(dmgpalettes GRAPHICS_LIB)
install(TARGETS dmgpalettes DESTINATION bin)
