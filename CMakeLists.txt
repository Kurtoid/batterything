cmake_minimum_required(VERSION 3.10)
project(kurt_battery_thing VERSION 0.1.0 LANGUAGES CXX)
# use clang
set(CMAKE_GENERATOR "Ninja")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set sdbus-c++ module path
find_package(sdbus-c++ REQUIRED)
find_package(X11 REQUIRED)
# use plog from extern folder, only if it exists (otherwise, the system plog will be used)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/extern/plog/include)
    message(STATUS "Using plog from extern folder")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/extern/plog/include)
else()
    message(STATUS "Using plog from system")
endif()

add_library(processes ${CMAKE_CURRENT_SOURCE_DIR}/src/processes.cpp)

find_package(Qt6 REQUIRED COMPONENTS Core Gui UiTools Widgets)
set_target_properties(Qt6::Core PROPERTIES INTERFACE_COMPILE_OPTIONS "")
qt_standard_project_setup()


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(kwplogformatter ${CMAKE_CURRENT_SOURCE_DIR}/src/plogformatter.cpp)

add_library(battery_thing_cgroups ${CMAKE_CURRENT_SOURCE_DIR}/src/cgroups.cpp)
target_link_libraries(battery_thing_cgroups PUBLIC
    sdbus-c++
    kwplogformatter
)

add_executable(battery_thing ${CMAKE_CURRENT_SOURCE_DIR}/src/battery_thing.cpp)
target_link_libraries(battery_thing PUBLIC
    battery_thing_cgroups
    processes
    Qt::Core
    Qt::Gui
    Qt::UiTools
    Qt::Widgets
)

add_library(windowfocus ${CMAKE_CURRENT_SOURCE_DIR}/src/windowfocus.cpp)
target_link_libraries(windowfocus PUBLIC
X11
)

add_executable(background_mode ${CMAKE_CURRENT_SOURCE_DIR}/src/background_mode.cpp)
target_link_libraries(background_mode PUBLIC
    battery_thing_cgroups
    processes
    windowfocus
    kwplogformatter
    Qt::Core
    Qt::Gui
    Qt::UiTools
    Qt::Widgets
)


# add_library(battery_thing_ui ${CMAKE_CURRENT_SOURCE_DIR}/src/battery_thing_ui.cpp)
# target_link_libraries(battery_thing_ui PUBLIC
#     Qt::Core
#     Qt::Gui
#     Qt::UiTools
#     Qt::Widgets
# )
