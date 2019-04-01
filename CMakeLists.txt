cmake_minimum_required(VERSION 3.4)
project(ble++)

set(CMAKE_CXX_STANDARD 11)

set(BINARIES
    src/crownstone.cc)

set(DEPS
    thd/aes.c)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(Bluez REQUIRED)
find_package(blepp REQUIRED)
find_package(CURL REQUIRED)

include_directories(${PROJECT_SOURCE_DIR} ${BLUEZ_INCLUDE_DIRS} ${BLEPP_INCLUDE_DIRS})

foreach (binary_src ${BINARIES})
    get_filename_component(binary_name ${binary_src} NAME_WE)
    message(STATUS "Configure \"${binary_name}\" to be created subsequently by make")

    add_executable(${binary_name} ${binary_src} ${DEPS})
    target_link_libraries(${binary_name} ${PROJECT_NAME} ${BLUEZ_LIBRARIES} ${BLEPP_LIBRARIES} ${CURL_LIBRARIES})
endforeach()

