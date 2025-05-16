cmake_minimum_required(VERSION 3.5)

project(Server VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core LinguistTools Network Sql REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core LinguistTools Network Sql REQUIRED)


set(PROJECT_SOURCES
        AddMessageTask.h
        AddMessageTask.cpp
        ChatDataWrapper.h
        CheckUsernameTask.h
        CheckUsernameTask.cpp
        ChatDataWrapper.cpp
        GetHistoryTask.h
        GetHistoryTask.cpp
        main.cpp
        SqlConnectionManager.h
        SqlConnectionManager.cpp
        SqlTask.h
        SqlTask.cpp
        TcpServer.h
        TcpServer.cpp
        TcpServerExecutor.h
        TcpServerExecutor.cpp
        UuidHash.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Server
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(Server
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(Server PRIVATE Qt${QT_VERSION_MAJOR}::Core
    PRIVATE Qt${QT_VERSION_MAJOR}::Network
    PRIVATE Qt${QT_VERSION_MAJOR}::Sql
    PRIVATE TcpDataTransmitter
    PRIVATE MessagingSystem)

set_target_properties(Server PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Server)
endif()
