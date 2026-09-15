cmake_minimum_required(VERSION 3.16)

project(TaskManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(taskmanager_lib
    src/Task.cpp
    src/TaskManager.cpp
    src/CommandParser.cpp
    src/CLI.cpp
)

target_include_directories(taskmanager_lib PUBLIC include)

add_executable(taskmanager src/main.cpp)
target_link_libraries(taskmanager PRIVATE taskmanager_lib)

if(WIN32)
    target_link_libraries(taskmanager PRIVATE shell32)
endif()

enable_testing()

add_executable(taskmanager_tests tests/TaskManagerTests.cpp)
target_link_libraries(taskmanager_tests PRIVATE taskmanager_lib)

add_test(NAME TaskManagerTests COMMAND taskmanager_tests)
