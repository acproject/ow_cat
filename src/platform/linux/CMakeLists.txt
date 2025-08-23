# Linux Platform CMakeLists.txt

# Find required packages for Linux
find_package(PkgConfig REQUIRED)

# Find GTK+3
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# Find IBus
pkg_check_modules(IBUS REQUIRED ibus-1.0)

# Find GLib
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Find GObject
pkg_check_modules(GOBJECT REQUIRED gobject-2.0)

# Find Pango
pkg_check_modules(PANGO REQUIRED pango)

# Find Cairo
pkg_check_modules(CAIRO REQUIRED cairo)

# Find X11
find_package(X11 REQUIRED)

# Find libprocps (for process information)
find_library(PROCPS_LIBRARY NAMES procps proc)
if(NOT PROCPS_LIBRARY)
    message(WARNING "libprocps not found, process information features may be limited")
endif()

# Define source files
set(LINUX_PLATFORM_SOURCES
    linux_ime_adapter.cpp
    linux_input_engine.cpp
    linux_candidate_window.cpp
    linux_system_integration.cpp
)

# Define header files
set(LINUX_PLATFORM_HEADERS
    ${CMAKE_SOURCE_DIR}/include/platform/linux/linux_ime_adapter.h
)

# Create static library
add_library(owcat_platform_linux STATIC
    ${LINUX_PLATFORM_SOURCES}
    ${LINUX_PLATFORM_HEADERS}
)

# Set C++ standard
target_compile_features(owcat_platform_linux PRIVATE cxx_std_17)

# Include directories
target_include_directories(owcat_platform_linux
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/platform/linux
    PRIVATE
        ${GTK3_INCLUDE_DIRS}
        ${IBUS_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
        ${GOBJECT_INCLUDE_DIRS}
        ${PANGO_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
        ${X11_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(owcat_platform_linux
    PUBLIC
        owcat_core
    PRIVATE
        ${GTK3_LIBRARIES}
        ${IBUS_LIBRARIES}
        ${GLIB_LIBRARIES}
        ${GOBJECT_LIBRARIES}
        ${PANGO_LIBRARIES}
        ${CAIRO_LIBRARIES}
        ${X11_LIBRARIES}
)

# Add libprocps if found
if(PROCPS_LIBRARY)
    target_link_libraries(owcat_platform_linux PRIVATE ${PROCPS_LIBRARY})
    target_compile_definitions(owcat_platform_linux PRIVATE HAVE_LIBPROCPS=1)
endif()

# Compiler flags
target_compile_options(owcat_platform_linux
    PRIVATE
        ${GTK3_CFLAGS_OTHER}
        ${IBUS_CFLAGS_OTHER}
        ${GLIB_CFLAGS_OTHER}
        ${GOBJECT_CFLAGS_OTHER}
        ${PANGO_CFLAGS_OTHER}
        ${CAIRO_CFLAGS_OTHER}
        -Wall
        -Wextra
        -Wpedantic
        -Wno-deprecated-declarations
)

# Define platform-specific macros
target_compile_definitions(owcat_platform_linux
    PRIVATE
        OWCAT_PLATFORM_LINUX=1
        OWCAT_USE_IBUS=1
        OWCAT_USE_GTK=1
        OWCAT_USE_X11=1
        G_LOG_DOMAIN="OwCat"
        GETTEXT_PACKAGE="owcat"
)

# Link directories
target_link_directories(owcat_platform_linux
    PRIVATE
        ${GTK3_LIBRARY_DIRS}
        ${IBUS_LIBRARY_DIRS}
        ${GLIB_LIBRARY_DIRS}
        ${GOBJECT_LIBRARY_DIRS}
        ${PANGO_LIBRARY_DIRS}
        ${CAIRO_LIBRARY_DIRS}
)

# Set library properties
set_target_properties(owcat_platform_linux PROPERTIES
    OUTPUT_NAME "owcat_platform_linux"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    POSITION_INDEPENDENT_CODE ON
)

# Installation
install(TARGETS owcat_platform_linux
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${LINUX_PLATFORM_HEADERS}
    DESTINATION include/platform/linux
)

# Create IBus engine executable (optional)
if(BUILD_IBUS_ENGINE)
    add_executable(ibus-engine-owcat
        ibus_engine_main.cpp
    )
    
    target_link_libraries(ibus-engine-owcat
        owcat_platform_linux
        owcat_core
        ${IBUS_LIBRARIES}
        ${GLIB_LIBRARIES}
        ${GOBJECT_LIBRARIES}
    )
    
    target_include_directories(ibus-engine-owcat
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${IBUS_INCLUDE_DIRS}
            ${GLIB_INCLUDE_DIRS}
            ${GOBJECT_INCLUDE_DIRS}
    )
    
    target_compile_definitions(ibus-engine-owcat
        PRIVATE
            OWCAT_PLATFORM_LINUX=1
            OWCAT_USE_IBUS=1
    )
    
    install(TARGETS ibus-engine-owcat
        RUNTIME DESTINATION libexec
    )
    
    # Install IBus component file
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/owcat.xml.in
        ${CMAKE_CURRENT_BINARY_DIR}/owcat.xml
        @ONLY
    )
    
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/owcat.xml
        DESTINATION share/ibus/component
    )
endif()

# Debug information
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Linux Platform Configuration:")
    message(STATUS "  GTK3 version: ${GTK3_VERSION}")
    message(STATUS "  IBus version: ${IBUS_VERSION}")
    message(STATUS "  GLib version: ${GLIB_VERSION}")
    message(STATUS "  X11 found: ${X11_FOUND}")
    message(STATUS "  libprocps: ${PROCPS_LIBRARY}")
    message(STATUS "  Build IBus engine: ${BUILD_IBUS_ENGINE}")
endif()

# Warnings for missing dependencies
if(NOT GTK3_FOUND)
    message(WARNING "GTK+3 not found. GUI features will be disabled.")
endif()

if(NOT IBUS_FOUND)
    message(WARNING "IBus not found. IBus integration will be disabled.")
endif()

if(NOT X11_FOUND)
    message(WARNING "X11 not found. Some display features may not work.")
endif()

# Add custom target for generating IBus component file
add_custom_target(generate_ibus_component
    COMMAND ${CMAKE_COMMAND} -E echo "Generating IBus component file..."
    COMMENT "Generating IBus component configuration"
)

# Add dependency
add_dependencies(owcat_platform_linux generate_ibus_component)

# Platform-specific compiler warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(owcat_platform_linux PRIVATE
        -Wno-unused-parameter
        -Wno-missing-field-initializers
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(owcat_platform_linux PRIVATE
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        -Wno-gnu-zero-variadic-macro-arguments
    )
endif()

# Enable testing if requested
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()