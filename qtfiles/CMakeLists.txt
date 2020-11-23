set(QT_DEFINES "-DQT_NO_DEBUG -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED")
set(QT_INCPATH "-I${QT_INC_DIR}")

set(QT_HEADER ${TOP_DIRECTORY}/qtfiles/mainwindow.h)
set(QT_UI_HEADER ${TOP_DIRECTORY}/qtfiles/mainwindow.ui)
set(MOC_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/moc_mainwindow.cpp)
set(UIC_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/ui_mainwindow.h)

# Use moc to generate the moc_mainwindow source file.
add_custom_target(MocFile ALL DEPENDS ${MOC_FILENAME})
add_custom_command(OUTPUT ${MOC_FILENAME} COMMAND ${MOC_EXECUTABLE} ${QT_DEFINES} ${QT_INCPATH} ${QT_HEADER} -o ${MOC_FILENAME} DEPENDS ${QT_HEADER} VERBATIM)

# Use uic to generate the ui_mainwindow header file.
add_custom_target(UicFile ALL DEPENDS ${UIC_FILENAME})
add_custom_command(OUTPUT ${UIC_FILENAME} COMMAND ${UIC_EXECUTABLE} ${QT_UI_HEADER} -o ${UIC_FILENAME} DEPENDS ${QT_UI_HEADER} VERBATIM)

# Include the newly generated files
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Generate the QT window library
add_library(QTDEBUG_OBJECTS OBJECT mainwindow.cpp ${MOC_FILENAME})
add_library(QTDEBUG_LIB STATIC $<TARGET_OBJECTS:QTDEBUG_OBJECTS>)
