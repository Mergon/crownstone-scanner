find_library(BLEPP_LIBRARIES
	NAMES
	ble++
	PATHS
	"/usr/"
	"/usr/local/"
	PATH_SUFFIXES
	lib)

find_path(BLEPP_INCLUDE_DIRS
	NAMES
	blepp/lescan.h
	PATHS
	"/usr/"
	"/usr/local/"
	PATH_SUFFIXES
	include)

message(STATUS "Found blepp include directories: ${BLEPP_INCLUDE_DIRS}")
message(STATUS "Found blepp libraries: ${BLEPP_LIBRARIES}")
