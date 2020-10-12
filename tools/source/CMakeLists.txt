#Build renderer executable.
add_executable(disassembler disassembler.cpp)
install(TARGETS disassembler DESTINATION bin)
