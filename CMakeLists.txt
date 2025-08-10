cmake_minimum_required(VERSION 3.10)
project(DistributedFileShare)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(src)

# Find POCO libraries directly without pkg-config
find_library(POCO_FOUNDATION PocoFoundation)
find_library(POCO_NET PocoNet)
find_library(POCO_UTIL PocoUtil)
find_library(POCO_DATA PocoData)
find_library(POCO_DATA_POSTGRESQL PocoDataPostgreSQL)
find_library(POCO_CRYPTO PocoCrypto)
find_library(POCO_JSON PocoJSON)

# Check if all libraries were found
if(NOT POCO_FOUNDATION OR NOT POCO_NET OR NOT POCO_UTIL OR 
   NOT POCO_DATA OR NOT POCO_DATA_POSTGRESQL OR NOT POCO_CRYPTO OR NOT POCO_JSON)
    message(FATAL_ERROR "POCO libraries not found. Please install libpoco-dev")
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/Database.cpp
    src/User.cpp
    src/FileManager.cpp
    src/WebServer.cpp
    src/Utils.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link POCO libraries
target_link_libraries(${PROJECT_NAME}
    ${POCO_FOUNDATION}
    ${POCO_NET}
    ${POCO_UTIL}
    ${POCO_DATA}
    ${POCO_DATA_POSTGRESQL}
    ${POCO_CRYPTO}
    ${POCO_JSON}
)

# Create uploads directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/uploads)
