cmake_minimum_required(VERSION 3.14)
project(SchedulerGUI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# REQUIRED FOR QT6
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add source files
add_executable(SchedulerGUI
    main.cpp
    mainwindow.cpp
    mainwindow.h
    scheduler.cpp
    scheduler.h
    threadcontrol.cpp
    threadcontrol.h
    threadedscheduler.cpp
    threadedscheduler.h
    ganttwidget.cpp
    ganttwidget.h
)


target_link_libraries(SchedulerGUI Qt6::Widgets)
