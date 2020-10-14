add_executable(disassembler disassembler.cpp)
install(TARGETS disassembler DESTINATION bin)

add_executable(interpreter interpreter.cpp)
target_link_libraries(interpreter CORE_LIB -lSDL2)
install(TARGETS interpreter DESTINATION bin)
