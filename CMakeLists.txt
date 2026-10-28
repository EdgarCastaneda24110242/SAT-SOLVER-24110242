cmake_minimum_required(VERSION 3.16)
project(sat_solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
set(GUI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GuiApp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SoundManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TextInput.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/UITheme.cpp")

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    pkg_check_modules(SFML QUIET IMPORTED_TARGET sfml-graphics sfml-window sfml-system sfml-audio)
endif()

if (TARGET PkgConfig::SFML)
    add_executable(sat_solver ${SOURCES})
    target_compile_definitions(sat_solver PRIVATE SAT_HAS_SFML)
    target_link_libraries(sat_solver PRIVATE PkgConfig::SFML)
else()
    find_package(SFML 3 QUIET COMPONENTS Graphics Window System Audio)
    if (SFML_FOUND)
        add_executable(sat_solver ${SOURCES})
        target_compile_definitions(sat_solver PRIVATE SAT_HAS_SFML)
        target_link_libraries(sat_solver PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)
    else()
        message(STATUS "SFML not found: building console version")
        list(FILTER SOURCES EXCLUDE REGEX ".*/(GuiApp|SoundManager|TextInput|UITheme)\\.cpp$")
        add_executable(sat_solver ${SOURCES})
    endif()
endif()

target_include_directories(sat_solver PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (MSVC)
    target_compile_options(sat_solver PRIVATE /W4)
else()
    target_compile_options(sat_solver PRIVATE -Wall -Wextra -pedantic)
endif()
