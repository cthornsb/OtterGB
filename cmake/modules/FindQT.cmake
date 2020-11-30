# - Finds Qt installation
# This module sets up Qt information 
# It defines:
#
# QT_FOUND        If the qt installation is found
# QT_LIB_DIR      PATH to the qt install directory
# QT_GUI_LIB      Path to libQtGui.so
# QT_CORE_LIB     Path to libQtCore.so
# QT_OPENGL_LIB   Path to libQtOpenGL.so
# QT_INC_DIR      Path to Qt include directory
# QT_CORE_INC_DIR Path to QtCore include directory
# QT_GUI_INC_DIR  Path to QtGui include directory
# MOC_EXECUTABLE  Path to moc executable
# UIC_EXECUTABLE  Path to uic executable
#
#Last updated by C. R. Thornsberry (desertedotter@gmail.com) on Nov. 22nd, 2020

#Look for a valid qt4 install.
find_path(QT_LIB_DIR
	NAMES libQtGui.so libQtCore.so
	PATHS /usr/lib
	PATH_SUFFIXES x86_64-linux-gnu i386-linux-gnu)

if(QT_LIB_DIR)
	set(QT_GUI_LIB ${QT_LIB_DIR}/libQtGui.so)
	set(QT_CORE_LIB ${QT_LIB_DIR}/libQtCore.so)
	set(QT_OPENGL_LIB ${QT_LIB_DIR}/libQtOpenGL.so)

	find_path(QT_INC_DIR
		NAMES QtCore QtGui
		PATHS /usr/include/qt4)

	find_path(QT_CORE_INC_DIR
		NAMES qcoreapplication.h
		PATHS ${QT_INC_DIR}
		PATH_SUFFIXES QtCore)

	find_path(QT_GUI_INC_DIR
		NAMES qapplication.h
		PATHS ${QT_INC_DIR}
		PATH_SUFFIXES QtGui)

	find_program(MOC_EXECUTABLE moc ${QT_LIB_DIR}/qt4/bin/)
	find_program(UIC_EXECUTABLE uic ${QT_LIB_DIR}/qt4/bin/)
endif()

#---Report the status of finding paass-------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QT DEFAULT_MSG QT_LIB_DIR QT_INC_DIR QT_CORE_INC_DIR QT_GUI_INC_DIR MOC_EXECUTABLE UIC_EXECUTABLE)

mark_as_advanced(FORCE MOC_EXECUTABLE UIC_EXECUTABLE)

