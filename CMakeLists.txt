cmake_minimum_required(VERSION 3.16)

#
#   UPDATE THE VERSION NUMBER:
#
project(Tablecruncher VERSION 1.8.10)


#
# NO MORE SETTINGS BENEATH THIS LINE
#

if(PROJECT_VERSION_PATCH LESS 10)
    set(APP_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}-beta${PROJECT_VERSION_PATCH}")
elseif(PROJECT_VERSION_PATCH EQUAL 10)
    set(APP_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
else()
    math(EXPR APP_VERSION_PATCH "${PROJECT_VERSION_PATCH}-10" OUTPUT_FORMAT DECIMAL) 
    set(APP_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${APP_VERSION_PATCH}")
endif()


message(STATUS ">>> Calculated APP_VERSION: ${APP_VERSION}")


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optional: single combined FLTKDIR
set(FLTKDIR "" CACHE PATH "Optional root path to FLTK (used if FLTKINCDIR and FLTKLIBDIR are not set)")

# FLTKINCDIR and FLTKLIBDIR (can override FLTKDIR)
set(FLTKINCDIR "" CACHE PATH "Path to FLTK include directory (optional if FLTKDIR is set)")
set(FLTKLIBDIR "" CACHE PATH "Path to FLTK lib directory (optional if FLTKDIR is set)")

# Fallback logic
if(NOT FLTKINCDIR)
    if(FLTKDIR)
        set(FLTKINCDIR ${FLTKDIR})
    else()
        message(FATAL_ERROR "Please set either FLTKINCDIR or FLTKDIR.")
    endif()
endif()

if(NOT FLTKLIBDIR)
    if(FLTKDIR)
        set(FLTKLIBDIR ${FLTKDIR})
    else()
        message(FATAL_ERROR "Please set either FLTKLIBDIR or FLTKDIR.")
    endif()
endif()


if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

string(TIMESTAMP NOW "%Y-%m-%dT%H:%M")
string(TIMESTAMP TODAY "%Y-%m-%d")
string(TIMESTAMP YEAR "%Y")


set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")


# Detect the platform
if(WIN32)
    set(PLATFORM "Windows")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /DMY_VERSION=\\\"${APP_VERSION}\\\" /DMY_YEAR=\\\"${YEAR}\\\" /DMY_TODAY=\\\"${TODAY}\\\" /DMY_NOW=\\\"${NOW}\\\"  /D_FILE_OFFSET_BITS=64 /DWIN32_LEAN_AND_MEAN /DNOMINMAX")
    set(LINKER_FLAGS "/NODEFAULTLIB:library")
elseif(APPLE)
    execute_process(COMMAND uname -p OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(ARCH STREQUAL "arm")
        set(PLATFORM "macOS_ARM")
    else()
        set(PLATFORM "macOS_X86")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -DMY_VERSION=\\\"${APP_VERSION}\\\" -DMY_YEAR=\\\"${YEAR}\\\" -DMY_TODAY=\\\"${TODAY}\\\" -DMY_NOW=\\\"${NOW}\\\" -mmacosx-version-min=${MACOS_VERSION} -D_THREAD_SAFE -D_REENTRANT")
    set(CMAKE_C_FLAGS "-mmacosx-version-min=${MACOS_VERSION}")
elseif(LINUX)
    set(PLATFORM "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -DMY_VERSION=\\\"${APP_VERSION}\\\" -DMY_YEAR=\\\"${YEAR}\\\" -DMY_TODAY=\\\"${TODAY}\\\" -DMY_NOW=\\\"${NOW}\\\" -D_THREAD_SAFE -D_REENTRANT")
    if(APPIMAGE STREQUAL "1")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--dynamic-linker=/lib64/ld-linux-x86-64.so.2")
    endif()
endif()

# Source and header files
set(SRCDIR "src")
set(EXTERNAL "external")

set(SOURCES
    ${SRCDIR}/colorthemes.cpp
    ${SRCDIR}/csvapplication.cpp
    ${SRCDIR}/csvdatastorage.cpp
    ${SRCDIR}/csvgrid.cpp
    ${SRCDIR}/csvmenu.cpp
    ${SRCDIR}/csvparser.cpp
    ${SRCDIR}/csvtable.cpp
    ${SRCDIR}/csvundo.cpp
    ${SRCDIR}/csvwidgets.cpp
    ${SRCDIR}/csvwindow.cpp
    ${SRCDIR}/helper.cpp
    ${SRCDIR}/macro.cpp
    ${SRCDIR}/main.cpp
    ${EXTERNAL}/duktape/duktape.c
)

include_directories(${SRCDIR} ${EXTERNAL} ${FLTKINCDIR} ${FLTKLIBDIR})

if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

# Link libraries
if(APPLE)
    if(ARCH STREQUAL "arm")
        target_link_libraries(${PROJECT_NAME} PRIVATE
            "-framework Cocoa"
            "-weak_framework UniformTypeIdentifiers"
        )
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE
            "-framework Cocoa"
        )
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${FLTKLIBDIR}/lib/libfltk.a
        ${FLTKLIBDIR}/lib/libfltk_images.a
        ${FLTKLIBDIR}/lib/libfltk_z.a
    )
elseif(WIN32)
    set(FLTK_DIR ${FLTKLIBDIR})
    find_package(FLTK 1.4 CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE fltk::fltk fltk::images fltk::z)
    target_link_options(${PROJECT_NAME} PRIVATE ${LINKER_FLAGS})
    target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
elseif(LINUX)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${FLTKLIBDIR}/lib/libfltk.a
        ${FLTKLIBDIR}/lib/libfltk_images.a
        ${FLTKLIBDIR}/lib/libfltk_z.a
        Xft
        fontconfig
        Xrender
        X11
        pthread
        dl
    )
endif()


file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/dist)

# Output paths
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dist"
)


if(APPLE)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/create-bundle.sh ${MACOS_VERSION} ${APP_VERSION}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Creating macOS App Bundle"
    )
    add_custom_target(create_bundle
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/create-bundle.sh ${MACOS_VERSION} ${APP_VERSION}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Creating macOS App Bundle"
    )
endif()



if(FALSE)
    # Install binary
    install(TARGETS tablecruncher RUNTIME DESTINATION bin)

    # Desktop entry
    install(FILES assets/linux/tablecruncher.desktop
        DESTINATION share/applications)

    # App icon
    install(FILES assets/linux/icons/64x64/tablecruncher.png
        DESTINATION share/icons/hicolor/64x64/apps)

    install(FILES assets/linux/icons/128x128/tablecruncher.png
        DESTINATION share/icons/hicolor/128x128/apps)

    install(FILES assets/linux/icons/256x256/tablecruncher.png
        DESTINATION share/icons/hicolor/256x256/apps)
endif()




# Custom clean target
add_custom_target(custom_clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/dist
    COMMENT "Cleaning build directory"
)
