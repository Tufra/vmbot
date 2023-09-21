cmake_minimum_required(VERSION 3.15)
project(vmbot CXX)

set(CMAKE_CXX_STANDARD 20)
MESSAGE("This is the C++ Standard we are using: ${CMAKE_CXX_STANDARD}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set(Boost_ROOT /usr/local/boost_1_82_0)
# set(BOOST_LIBRARYDIR /usr/local/boost_1_82_0/libs)
find_package(OpenSSL REQUIRED)
find_package(Boost 1.82.0 REQUIRED)
find_package(fmt REQUIRED)
# include_directories(${Boost_INCLUDE_DIR})

# set(Boost_DEBUG ON)

IF (Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
    ADD_DEFINITIONS( "-DHAS_BOOST" )
ENDIF()

set(SRC_DIR "src/")
set(SOURCES 
    ${SRC_DIR}/main.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} 
    ${Boost_LIBRARIES}
    fmt::fmt 
    openssl::openssl)