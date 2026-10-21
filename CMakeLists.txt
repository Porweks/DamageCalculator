cmake_minimum_required(VERSION 3.16)
project(DamageCalculator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(DamageCalculator main.cpp)

target_link_libraries(DamageCalculator PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Для Windows, чтобы не открывалась консоль
if(WIN32)
    set_target_properties(DamageCalculator PROPERTIES WIN32_EXECUTABLE TRUE)
endif()