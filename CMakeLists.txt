cmake_minimum_required(VERSION 3.31)
project(MindBogglerCPP)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)

add_executable(MindBogglerCPP main.cpp
        INTERPRETER/Interpreter.cpp
        INTERPRETER/Interpreter.h
        MainWindow/MainWindow.cpp
        MainWindow/MainWindow.h
        resources.qrc)

target_link_libraries(MindBogglerCPP
        Qt6::Core
        Qt6::Widgets
)

set_target_properties(MindBogglerCPP PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS MindBogglerCPP
        RUNTIME DESTINATION bin
)

if(EXISTS "${CMAKE_SOURCE_DIR}/resources/icon.ico")
    configure_file("${CMAKE_SOURCE_DIR}/resources/icon.ico"
            "${CMAKE_BINARY_DIR}/bin/resources/icon.ico" COPYONLY)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/resources/icon.png")
    configure_file("${CMAKE_SOURCE_DIR}/resources/icon.png"
            "${CMAKE_BINARY_DIR}/bin/resources/icon.png" COPYONLY)
endif()


if(WIN32)
    # Commented out WIN32_EXECUTABLE to avoid MinGW linking issues
    # One day I'll sort this out
    # set_target_properties(MindBogglerCPP PROPERTIES
    #         WIN32_EXECUTABLE TRUE
    # )
elseif(APPLE)
    set_target_properties(MindBogglerCPP PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER com.mindbogglerapp.MindBogglerCPP
            MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
endif()

if(MSVC)
    target_compile_options(MindBogglerCPP PRIVATE /W4)
else()
    target_compile_options(MindBogglerCPP PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(MindBogglerCPP PRIVATE DEBUG)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(MindBogglerCPP PRIVATE NDEBUG)
    if(NOT MSVC)
        target_compile_options(MindBogglerCPP PRIVATE -O3)
    endif()
endif()