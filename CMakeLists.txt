cmake_minimum_required(VERSION 3.14)
project(SmartLibrary VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)

# ---- core: models + services + repositories + utils (pure C++, no sockets) ----
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
     backend/src/models/*.cpp backend/src/services/*.cpp
     backend/src/repositories/*.cpp backend/src/utils/*.cpp)
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*HttpServer\\.cpp$")
add_library(smartlibrary_core STATIC ${CORE_SOURCES})
target_include_directories(smartlibrary_core PUBLIC backend/include)

# ---- api: controllers + the thin HTTP transport ----
file(GLOB_RECURSE API_SOURCES CONFIGURE_DEPENDS backend/src/controllers/*.cpp)
add_library(smartlibrary_api STATIC ${API_SOURCES} backend/src/utils/HttpServer.cpp)
target_link_libraries(smartlibrary_api PUBLIC smartlibrary_core Threads::Threads)
if(WIN32)
  target_link_libraries(smartlibrary_api PUBLIC ws2_32)
endif()

# ---- server executable ----
add_executable(smartlibrary backend/main.cpp)
target_link_libraries(smartlibrary PRIVATE smartlibrary_api)
target_compile_definitions(smartlibrary PRIVATE SMARTLIBRARY_ROOT="${CMAKE_SOURCE_DIR}")

# ---- tests (own tiny framework, no downloads needed) ----
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
add_executable(smartlibrary_tests ${TEST_SOURCES})
target_link_libraries(smartlibrary_tests PRIVATE smartlibrary_api)
add_test(NAME smartlibrary_tests COMMAND smartlibrary_tests)
