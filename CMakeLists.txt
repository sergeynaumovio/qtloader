# Copyright (C) 2025 Sergey Naumov <sergey@naumov.io>
# SPDX-License-Identifier: 0BSD

cmake_minimum_required(VERSION 3.18)

project(QtLoader LANGUAGES CXX)

if(MSVC)
    cmake_policy(SET CMP0092 NEW) # Disable passing /W3 by default on MSVC
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

find_package(QT NAMES Qt6)
find_package(Qt${QT_VERSION_MAJOR} 6.8 REQUIRED COMPONENTS Widgets)
#find_package(Python3 REQUIRED COMPONENTS Development)

if(NOT APPLE)
    set(CMAKE_INSTALL_RPATH $ORIGIN)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

# QT_LOADER_LIBRARY ------------------------------------------------------------
include("cmake/QtLoaderConfigVersion.in")
set(QT_LOADER_LIBRARY Qt${QT_VERSION_MAJOR}Loader)

add_library(${QT_LOADER_LIBRARY} SHARED)

target_compile_definitions(${QT_LOADER_LIBRARY}
    PRIVATE QT_NO_CAST_FROM_ASCII
            QT_LEAN_HEADERS
)

target_sources(${QT_LOADER_LIBRARY}
    PRIVATE src/core/shell/qloadercommandinterface.h
            src/core/shell/qloadershell.cpp
            src/core/shell/qloadershell.h
            src/core/shell/qloadershellcd.cpp
            src/core/shell/qloadershellcd.h
            src/core/shell/qloadershellexit.cpp
            src/core/shell/qloadershellexit.h
            src/core/shell/qloadershellsave.cpp
            src/core/shell/qloadershellsave.h
            src/core/qloadererror.h
            src/core/qloaderplugininterface.h
            src/core/qloadersaveinterface.h
            src/core/qloadersettings.cpp
            src/core/qloadersettings.h
            src/core/qloadertree.cpp
            src/core/qloadertree.h
            src/core/qloadertree_p.cpp
            src/core/qloadertree_p.h
            src/core/qscopedstorage.h
            src/core/qtloaderglobal.h
            src/widgets/qloadermarkupcommander.cpp
            src/widgets/qloadermarkupcommander.h
            src/widgets/qloadermarkupeditor.cpp
            src/widgets/qloadermarkupeditor.h
            src/widgets/qloaderterminal.cpp
            src/widgets/qloaderterminal.h
)

target_include_directories(${QT_LOADER_LIBRARY}
    PRIVATE src/core/shell
            src/core
            src/widgets
)

set(QT_LOADER_PUBLIC_HEADERS
    src/core/shell/QLoaderCommandInterface
    src/core/shell/QLoaderShell
    src/core/shell/qloadercommandinterface.h
    src/core/shell/qloadershell.h
    src/core/QLoaderError
    src/core/QLoaderPluginInterface
    src/core/QLoaderSaveInterface
    src/core/QLoaderSettings
    src/core/QLoaderTree
    src/core/QScopedStorage
    src/core/qloadererror.h
    src/core/qloaderplugininterface.h
    src/core/qloadersaveinterface.h
    src/core/qloadersettings.h
    src/core/qloadertree.h
    src/core/qscopedstorage.h
    src/core/qtloaderglobal.h
    src/widgets/QLoaderMarkupCommander
    src/widgets/QLoaderMarkupEditor
    src/widgets/QLoaderTerminal
    src/widgets/qloadermarkupcommander.h
    src/widgets/qloadermarkupeditor.h
    src/widgets/qloaderterminal.h
)

set_target_properties(${QT_LOADER_LIBRARY} PROPERTIES
    OUTPUT_NAME ${QT_LOADER_LIBRARY}
    SOVERSION ${QT_VERSION_MAJOR}
    VERSION ${QT_LOADER_VERSION}
    PUBLIC_HEADER "${QT_LOADER_PUBLIC_HEADERS}"
)

target_link_libraries(${QT_LOADER_LIBRARY}
    PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
)

target_compile_definitions(${QT_LOADER_LIBRARY}
    PRIVATE QT_BUILD_LOADER_LIB
)

include(GNUInstallDirs)
set(QT_LOADER_INCLUDEDIR qt${QT_VERSION_MAJOR}/QtLoader)
install(TARGETS ${QT_LOADER_LIBRARY}
    EXPORT ${QT_LOADER_LIBRARY}Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT QtLoader_Runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT QtLoader_Runtime
            NAMELINK_COMPONENT QtLoader_Development
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT QtLoader_Development
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${QT_LOADER_INCLUDEDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${QT_LOADER_INCLUDEDIR}
                  COMPONENT QtLoader_Development
)

install(EXPORT ${QT_LOADER_LIBRARY}Targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${QT_LOADER_LIBRARY}
    FILE Qt${QT_VERSION_MAJOR}LoaderTargets.cmake
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/QtLoaderConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${QT_LOADER_LIBRARY}Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${QT_LOADER_LIBRARY}
    PATH_VARS CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${QT_LOADER_LIBRARY}ConfigVersion.cmake
    VERSION ${QT_LOADER_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${QT_LOADER_LIBRARY}Config.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/${QT_LOADER_LIBRARY}ConfigVersion.cmake"
          DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${QT_LOADER_LIBRARY}"
)

# QT_LOADER_EXECUTABLE ---------------------------------------------------------
set(QT_LOADER_EXECUTABLE qtloader${QT_VERSION_MAJOR})

add_executable(${QT_LOADER_EXECUTABLE})

target_compile_definitions(${QT_LOADER_EXECUTABLE}
    PRIVATE QT_NO_CAST_FROM_ASCII
)

set_target_properties(${QT_LOADER_EXECUTABLE} PROPERTIES
    OUTPUT_NAME ${QT_LOADER_EXECUTABLE}
)

if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )

    set_target_properties(${QT_LOADER_EXECUTABLE} PROPERTIES
        LINK_FLAGS /SUBSYSTEM:WINDOWS
    )
endif()

target_sources(${QT_LOADER_EXECUTABLE}
    PRIVATE src/main.cpp
)

target_include_directories(${QT_LOADER_EXECUTABLE}
    PRIVATE src/core
)

target_link_libraries(${QT_LOADER_EXECUTABLE}
    PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
    PRIVATE ${QT_LOADER_LIBRARY}
    PRIVATE ${PYTHON_LINK_FLAGS}
    #PRIVATE Python3::Python
)

if(UNIX)
    list(GET PYTHON_LINKING_DATA 0 PYTHON_LIBDIR)
    list(GET PYTHON_LINKING_DATA 1 PYTHON_LIB)
    find_library(PYTHON_LINK_FLAGS ${PYTHON_LIB} PATHS ${PYTHON_LIBDIR} HINTS ${PYTHON_LIBDIR})
endif()

install(TARGETS ${QT_LOADER_EXECUTABLE}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT QtLoader_Runtime
)
